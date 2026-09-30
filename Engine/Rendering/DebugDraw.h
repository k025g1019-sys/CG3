#pragma once

#include <cstdint>
#include <d3d12.h>
#include <vector>
#include <wrl.h>

#include "Engine/Math/Shapes.h"
#include "Engine/Math/Vector3.h"
#include "Engine/Math/Vector4.h"

namespace Engine {

class Curve;

/// <summary>
/// デバッグ用の線描画（当たり判定の形・曲線・補助線など）。
/// シーンの OnUpdate の中で DrawLine / DrawSphere などを呼ぶと、そのフレームのシーン描画の最後に、
/// 他の物体に隠れずに（深度テストなしで）表示される。線はフレームごとに消えるので、表示したい間は毎フレーム呼ぶ。
/// 開発中だけ表示したいときは #ifndef NDEBUG で囲む。
/// </summary>
class DebugDraw {
public:

    // 線分（始点・終点）
    static void DrawLine(const Vector3& start, const Vector3& end, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 線分（Segment3D）
    static void DrawSegment(const Segment3D& segment, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 球（緯度・経度の線で描く）
    static void DrawSphere(const Sphere& sphere, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // AABB（12本の辺）
    static void DrawAABB(const AABB3D& aabb, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // OBB（12本の辺）
    static void DrawOBB(const OBB3D& obb, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 三角形（3本の辺）
    static void DrawTriangle(const Triangle3D& triangle, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // カプセル（両端の球と、それをつなぐ側面の線）
    static void DrawCapsule(const Capsule3D& capsule, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f });

    // 平面（原点に最も近い点を中心に、size 四方の枠と法線を描く）
    static void DrawPlane(const Plane3D& plane, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, float size = 2.0f);

    // 曲線（折れ線で近似する）。drawControlPoints が true なら制御点と、制御点を結ぶ線も描く
    static void DrawCurve(
        const Curve& curve, const Vector4& color = { 1.0f, 1.0f, 1.0f, 1.0f }, bool drawControlPoints = true);

    // XZ平面のグリッド（原点中心。size 四方を divisions 分割する）
    static void DrawGrid(
        float size = 10.0f, uint32_t divisions = 10, const Vector4& color = { 0.5f, 0.5f, 0.5f, 1.0f });

    // --- 以下はエンジン内部から呼ばれる ---

    static DebugDraw* GetInstance();

    // 線描画用のシェーダー・PSO・頂点バッファを作る（ShaderCompiler・PipelineManagerの初期化後に呼ぶ）
    void Initialize(ID3D12Device* device);

    // GPUリソースを解放する（リソースリークチェックより前に呼ぶ）
    void Finalize();

    // 積んだ線を描く（BaseScene::Drawの最後に呼ばれる。立体視では視点数ぶん呼ばれ、転送は最初の1回だけ）
    void Render(ID3D12GraphicsCommandList* commandList, D3D12_GPU_VIRTUAL_ADDRESS viewProjection);

    // 積んだ線を消す（Frameworkがフレームの最後に呼ぶ）
    void EndFrame();

private:

    DebugDraw() = default;
    ~DebugDraw() = default;

    DebugDraw(const DebugDraw&) = delete;
    DebugDraw& operator=(const DebugDraw&) = delete;

    // 線1本ぶんの頂点を積む
    void AddLine(const Vector3& start, const Vector3& end, const Vector4& color);

private:

    // 線の頂点（Line.VS.hlsl の入力と同じ並び）
    struct LineVertex {
        Vector3 position;
        Vector4 color;
    };

    // 1フレームに描ける線の最大数（超えた分は描かれない）
    static constexpr uint32_t kMaxLineCount = 65536;
    static constexpr uint32_t kMaxVertexCount = kMaxLineCount * 2;

    // このフレームに積まれた線の頂点（2頂点で1本）
    std::vector<LineVertex> vertices_;

    // 頂点バッファ（フレームインフライト数ぶんの領域を持ち、GPUが読んでいる領域には書かない）
    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    LineVertex* mappedVertices_ = nullptr;

    // このフレームの頂点をGPUへ転送済みか（転送は最初のRenderの1回だけ）と、その頂点数
    bool uploaded_ = false;
    uint32_t uploadedVertexCount_ = 0;

    // 上限を超えたことを一度だけログに出すためのフラグ
    bool overflowLogged_ = false;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
};

} // namespace Engine
