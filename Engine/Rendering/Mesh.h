#pragma once

#include <cstdint>
#include <d3d12.h>
#include <string>
#include <vector>
#include <wrl.h>

#include "Engine/Math/Vector3.h"
#include "Engine/Rendering/VertexData.h"

namespace Engine {

/// <summary>
/// 頂点（＋任意でインデックス）バッファと、カリング／ピッキング用の
/// ローカル空間バウンディング球を持つメッシュ。
/// 頂点バッファはMapしたままにするため、GetMappedVerticesで直接編集できる。
/// OBJから生成した場合はo/g/usemtl単位のサブメッシュ範囲も保持する。
/// </summary>
class Mesh {
public:

    /// <summary>
    /// OBJのo/g/usemtl単位の描画範囲。テクスチャの読み込み・バインドはObject3D側で行う
    /// （Meshはmtlから解決したパス文字列を保持するだけで、TextureManagerには依存しない）。
    /// </summary>
    struct SubMesh {
        std::string name;             // o/gの名前（無ければ空）
        std::string materialName;     // usemtlのマテリアル名（無ければ空）
        std::string textureFilePath;  // mtlのmap_Kdから解決したパス（無ければ空）
        uint32_t vertexStart = 0;     // 頂点バッファ内の開始位置
        uint32_t vertexCount = 0;     // 頂点数
    };

    // 頂点配列から生成する（verticesがnullptrなら領域確保のみ）
    void Create(ID3D12Device* device, const VertexData* vertices, uint32_t vertexCount);

    // 頂点＋インデックス配列から生成する
    void Create(
        ID3D12Device* device,
        const VertexData* vertices, uint32_t vertexCount,
        const uint32_t* indices, uint32_t indexCount);

    // OBJファイルから生成する
    void CreateFromObj(ID3D12Device* device, const std::string& directoryPath, const std::string& filename);

    // 緯度経度分割の球（半径1・原点中心）を生成する
    void CreateSphere(ID3D12Device* device, uint32_t subdivision);

    // 1辺1・原点中心の立方体を生成する（面ごとに法線・UV 0..1を持つ24頂点＋36インデックス）
    void CreateCube(ID3D12Device* device);

    // 頂点（インデックスがあればインデックス）バッファを設定して描画コマンドを積む。
    // instanceCountを指定すると、同じメッシュをその数だけ1回の描画命令でまとめて描く
    // （Instancing。VSのSV_InstanceIDに 0～instanceCount-1 が入る）
    void Draw(ID3D12GraphicsCommandList* commandList, uint32_t instanceCount = 1) const;

    // 指定サブメッシュの頂点範囲だけ描画する（OBJ由来の非インデックスメッシュ専用）
    void DrawSubMesh(ID3D12GraphicsCommandList* commandList, uint32_t index) const;

    // サブメッシュ数（OBJ以外から生成したメッシュは0）
    uint32_t GetSubMeshCount() const { return uint32_t(subMeshes_.size()); }

    // 指定サブメッシュの情報を取得する
    const SubMesh& GetSubMesh(uint32_t index) const { return subMeshes_[index]; }

    // Map済み頂点への書き込みアクセス（ImGuiでの頂点編集用）
    VertexData* GetMappedVertices() { return mappedVertices_; }

    uint32_t GetVertexCount() const { return vertexCount_; }

    // ローカル空間のバウンディング球（Create時に頂点から算出）
    const Vector3& GetLocalCenter() const { return localCenter_; }
    float GetLocalRadius() const { return localRadius_; }

    // 頂点編集後にバウンディング球を再計算する
    void RecomputeBoundingSphere();

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vbv_{};
    VertexData* mappedVertices_ = nullptr;
    uint32_t vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    D3D12_INDEX_BUFFER_VIEW ibv_{};
    uint32_t indexCount_ = 0;

    std::vector<SubMesh> subMeshes_;  // OBJ以外から生成した場合は空

    Vector3 localCenter_{ 0.0f, 0.0f, 0.0f };
    float localRadius_ = 0.0f;
};

} // namespace Engine
