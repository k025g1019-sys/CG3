#include "Engine/Graphics/TextureManager.h"
#include <cassert>
#include <cstring>
#include <wrl.h>
#include "Engine/String/ConvertString.h"
#include "Engine/Core/DirectXCore.h"
#include "Engine/Graphics/GpuResource.h"
#include "Engine/Graphics/DescriptorHeapManager.h"
#include "externals/DirectXTex/d3dx12.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace Engine {

namespace {

    // 動的テクスチャの画素形式はRGBA8（1画素4バイト）
    constexpr size_t kBytesPerPixel = sizeof(uint32_t);

    // リソースの状態遷移バリアを1つ積む
    void TransitionBarrier(
        ID3D12GraphicsCommandList* commandList,
        ID3D12Resource* resource,
        D3D12_RESOURCE_STATES before,
        D3D12_RESOURCE_STATES after) {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = resource;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = before;
        barrier.Transition.StateAfter = after;
        commandList->ResourceBarrier(1, &barrier);
    }

} // namespace

TextureManager* TextureManager::GetInstance() {
    static TextureManager instance;
    return &instance;
}

void TextureManager::Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList) {
    assert(device != nullptr);
    assert(commandList != nullptr);

    device_ = device;
    commandList_ = commandList;
}

uint32_t TextureManager::Load(const std::string& filepath) {
    assert(device_ != nullptr && "TextureManager::Initializeが呼ばれていない");

    // 読み込み済みならキャッシュから返す
    uint32_t handle = 0;
    if (FindHandle(filepath, handle)) {
        return handle;
    }

    // 読み込み → リソース作成 → 転送コマンド発行 → SRV作成 → キャッシュ登録
    ScratchImage mipImages = LoadTextureImage(filepath);
    return Register(filepath, mipImages);
}

uint32_t TextureManager::Reload(const std::string& filepath) {
    assert(device_ != nullptr && "TextureManager::Initializeが呼ばれていない");

    // まだ読み込んでいなければ通常のLoadと同じ
    uint32_t handle = 0;
    if (!FindHandle(filepath, handle)) {
        return Load(filepath);
    }

    Texture& texture = textures_[handle];
    assert(!texture.isDynamic && "動的テクスチャはReloadできない（ファイルに対応しない）");

    // 前フレームまでの描画が古いリソース・SRVを参照し終わるのを待ってから差し替える
    DirectXCore::GetInstance()->WaitForGPU();

    // 古いリソースは、記録中のコマンドリスト（同じフレームでLoadした転送コマンド等）が参照している
    // 可能性があるので、すぐには解放せず数フレーム保持してから解放する
    DeferRelease(texture.resource);
    DeferRelease(texture.intermediate);

    // 読み直し → 本体を作り直し → 転送コマンド発行 → 同じスロットへSRVを作り直す（ハンドルは変わらない）
    ScratchImage mipImages = LoadTextureImage(filepath);
    texture.metadata = mipImages.GetMetadata();
    texture.resource = CreateTextureResource(device_, texture.metadata);
    texture.intermediate = UploadTextureData(texture.resource.Get(), mipImages, device_, commandList_);
    texture.state = D3D12_RESOURCE_STATE_GENERIC_READ;  // UploadTextureDataの末尾でGENERIC_READへ遷移している
    CreateSrv(texture);

    return handle;
}

bool TextureManager::IsLoaded(const std::string& filepath) const {
    uint32_t handle = 0;
    return FindHandle(filepath, handle);
}

uint32_t TextureManager::GetWhiteTexture() {
    assert(device_ != nullptr && "TextureManager::Initializeが呼ばれていない");

    // 予約キー（実ファイルパスと衝突しないよう<>で囲む）でキャッシュ検索
    static const std::string kWhiteKey = "<white1x1>";
    uint32_t handle = 0;
    if (FindHandle(kWhiteKey, handle)) {
        return handle;
    }

    // 1x1の白イメージをプログラム生成する（1x1なのでミップマップ生成は不要）
    ScratchImage image;
    [[maybe_unused]] HRESULT hr = image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, 1, 1, 1, 1);
    assert(SUCCEEDED(hr));
    std::memset(image.GetPixels(), 0xFF, image.GetPixelsSize()); // RGBA = (255, 255, 255, 255)

    return Register(kWhiteKey, image);
}

uint32_t TextureManager::CreateDynamic(const std::string& key, uint32_t width, uint32_t height) {
    assert(device_ != nullptr && "TextureManager::Initializeが呼ばれていない");
    assert(width > 0 && height > 0);

    // 同じkeyで作ってあればそのハンドルを返す（サイズは変えない。変えたいときはResizeDynamic）
    uint32_t handle = 0;
    if (FindHandle(key, handle)) {
        assert(textures_[handle].isDynamic && "同じkeyが動的でないテクスチャに使われている");
        return handle;
    }

    // SRVスロット確保 → 本体・アップロードバッファ作成 → SRV作成 → キャッシュ登録
    Texture texture;
    texture.filepath = key;
    texture.isDynamic = true;
    texture.srvIndex = DescriptorHeapManager::GetInstance()->AllocateSrv();
    CreateDynamicResources(texture, width, height);
    CreateSrv(texture);

    // uploadPtrはMapしたリソースのメモリを指しているので、moveしても有効なまま
    textures_.push_back(std::move(texture));
    return uint32_t(textures_.size() - 1);
}

void TextureManager::UpdateDynamic(uint32_t textureHandle, const uint32_t* pixels) {
    assert(textureHandle < textures_.size());
    assert(pixels != nullptr);
    Texture& texture = textures_[textureHandle];
    assert(texture.isDynamic && "動的テクスチャではない");

    // CPU側へコピーするだけにして、GPUへの転送は次のBeginFrameで1回にまとめる
    std::memcpy(texture.cpuPixels.data(), pixels, texture.cpuPixels.size() * kBytesPerPixel);
    texture.dirty = true;
}

void TextureManager::ResizeDynamic(uint32_t textureHandle, uint32_t width, uint32_t height) {
    assert(textureHandle < textures_.size());
    assert(width > 0 && height > 0);
    Texture& texture = textures_[textureHandle];
    assert(texture.isDynamic && "動的テクスチャではない");

    // 同じサイズなら作り直さない
    if (texture.metadata.width == width && texture.metadata.height == height) {
        return;
    }

    // 古い本体・アップロードバッファを前フレームの転送・描画が参照している可能性を消してから作り直す
    // （古いリソースはCreateDynamicResources内のComPtr代入で解放される。ハンドル・SRVスロットは変わらない）
    DirectXCore::GetInstance()->WaitForGPU();
    CreateDynamicResources(texture, width, height);
    CreateSrv(texture);
}

void TextureManager::BeginFrame(ID3D12GraphicsCommandList* commandList) {
    assert(commandList != nullptr);

    // --- 差し替えで不要になったリソースの遅延解放（保持フレーム数を数え、0になったものを解放する）---
    for (size_t i = 0; i < pendingReleases_.size();) {
        PendingRelease& pending = pendingReleases_[i];
        if (pending.framesLeft > 0) {
            --pending.framesLeft;
        }
        if (pending.framesLeft == 0) {
            pendingReleases_[i] = std::move(pendingReleases_.back());
            pendingReleases_.pop_back();
        } else {
            ++i;
        }
    }

    // 書き込み先スロット（GPUが前フレームの転送で読んでいる可能性のあるスロットを避ける）
    const uint32_t slot = DirectXCore::GetInstance()->GetFrameIndex();

    for (Texture& texture : textures_) {
        if (!texture.isDynamic || !texture.dirty) {
            continue;
        }

        // --- 今フレームのスロットへ行ごとに詰める（行ピッチは256境界なので幅×4とは一致しない）---
        const uint32_t width = static_cast<uint32_t>(texture.metadata.width);
        const uint32_t height = static_cast<uint32_t>(texture.metadata.height);
        const size_t rowBytes = static_cast<size_t>(width) * kBytesPerPixel;
        const UINT64 slotOffset = static_cast<UINT64>(slot) * texture.uploadSlotSize;
        uint8_t* dst = texture.uploadPtr + static_cast<size_t>(slotOffset);
        const uint8_t* src = reinterpret_cast<const uint8_t*>(texture.cpuPixels.data());
        for (uint32_t y = 0; y < height; ++y) {
            std::memcpy(
                dst + static_cast<size_t>(y) * texture.footprint.Footprint.RowPitch,
                src + static_cast<size_t>(y) * rowBytes,
                rowBytes);
        }

        // --- 本体をコピー先の状態へ（作成直後はCOPY_DESTのままなのでバリア不要）---
        if (texture.state != D3D12_RESOURCE_STATE_COPY_DEST) {
            TransitionBarrier(
                commandList, texture.resource.Get(), texture.state, D3D12_RESOURCE_STATE_COPY_DEST);
            texture.state = D3D12_RESOURCE_STATE_COPY_DEST;
        }

        // --- アップロードバッファのスロット → 本体 ---
        D3D12_TEXTURE_COPY_LOCATION dstLocation{};
        dstLocation.pResource = texture.resource.Get();
        dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dstLocation.SubresourceIndex = 0;

        D3D12_TEXTURE_COPY_LOCATION srcLocation{};
        srcLocation.pResource = texture.upload.Get();
        srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        srcLocation.PlacedFootprint = texture.footprint;
        srcLocation.PlacedFootprint.Offset = slotOffset;

        commandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);

        // --- Loadしたテクスチャと同じ状態（GENERIC_READ）に揃えて描画で読めるようにする ---
        TransitionBarrier(
            commandList, texture.resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
        texture.state = D3D12_RESOURCE_STATE_GENERIC_READ;
        texture.dirty = false;
    }
}

uint32_t TextureManager::Register(const std::string& key, const ScratchImage& mipImages) {
    Texture texture;
    texture.filepath = key;
    texture.metadata = mipImages.GetMetadata();
    texture.resource = CreateTextureResource(device_, texture.metadata);
    texture.intermediate = UploadTextureData(texture.resource.Get(), mipImages, device_, commandList_);
    texture.state = D3D12_RESOURCE_STATE_GENERIC_READ;  // UploadTextureDataの末尾でGENERIC_READへ遷移している

    // SRVスロットを確保して作成する
    texture.srvIndex = DescriptorHeapManager::GetInstance()->AllocateSrv();
    CreateSrv(texture);

    textures_.push_back(std::move(texture));
    return uint32_t(textures_.size() - 1);
}

void TextureManager::DeferRelease(ComPtr<ID3D12Resource> resource) {
    if (!resource) {
        return;
    }
    // 今フレームの記録中コマンドリストが参照していても、実行完了（フレームインフライト分）を越えてから解放する
    PendingRelease pending;
    pending.resource = std::move(resource);
    pending.framesLeft = DirectXCore::kFramesInFlight + 1;
    pendingReleases_.push_back(std::move(pending));
}

bool TextureManager::FindHandle(const std::string& key, uint32_t& outHandle) const {
    for (uint32_t i = 0; i < textures_.size(); ++i) {
        if (textures_[i].filepath == key) {
            outHandle = i;
            return true;
        }
    }
    return false;
}

void TextureManager::CreateSrv(Texture& texture) {
    assert(texture.resource.Get() != nullptr);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = texture.metadata.format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = UINT(texture.metadata.mipLevels);

    // 確保済みのスロットへ作る（差し替え時も同じスロットに上書きするのでハンドルは変わらない）
    DescriptorHeapManager* heapManager = DescriptorHeapManager::GetInstance();
    device_->CreateShaderResourceView(
        texture.resource.Get(), &srvDesc, heapManager->GetSrvCPUHandle(texture.srvIndex));
    texture.srvHandleGPU = heapManager->GetSrvGPUHandle(texture.srvIndex);
}

void TextureManager::CreateDynamicResources(Texture& texture, uint32_t width, uint32_t height) {
    assert(width > 0 && height > 0);

    // --- メタデータ（RGBA8 sRGB・ミップマップなしの2Dテクスチャ）---
    texture.metadata = TexMetadata{};
    texture.metadata.width = width;
    texture.metadata.height = height;
    texture.metadata.depth = 1;
    texture.metadata.arraySize = 1;
    texture.metadata.mipLevels = 1;
    texture.metadata.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    texture.metadata.dimension = TEX_DIMENSION_TEXTURE2D;

    // --- 本体（DEFAULTヒープ）。最初のBeginFrameで転送するのでCOPY_DESTで作る ---
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = texture.metadata.format;
    desc.SampleDesc.Count = 1;

    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    ComPtr<ID3D12Resource> resource;
    [[maybe_unused]] HRESULT hr = device_->CreateCommittedResource(
        &heap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));

    // 古い本体は数フレーム保持してから解放する（呼び出し側でGPUの完了は待っているが、
    // 記録中のコマンドリストが参照している可能性を残さないため）
    DeferRelease(texture.resource);
    texture.resource = resource;
    texture.state = D3D12_RESOURCE_STATE_COPY_DEST;
    texture.intermediate.Reset();  // 動的テクスチャでは使わない

    // --- アップロードバッファ（行ピッチは256境界）。フレームインフライトぶんのスロットを確保する ---
    UINT numRows = 0;
    UINT64 rowSizeBytes = 0;
    UINT64 totalBytes = 0;
    device_->GetCopyableFootprints(&desc, 0, 1, 0, &texture.footprint, &numRows, &rowSizeBytes, &totalBytes);
    // スロット先頭はPLACED_FOOTPRINTのアラインメント（512）に合わせる
    texture.uploadSlotSize = (totalBytes + (D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1))
                             & ~static_cast<UINT64>(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);

    // 古いアップロードバッファはMapしたままでも解放してよい（本体と同じく数フレーム保持してから解放する）
    DeferRelease(texture.upload);
    texture.uploadPtr = nullptr;
    texture.upload = CreateBufferResource(
        device_, static_cast<size_t>(texture.uploadSlotSize) * DirectXCore::kFramesInFlight);
    hr = texture.upload->Map(0, nullptr, reinterpret_cast<void**>(&texture.uploadPtr));
    assert(SUCCEEDED(hr));

    // CPU側の内容は透明にリセットし、最初のBeginFrameで転送する（そこでGENERIC_READへ遷移する）
    texture.cpuPixels.assign(static_cast<size_t>(width) * height, 0);
    texture.dirty = true;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureHandle) const {
    assert(textureHandle < textures_.size());
    return textures_[textureHandle].srvHandleGPU;
}

uint32_t TextureManager::GetWidth(uint32_t textureHandle) const {
    assert(textureHandle < textures_.size());
    return static_cast<uint32_t>(textures_[textureHandle].metadata.width);
}

uint32_t TextureManager::GetHeight(uint32_t textureHandle) const {
    assert(textureHandle < textures_.size());
    return static_cast<uint32_t>(textures_[textureHandle].metadata.height);
}

void TextureManager::Finalize() {
    // Mapしたままのアップロードバッファも含めて、ComPtrの解放に任せる（呼び出し側でGPU完了待ち済み）
    pendingReleases_.clear();
    textures_.clear();
    device_ = nullptr;
    commandList_ = nullptr;
}

ScratchImage TextureManager::LoadTextureImage(const std::string& filepath) {
    std::wstring filePathW = ConvertString(filepath);

    ScratchImage image;
    HRESULT hr = LoadFromWICFile(
        filePathW.c_str(),
        WIC_FLAGS_FORCE_SRGB,
        nullptr,
        image);
    assert(SUCCEEDED(hr));

    ScratchImage mipImages;
    hr = GenerateMipMaps(
        image.GetImages(),
        image.GetImageCount(),
        image.GetMetadata(),
        TEX_FILTER_SRGB,
        0,
        mipImages);
    assert(SUCCEEDED(hr));

    return mipImages;
}

ComPtr<ID3D12Resource> TextureManager::CreateTextureResource(
    ID3D12Device* device,
    const DirectX::TexMetadata& metadata) {
    D3D12_RESOURCE_DESC desc{};
    desc.Width = UINT(metadata.width);
    desc.Height = UINT(metadata.height);
    desc.MipLevels = UINT16(metadata.mipLevels);
    desc.DepthOrArraySize = UINT16(metadata.arraySize);
    desc.Format = metadata.format;
    desc.SampleDesc.Count = 1;
    desc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);

    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_DEFAULT;

    ComPtr<ID3D12Resource> resource;

    [[maybe_unused]] HRESULT hr = device->CreateCommittedResource(
        &heap,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&resource));

    assert(SUCCEEDED(hr));
    return resource;
}

ComPtr<ID3D12Resource> TextureManager::UploadTextureData(
    ID3D12Resource* texture,
    const DirectX::ScratchImage& mipImages,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList) {
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;

    PrepareUpload(
        device,
        mipImages.GetImages(),
        mipImages.GetImageCount(),
        mipImages.GetMetadata(),
        subresources);

    uint64_t intermediateSize =
        GetRequiredIntermediateSize(texture, 0, UINT(subresources.size()));

    ComPtr<ID3D12Resource> intermediateResource =
        CreateBufferResource(device, intermediateSize);

    UpdateSubresources(
        commandList,
        texture,
        intermediateResource.Get(),
        0, 0,
        UINT(subresources.size()),
        subresources.data());

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = texture;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;

    commandList->ResourceBarrier(1, &barrier);

    return intermediateResource;
}

} // namespace Engine
