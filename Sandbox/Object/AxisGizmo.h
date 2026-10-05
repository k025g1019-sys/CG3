#pragma once

#include <cstdint>
#include <d3d12.h>

#include "Engine/Rendering/ConstantBuffer.h"
#include "Engine/Rendering/Material.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/Object3D.h"
#include "Engine/Rendering/TransformationMatrix.h"

/// <summary>
/// 選択中オブジェクトのローカル回転軸を示すギズモ（X=赤 / Y=緑 / Z=青の3本線）。
/// ・対象オブジェクトの回転・位置に追従し、長さはワールドバウンディング球の大きさに連動する
/// ・深度無効のラインPSO（PipelineManagerのkLine）で描くため、他オブジェクトに隠れず常に見える
/// ・ライティング無効（軸色をそのまま表示）
/// </summary>
class AxisGizmo {
public:
    // 3本線のメッシュ・マテリアル等を初期化する（PipelineManager・TextureManagerの初期化後に呼ぶ）
    void Initialize();

    /// <summary>
    /// 対象オブジェクトのTransformからワールド行列を計算し、定数バッファへ書き込む。
    /// 毎フレーム1回呼ぶ（立体視でDrawが視点ごとに複数回呼ばれても書き込みはここだけで行う）。
    /// </summary>
    /// <param name="target">選択中のオブジェクト（nullptrなら非表示になる）</param>
    void Update(const Engine::Object3D* target);

    /// <summary>
    /// ラインPSOに切り替えて3本の軸線を描画し、標準PSOへ戻す（シーン内オブジェクトの最後に呼ぶ）。
    /// RootSignature・DescriptorHeap・光源・ビュー射影のCBVはシーン（BaseScene）で設定済みの前提。
    /// </summary>
    void Draw();

#ifdef USE_IMGUI
    // 表示ON/OFFのチェックボックス
    void DrawImGui();
#endif

private:
    static constexpr int kAxisCount = 3;  // X/Y/Zの3軸

    // --- 軸線メッシュ（各2頂点。原点から各軸の+方向へ長さ1の線分）---
    Engine::Mesh axisMeshes_[kAxisCount];

    // --- マテリアル（X=赤/Y=緑/Z=青、ライティング無効）---
    Engine::Material materials_[kAxisCount]{};
    Engine::ConstantBuffer<Engine::Material> materialCBs_[kAxisCount];

    // --- Transform（3本共通。対象の回転・位置＋長さのスケール）---
    Engine::ConstantBuffer<Engine::TransformationMatrix> transformCB_;

    // --- 共通ルートシグネチャが要求するテクスチャ（1x1白。マテリアル色がそのまま出る）---
    uint32_t whiteTextureHandle_ = 0;

    bool show_ = true;        // ImGuiでの表示切り替え
    bool hasTarget_ = false;  // このフレームに描画対象があるか（Updateで決まる）
};
