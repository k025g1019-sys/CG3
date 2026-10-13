#pragma once

#include "Engine/Math/Vector3.h"

#include <cstdint>
#include <map>
#include <string>
#include <variant>

/// <summary>
/// グローバル変数（調整項目の一元管理）。各クラスが既定値を登録し、ImGui で編集し、CSV に保存・読込する。
/// 使い方: 起動時に LoadFiles() → 各クラスの RegisterGlobalVariables()（既定値の登録。読み込み済みの項目は上書きしない）
/// → 各クラスの ApplyGlobalVariables()（値を取り出して調整値へ反映。ImGui の変更を効かせるため毎フレーム呼ぶ）。
/// 保存先は resources/GlobalVariables/<グループ名>.csv。1 行が「項目名,型,値」で、型は int / float / vector3（値が 3 つ並ぶ）
/// </summary>
class GlobalVariables {
public:

	// 項目（1 項目分の値。int / float / Vector3 のどれかを保持する）
	using Item = std::variant<int32_t, float, Engine::Vector3>;
	// グループ（項目の集まり。項目は名前をキーにして管理する）
	using Group = std::map<std::string, Item>;

	/// <summary>
	/// インスタンスの取得
	/// </summary>
	static GlobalVariables* GetInstance();

	/// <summary>
	/// グループの作成（既にあれば何もしない）
	/// </summary>
	/// <param name="groupName">グループ名（保存ファイルの名前になる）</param>
	void CreateGroup(const std::string& groupName);

	// 値のセット（int ／ float ／ Vector3。グループと項目が無ければ作る）
	void SetValue(const std::string& groupName, const std::string& key, int32_t value);
	void SetValue(const std::string& groupName, const std::string& key, float value);
	void SetValue(const std::string& groupName, const std::string& key, const Engine::Vector3& value);

	// 項目の追加（未登録のときだけ既定値を入れる。ファイルから読み込み済みの値は上書きしない）
	void AddItem(const std::string& groupName, const std::string& key, int32_t value);
	void AddItem(const std::string& groupName, const std::string& key, float value);
	void AddItem(const std::string& groupName, const std::string& key, const Engine::Vector3& value);

	// 値の取得（グループか項目が無ければ assert。型が違えば assert して 0 を返す）
	int32_t GetIntValue(const std::string& groupName, const std::string& key) const;
	float GetFloatValue(const std::string& groupName, const std::string& key) const;
	Engine::Vector3 GetVector3Value(const std::string& groupName, const std::string& key) const;

	/// <summary>
	/// 1 グループを CSV に書き出す（保存先のフォルダが無ければ作る）
	/// </summary>
	/// <param name="groupName">グループ名</param>
	void SaveFile(const std::string& groupName);

	/// <summary>
	/// 全グループを CSV に書き出す
	/// </summary>
	void SaveAllFiles();

	/// <summary>
	/// 保存先フォルダの全 CSV を読み込む（起動時に 1 回。フォルダが無ければ何もしない）
	/// </summary>
	void LoadFiles();

	/// <summary>
	/// 1 グループの CSV を読み込む（無ければ何もしない。ImGui の Reload からも呼ぶ）
	/// </summary>
	/// <param name="groupName">グループ名</param>
	void LoadFile(const std::string& groupName);

#ifdef USE_IMGUI
	/// <summary>
	/// 「Global Variables」ウィンドウに全グループ・全項目を表示して編集する（保存・再読込のボタン付き）
	/// </summary>
	void DrawImGui();
#endif

private:

	GlobalVariables() = default;
	~GlobalVariables() = default;
	GlobalVariables(const GlobalVariables&) = delete;
	GlobalVariables& operator=(const GlobalVariables&) = delete;

	std::map<std::string, Group> datas_;  // 全データ（グループ名 → グループ）
	std::string message_;                 // 直近の保存・読込の結果（ImGui に表示する）
};
