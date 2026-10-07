#pragma once

#include <d3d12.h>
#include <cstdint>
#include <string>
#include <vector>
#include <wrl.h>
#include "externals/DirectXTex/DirectXTex.h"

namespace Engine {

/// <summary>
/// テクスチャの読み込み・GPU転送・SRV作成・キャッシュを行うシングルトン。
/// Loadが返すハンドルをGetSrvHandleGPUに渡して描画に使う。
/// 同じパスを複数回Loadしても読み込みは1回だけ行われ、同じハンドルが返る。
/// ハンドルはSRVヒープのスロットに対応し、Reload・ResizeDynamicで中身を差し替えてもハンドルは変わらない。
///
/// 動的テクスチャ（CreateDynamic）はCPUから何度でも書き換えられるRGBA8（sRGB）テクスチャで、
/// UpdateDynamicで渡した内容が次のBeginFrameでGPUへ転送される（スプライトエディタのキャンバス等に使う）。
/// </summary>
class TextureManager {
public:

    static TextureManager* GetInstance();

    // デバイスと、転送コマンドを積むコマンドリストを登録する
    // （DescriptorHeapManager::Initializeの後に呼ぶ）
    void Initialize(ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

    /// <summary>
    /// テクスチャを読み込み、GPU転送コマンド発行とSRV作成まで行ってハンドルを返す。
    /// 転送コマンドはコマンドリストに積まれ、次のExecuteCommandListsで実行される。
    /// </summary>
    /// <param name="filepath">実行ディレクトリからの相対パス</param>
    /// <returns>GetSrvHandleGPUに渡すテクスチャハンドル</returns>
    uint32_t Load(const std::string& filepath);

    /// <summary>
    /// ファイルを読み直して、同じハンドルのテクスチャの中身を差し替える（保存し直した画像の反映に使う）。
    /// まだ読み込んでいないパスならLoadと同じ。差し替えの前にGPUの完了を待つので、頻繁に呼ばないこと。
    /// </summary>
    /// <returns>そのパスのテクスチャハンドル（読み込み済みなら従来と同じ値）</returns>
    uint32_t Reload(const std::string& filepath);

    // そのパスが読み込み済みか（Loadのキャッシュにあるか）
    bool IsLoaded(const std::string& filepath) const;

    /// <summary>
    /// 1x1の白テクスチャのハンドルを返す（初回呼び出し時に生成してキャッシュする）。
    /// map_Kdを持たないマテリアル（テクスチャなしモデル）のフォールバックに使う。
    /// </summary>
    uint32_t GetWhiteTexture();

    // --- 動的テクスチャ（CPUから書き換えられるテクスチャ） ---

    /// <summary>
    /// 動的テクスチャを作ってハンドルを返す。フォーマットはR8G8B8A8（sRGB）・ミップマップなしで、中身は透明（0）。
    /// 同じkeyで既に作ってあればそのハンドルを返す。
    /// </summary>
    /// <param name="key">他のテクスチャと重複しない識別名（実ファイルパスと衝突しないよう "<name>" のように書く）</param>
    uint32_t CreateDynamic(const std::string& key, uint32_t width, uint32_t height);

    /// <summary>
    /// 動的テクスチャの中身を差し替える。内容はCPU側へコピーされ、次のBeginFrameでGPUへ転送される
    /// （1フレームに何度呼んでも転送は1回）。
    /// </summary>
    /// <param name="pixels">幅×高さ個のRGBA8（メモリ順 R,G,B,A。uint32_tとしては0xAABBGGRR）</param>
    void UpdateDynamic(uint32_t textureHandle, const uint32_t* pixels);

    /// <summary>
    /// 動的テクスチャのサイズを変える（中身は透明にリセットされる。ハンドルは変わらない）。
    /// 作り直しの前にGPUの完了を待つので、頻繁に呼ばないこと。
    /// </summary>
    void ResizeDynamic(uint32_t textureHandle, uint32_t width, uint32_t height);

    /// <summary>
    /// 毎フレーム1回、描画コマンドを積む前に呼ぶ（Frameworkが呼ぶ）。
    /// UpdateDynamicで変更された動的テクスチャの転送コマンドを積む。
    /// </summary>
    void BeginFrame(ID3D12GraphicsCommandList* commandList);

    // 描画に使うSRVのGPUハンドルを取得する
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureHandle) const;

    // テクスチャのサイズ（ピクセル）
    uint32_t GetWidth(uint32_t textureHandle) const;
    uint32_t GetHeight(uint32_t textureHandle) const;

    // 全テクスチャを解放する（リソースリークチェックより前に呼ぶ）
    void Finalize();

private:

    TextureManager() = default;

    ~TextureManager() = default;

    TextureManager(const TextureManager&) = delete;

    TextureManager& operator=(const TextureManager&) = delete;

private:

    // 読み込み済みテクスチャ1枚分のデータ
    struct Texture {
        std::string filepath;  // 読み込み元のパス（動的テクスチャ・白テクスチャは識別用のkey）
        DirectX::TexMetadata metadata{};
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        // 転送コマンドが実行されるまでGPUが参照する中間リソース（Finalizeまで保持）
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediate;
        uint32_t srvIndex = 0;  // SRVヒープのスロット（差し替え時は同じスロットへSRVを作り直す）
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU{};

        // --- 動的テクスチャ用（isDynamicのときだけ使う） ---
        bool isDynamic = false;
        std::vector<uint32_t> cpuPixels;  // 最新の内容（UpdateDynamicでコピー）
        bool dirty = false;               // cpuPixelsがまだGPUへ転送されていない
        // アップロードバッファ（常時Map）。GPUが前フレームの転送を実行中でも書き込めるよう、
        // DirectXCore::kFramesInFlightぶんのスロットを持つ
        Microsoft::WRL::ComPtr<ID3D12Resource> upload;
        uint8_t* uploadPtr = nullptr;
        uint64_t uploadSlotSize = 0;                       // 1スロットのサイズ（512境界）
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};   // CopyTextureRegion用（RowPitchは256境界）
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COPY_DEST;  // 現在のリソース状態
    };

    // 読み込み済みイメージをGPUリソース化しSRVを作ってキャッシュへ登録する（Loadと共通の後半処理）
    uint32_t Register(const std::string& key, const DirectX::ScratchImage& mipImages);

    // ファイルパス（key）からハンドルを探す（見つからなければfalse）
    bool FindHandle(const std::string& key, uint32_t& outHandle) const;

    // texture.srvIndexのスロットへ、現在のresource・metadataでSRVを作る（作り直しにも使う）
    void CreateSrv(Texture& texture);

    // 動的テクスチャ本体・アップロードバッファ・メタデータを指定サイズで（作り）直す（SRVは別途CreateSrv）
    void CreateDynamicResources(Texture& texture, uint32_t width, uint32_t height);

    // 差し替えで不要になったリソースを、GPUが参照し終わるまで（数フレーム）保持してから解放する。
    // 記録中のコマンドリストが古いリソースを参照していても（同じフレームでLoadとReloadが起きた場合など）壊れないようにする
    void DeferRelease(Microsoft::WRL::ComPtr<ID3D12Resource> resource);

    // 解放待ちのリソース
    struct PendingRelease {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        uint32_t framesLeft = 0;  // 残りフレーム数（BeginFrameで減らし、0になったら解放する）
    };

    // ファイルからミップマップ付きイメージを読み込む
    static DirectX::ScratchImage LoadTextureImage(const std::string& filepath);

    // メタデータからGPU上のテクスチャリソースを作成する
    static Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(
        ID3D12Device* device,
        const DirectX::TexMetadata& metadata);

    // イメージをGPUへ転送するコマンドを積む（戻り値は転送用の中間リソース）
    static Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(
        ID3D12Resource* texture,
        const DirectX::ScratchImage& mipImages,
        ID3D12Device* device,
        ID3D12GraphicsCommandList* commandList);

private:

    ID3D12Device* device_ = nullptr;                       // 非所有

    ID3D12GraphicsCommandList* commandList_ = nullptr;     // 非所有

    std::vector<Texture> textures_;  // ハンドル＝このvectorのindex

    std::vector<PendingRelease> pendingReleases_;  // 差し替えで不要になり、解放を待っているリソース
};

} // namespace Engine
