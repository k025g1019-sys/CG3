#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Primitive.h"

namespace Engine {

/// <summary>
/// メッシュの読み込み・キャッシュを行うシングルトン。
/// 同じOBJファイルを何度Loadしても読み込みは1回だけ行われ、同じメッシュが共有される。
/// 組み込みの形状（Primitive）も初回に生成して使い回す。
/// メッシュはアプリ終了（Finalize）まで保持される。
/// </summary>
class MeshManager {
public:

    static MeshManager* GetInstance();

    /// <summary>
    /// OBJファイルからメッシュを読み込む（2回目以降はキャッシュを返す）
    /// </summary>
    /// <param name="objFilePath">実行ディレクトリからの相対パス（例: "resources/player.obj"）</param>
    Mesh* Load(const std::string& objFilePath);

    // 組み込みの形状のメッシュを返す（初回に生成する）
    Mesh* GetPrimitive(Primitive primitive);

    // 全メッシュを解放する（リソースリークチェックより前に呼ぶ）
    void Finalize();

private:

    MeshManager() = default;
    ~MeshManager() = default;

    MeshManager(const MeshManager&) = delete;
    MeshManager& operator=(const MeshManager&) = delete;

private:

    // 球の緯度・経度の分割数
    static constexpr uint32_t kSphereSubdivision = 16;

    // 読み込み済みのOBJメッシュ（キー: 読み込み時のパス）
    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes_;

    // 組み込みの形状（未生成ならnullptr）
    std::unique_ptr<Mesh> primitives_[size_t(Primitive::kCount)];
};

} // namespace Engine
