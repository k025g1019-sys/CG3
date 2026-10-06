#pragma once

#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

#include "Engine/Graphics/DescriptorHeapManager.h"
#include "Engine/Graphics/GpuResource.h"

namespace Engine {

/// <summary>
/// 型付きのStructuredBuffer（SRVで読むアップロードバッファ。HLSLのStructuredBufferに対応）。
/// C++の配列のように elementCount 個の T を持ち、シェーダーからはインデックスで読む
/// （Instancingのインスタンスごとの行列など）。
/// ConstantBufferと同じくフレームインフライト数ぶんのスロットを持ち、CPUが書くスロットと
/// GPUが読むスロットを分けることで、描画中フレームのデータを壊さない。
/// 毎フレーム GetData(frameIndex) の配列へ書き込み、GetSrvHandleGPU(frameIndex) をDescriptorTableへ設定する。
/// SRVはスロットごとにDescriptorHeapManagerから払い出し、Reset（破棄）時に返す。
/// </summary>
template <typename T>
class StructuredBuffer {
public:

    StructuredBuffer() = default;

    ~StructuredBuffer() { Reset(); }

    // SRVのスロットを所有するためコピーできない
    StructuredBuffer(const StructuredBuffer&) = delete;
    StructuredBuffer& operator=(const StructuredBuffer&) = delete;

    // elementCountには1スロットの要素数（Instancingならインスタンスの最大数）、
    // slotCountには通常DirectXCore::kFramesInFlightを渡す
    void Create(ID3D12Device* device, uint32_t elementCount, uint32_t slotCount) {
        Reset();

        elementCount_ = elementCount;
        slotCount_ = slotCount;

        // StructuredBufferのAlignmentルールはC++の構造体と同様なので、sizeof(T)のまま並べてよい
        resource_ = CreateBufferResource(device, sizeof(T) * elementCount * slotCount);
        resource_->Map(0, nullptr, reinterpret_cast<void**>(&mapped_));

        // スロットごとにSRVを作る（同じリソースのうち、そのスロットの elementCount 個だけを見せる）
        DescriptorHeapManager* heapManager = DescriptorHeapManager::GetInstance();
        for (uint32_t slot = 0; slot < slotCount; ++slot) {
            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_UNKNOWN;  // 構造体のレイアウトは自由に設定できるため不明
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;  // TextureではなくBufferとして使う
            srvDesc.Buffer.FirstElement = UINT64(slot) * elementCount;  // このスロットの先頭の要素
            srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
            srvDesc.Buffer.NumElements = elementCount;  // シェーダーからアクセスする要素数
            srvDesc.Buffer.StructureByteStride = UINT(sizeof(T));  // 1要素（構造体）のサイズ

            const uint32_t srvIndex = heapManager->AllocateSrv();
            device->CreateShaderResourceView(resource_.Get(), &srvDesc, heapManager->GetSrvCPUHandle(srvIndex));
            srvIndices_.push_back(srvIndex);
        }
    }

    // 指定スロットの書き込み先（elementCount個の配列の先頭）
    T* GetData(uint32_t slot) {
        assert(mapped_ != nullptr);
        assert(slot < slotCount_);
        return mapped_ + size_t(slot) * elementCount_;
    }

    // 指定スロットのSRVのGPUハンドル（SetGraphicsRootDescriptorTableに渡す）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t slot) const {
        assert(slot < srvIndices_.size());
        return DescriptorHeapManager::GetInstance()->GetSrvGPUHandle(srvIndices_[slot]);
    }

    // 1スロットの要素数（Createで指定した数）
    uint32_t GetElementCount() const { return elementCount_; }

    // リソースを解放し、SRVのスロットを返す（GPUが使い終わってから呼ぶこと）
    void Reset() {
        for (uint32_t srvIndex : srvIndices_) {
            DescriptorHeapManager::GetInstance()->FreeSrv(srvIndex);
        }
        srvIndices_.clear();
        resource_.Reset();
        mapped_ = nullptr;
        elementCount_ = 0;
        slotCount_ = 0;
    }

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> resource_;

    T* mapped_ = nullptr;  // Map済みの書き込み先（resource_生存中のみ有効）

    uint32_t elementCount_ = 0;  // 1スロットの要素数

    uint32_t slotCount_ = 0;

    std::vector<uint32_t> srvIndices_;  // スロットごとのSRVのindex（DescriptorHeapManagerから払い出したもの）
};

} // namespace Engine
