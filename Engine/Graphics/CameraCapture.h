#pragma once

#include <cstdint>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include <d3d12.h>
#include <wrl.h>

namespace Engine {

class FaceTracker;  // フレームを渡して顔検出させる任意の連携先（前方宣言でwinrt依存を持ち込まない）

// ノートPC内蔵カメラ（Webカメラ）の映像をMedia Foundationで取得し、DX12テクスチャへ転送して
// SRV（GetSrvGpuHandle）として公開する（ImGui::Image等でそのまま表示できる）。
// フレーム取得はワーカースレッドで行い、描画スレッドは最新フレームのみを
// アップロードするため、カメラFPSと描画FPSが分離される。
//
// カメラが無い／開けない場合はIsAvailable()==falseのまま安全に無効化される（クラッシュしない）。
// 使わない間はStopCaptureでワーカーを止められる（非使用時のコストをゼロにする）。
class CameraCapture {
public:
    // GPUリソース生成に使うデバイスを登録し、SRVスロットを予約する。
    // この時点ではカメラを開かない（プライバシー配慮＋視線プロセスとの競合回避）。
    // 実際のオープンはStartCapture()または最初のUpdateTexture()呼び出しで行う。
    void Initialize(ID3D12Device* device);

    // ワーカースレッドを停止し、Media Foundationとリソースを解放する。
    void Finalize();

    // 表示の有無に関わらずカメラ取得ワーカーを起動する（視線追跡だけ使う＝映像非表示でもフレームを流す）。
    // 起動済みなら何もしない。UpdateTexture()でも内部的に呼ばれる。
    void StartCapture();

    // カメラ取得ワーカーを停止する（未起動なら何もしない）。
    // 表示にも視線追跡にも使わない間は止めておくことで、カメラとCPUを占有しない。
    // 生成済みのGPUテクスチャは保持され、再度StartCaptureすれば続きから使える。
    void StopCapture();

    // 取得した各カメラフレーム（BGRA）を顔検出へ渡す連携先を設定する（nullptrで無効）。
    // StartCapture()/UpdateTexture()でワーカーを起動する前に設定すること。
    void SetFaceTracker(FaceTracker* tracker) { faceTracker_ = tracker; }

    // 最新カメラフレームがあればGPUテクスチャへ反映する（描画コマンドを積む前に毎フレーム呼ぶ）。
    void UpdateTexture(ID3D12GraphicsCommandList* commandList);

    // テクスチャが利用可能か（カメラが開けて解像度が確定済みか）。
    bool IsAvailable() const { return available_.load(); }

    // ImGui::Image等でそのまま表示に使えるSRVのGPUハンドル（テクスチャ未生成の間は0）。
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuHandle() const { return srvGpu_; }

    // 生成済みテクスチャの解像度（未生成の間は0）。表示時のアスペクト計算に使う。
    int GetTextureWidth() const { return textureWidth_; }
    int GetTextureHeight() const { return textureHeight_; }

private:
    // 解像度確定後にDX12テクスチャ／SRV／アップロードバッファを生成する（描画スレッドで一度だけ）。
    void CreateTextureIfNeeded();
    // Media Foundationでカメラを開き、フレームを取得し続ける（ワーカースレッド本体）。
    void WorkerThread();
    // テクスチャを指定状態へ遷移する（不要なら何もしない）。
    void Transition(ID3D12GraphicsCommandList* commandList, D3D12_RESOURCE_STATES after);

private:
    ID3D12Device* device_ = nullptr;
    uint32_t srvIndex_ = 0;  // DescriptorHeapManagerから予約したSRVスロット

    // 顔検出の連携先（任意）。ワーカースレッドが各フレームを渡す。所有しない。
    FaceTracker* faceTracker_ = nullptr;

    // --- ワーカースレッド ↔ 描画スレッド 共有 ---
    std::thread worker_;
    bool workerStarted_ = false;            // カメラ取得スレッドを起動済みか（遅延起動）
    std::atomic<bool> stop_{ false };       // 終了要求
    std::atomic<bool> available_{ false };  // 解像度確定＆テクスチャ生成済み
    std::atomic<bool> dimsReady_{ false };  // 解像度が確定した（テクスチャ生成のトリガ）
    std::atomic<bool> newFrame_{ false };   // 未反映の新フレームがある

    std::mutex frameMutex_;
    std::vector<uint8_t> cpuFrame_;  // 最新フレーム（top-down・BGRA・隙間なし）
    int frameWidth_ = 0;
    int frameHeight_ = 0;

    // --- GPUリソース（描画スレッドが生成・所有）---
    Microsoft::WRL::ComPtr<ID3D12Resource> texture_;  // DEFAULT, B8G8R8A8_UNORM
    // アップロードバッファ（常時Map）。フレームインフライトぶんのスロットを持ち、
    // GPUが前フレームの転送を実行中でもCPUが安全に書き込めるようにする。
    Microsoft::WRL::ComPtr<ID3D12Resource> upload_;
    uint8_t* uploadPtr_ = nullptr;
    uint64_t uploadSlotSize_ = 0;  // 1スロットのサイズ（512境界）
    uint64_t uploadRowPitch_ = 0;  // GetCopyableFootprintsの行ピッチ（256境界）
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint_{};  // CopyTextureRegion用（Offsetはスロットごとに設定）
    D3D12_RESOURCE_STATES textureState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    D3D12_GPU_DESCRIPTOR_HANDLE srvGpu_{};
    bool textureCreated_ = false;
    bool needsCopy_ = false;  // uploadの内容をテクスチャへ転送する必要がある
    int textureWidth_ = 0;    // 生成済みテクスチャの解像度（未生成の間は0）
    int textureHeight_ = 0;
};

} // namespace Engine
