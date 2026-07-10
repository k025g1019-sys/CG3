#pragma once
#include "Engine/Math/Vector3.h"

namespace Engine {

/// <summary>
/// 4x4行列（行優先・行ベクトル規約）。
/// 変換は v * M の順で適用し、行列の合成は左から右へ掛ける（例：world = S * R * T、WVP = world * view * proj）。
/// 平行移動成分は第3行（m[3][0..2]）に格納される。
/// </summary>
struct Matrix4x4 {
	float m[4][4];

	// 行列同士の加減乗算（実装は既存のAdd/Subtract/Multiplyを利用）
	Matrix4x4 operator+(const Matrix4x4& rhs) const;
	Matrix4x4 operator-(const Matrix4x4& rhs) const;
	Matrix4x4 operator*(const Matrix4x4& rhs) const;
	Matrix4x4& operator*=(const Matrix4x4& rhs);
};

#pragma region

/// <summary>
/// 行列の加法
/// </summary>
/// <param name="m1">左辺の行列</param>
/// <param name="m2">右辺の行列</param>
/// <returns>m1 + m2 の成分ごとの和</returns>
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);

/// <summary>
/// 行列の減法
/// </summary>
/// <param name="m1">左辺の行列</param>
/// <param name="m2">右辺の行列</param>
/// <returns>m1 - m2 の成分ごとの差</returns>
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);

/// <summary>
/// 行列の積
/// </summary>
/// <param name="m1">左辺の行列（先に適用される変換）</param>
/// <param name="m2">右辺の行列（後に適用される変換）</param>
/// <returns>m1 * m2</returns>
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

/// <summary>
/// 逆行列（ガウス・ジョルダン法）
/// </summary>
/// <param name="m">元の行列</param>
/// <returns>m の逆行列。特異行列（逆行列が存在しない）の場合は単位行列</returns>
Matrix4x4 Inverse(const Matrix4x4& m);

/// <summary>
/// 転置行列
/// </summary>
/// <param name="m">元の行列</param>
/// <returns>行と列を入れ替えた行列</returns>
Matrix4x4 Transpose(const Matrix4x4& m);

/// <summary>
/// 単位行列の作成
/// </summary>
/// <returns>4x4の単位行列</returns>
Matrix4x4 MakeIdentity4x4();

#pragma endregion

#pragma region

/// <summary>
/// 平行移動行列の作成
/// </summary>
/// <param name="translate">各軸の移動量</param>
/// <returns>第3行に移動量を持つ平行移動行列</returns>
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

/// <summary>
/// 拡大縮小行列の作成
/// </summary>
/// <param name="scale">各軸の倍率</param>
/// <returns>対角成分に倍率を持つ拡大縮小行列</returns>
Matrix4x4 MakeScaleMatrix(const Vector3& scale);

/// <summary>
/// 点の座標変換。w=1の点として変換し、透視除算まで行う
/// </summary>
/// <param name="vector">変換する点</param>
/// <param name="matrix">変換行列</param>
/// <returns>変換後の点（wで除算済み。wが0の場合はassertで停止）</returns>
Vector3 Transform(const Vector3& vector, const Matrix4x4& matrix);

/// <summary>
/// X軸回転行列の作成
/// </summary>
/// <param name="radian">回転角（ラジアン）</param>
/// <returns>X軸まわりの回転行列</returns>
Matrix4x4 MakeRotateXMatrix(float radian);

/// <summary>
/// Y軸回転行列の作成
/// </summary>
/// <param name="radian">回転角（ラジアン）</param>
/// <returns>Y軸まわりの回転行列</returns>
Matrix4x4 MakeRotateYMatrix(float radian);

/// <summary>
/// Z軸回転行列の作成
/// </summary>
/// <param name="radian">回転角（ラジアン）</param>
/// <returns>Z軸まわりの回転行列</returns>
Matrix4x4 MakeRotateZMatrix(float radian);

/// <summary>
/// 3次元アフィン変換行列の作成。S * R * T の順で合成する
/// </summary>
/// <param name="scale">各軸の倍率</param>
/// <param name="rotate">各軸の回転角（ラジアン）。X → Y → Z の順で合成</param>
/// <param name="translate">各軸の移動量</param>
/// <returns>拡縮・回転・平行移動を合成したアフィン変換行列</returns>
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

#pragma endregion

#pragma region

/// <summary>
/// 透視投影行列の作成（左手系、深度は0..1のDirectX規約）
/// </summary>
/// <param name="fovY">垂直方向の視野角（ラジアン）</param>
/// <param name="aspectRatio">アスペクト比（幅 / 高さ）</param>
/// <param name="nearClip">近クリップ面までの距離</param>
/// <param name="farClip">遠クリップ面までの距離</param>
/// <returns>透視投影行列</returns>
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

/// <summary>
/// 正射影行列の作成（深度は0..1のDirectX規約）
/// </summary>
/// <param name="left">視界の左端</param>
/// <param name="top">視界の上端</param>
/// <param name="right">視界の右端</param>
/// <param name="bottom">視界の下端</param>
/// <param name="nearClip">近クリップ面までの距離</param>
/// <param name="farClip">遠クリップ面までの距離</param>
/// <returns>正射影行列</returns>
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

/// <summary>
/// ビューポート行列の作成。NDCをスクリーン座標へ変換する（Y軸は下向きに反転）
/// </summary>
/// <param name="left">ビューポート左上のX座標</param>
/// <param name="top">ビューポート左上のY座標</param>
/// <param name="width">ビューポートの幅</param>
/// <param name="height">ビューポートの高さ</param>
/// <param name="minDepth">深度の最小値</param>
/// <param name="maxDepth">深度の最大値</param>
/// <returns>ビューポート行列</returns>
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

/// <summary>
/// クロス積（外積）
/// </summary>
/// <param name="v1">左辺のベクトル</param>
/// <param name="v2">右辺のベクトル</param>
/// <returns>v1とv2の両方に垂直なベクトル</returns>
Vector3 Cross(const Vector3& v1, const Vector3& v2);

#pragma endregion

#pragma region

/// <summary>
/// 内積
/// </summary>
/// <param name="v1">左辺のベクトル</param>
/// <param name="v2">右辺のベクトル</param>
/// <returns>v1とv2の内積</returns>
float Dot(const Vector3& v1, const Vector3& v2);

/// <summary>
/// ベクトルの加算
/// </summary>
/// <param name="v1">左辺のベクトル</param>
/// <param name="v2">右辺のベクトル</param>
/// <returns>v1 + v2</returns>
Vector3 Add(const Vector3& v1, const Vector3& v2);

/// <summary>
/// ベクトルの減算
/// </summary>
/// <param name="v1">左辺のベクトル</param>
/// <param name="v2">右辺のベクトル</param>
/// <returns>v1 - v2</returns>
Vector3 Subtract(const Vector3& v1, const Vector3& v2);

/// <summary>
/// ベクトルのスカラー倍
/// </summary>
/// <param name="v">元のベクトル</param>
/// <param name="s">倍率</param>
/// <returns>v を s 倍したベクトル</returns>
Vector3 Multiply(const Vector3& v, float s);

/// <summary>
/// 正射影ベクトル。v1 を v2 の方向へ射影する
/// </summary>
/// <param name="v1">射影されるベクトル</param>
/// <param name="v2">射影先のベクトル</param>
/// <returns>v2 上への v1 の正射影。v2 がゼロベクトルの場合はゼロベクトル</returns>
Vector3 Project(const Vector3& v1, const Vector3& v2);

#pragma endregion

#pragma region

/// <summary>
/// ベクトルのスカラー倍（スカラーが左辺の形）
/// </summary>
/// <param name="s">倍率</param>
/// <param name="v">元のベクトル</param>
/// <returns>v を s 倍したベクトル</returns>
Vector3 Multiply(float s, const Vector3& v);

/// <summary>
/// ベクトルの長さ
/// </summary>
/// <param name="v">元のベクトル</param>
/// <returns>vの長さ（ユークリッドノルム）</returns>
float Length(const Vector3& v);

/// <summary>
/// ベクトルの正規化
/// </summary>
/// <param name="v">元のベクトル</param>
/// <returns>長さ1に正規化したベクトル。ゼロベクトルの場合はゼロベクトル</returns>
Vector3 Normalize(const Vector3& v);

#pragma endregion

} // namespace Engine
