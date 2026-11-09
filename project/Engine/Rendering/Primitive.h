#pragma once

namespace Engine {

/// <summary>
/// 組み込みの形状（仮モデル用）。どれも原点中心で、1x1x1の箱にちょうど収まる大きさ。
/// Transformのscaleがそのままワールドでの大きさになる（例: scale {1, 2, 1} で縦長の箱）。
/// 使い方: object.Initialize(Engine::Primitive::kCube);
/// </summary>
enum class Primitive {
    kCube,    // 立方体（1辺1）
    kSphere,  // 球（直径1）
    kPlane,   // 平面（XZ平面上の1x1。上(+Y)向き。地面などに使う）

    kCount,   // 形状の数（enumの末尾に置くこと）
};

} // namespace Engine
