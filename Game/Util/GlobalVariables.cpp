#include "Game/Util/GlobalVariables.h"

#include "Engine/Diagnostics/Log.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

using namespace Engine;

namespace {

const std::string kResourcesPath = "resources/";                         // 保存先の親フォルダ（実行時のカレントは Game/）
const std::string kDirectoryPath = kResourcesPath + "GlobalVariables/";  // 調整値の保存先
const std::string kExtension = ".csv";                                   // 保存ファイルの拡張子
const std::string kHeaderLine = "name,type,value";                       // CSV の 1 行目（見出し。読み込み時は飛ばす）
const std::string kTypeInt = "int";                                      // 型の名前（CSV の 2 列目）
const std::string kTypeFloat = "float";
const std::string kTypeVector3 = "vector3";

// 行の残りからカンマ区切りの 1 語を取り出して float にする（語が無ければ false）
bool ReadFloat(std::istringstream& lineStream, float& outValue) {
	std::string word;
	std::getline(lineStream, word, ',');
	if (word.empty()) {
		return false;
	}
	outValue = std::stof(word);
	return true;
}

} // namespace

#pragma region 登録と取得

GlobalVariables* GlobalVariables::GetInstance() {
	static GlobalVariables instance;
	return &instance;
}

void GlobalVariables::CreateGroup(const std::string& groupName) {
	// 指定名のグループが無ければ追加する（あれば何もしない）
	datas_[groupName];
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, int32_t value) {
	Group& group = datas_[groupName];
	Item newItem = value;
	group[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, float value) {
	Group& group = datas_[groupName];
	Item newItem = value;
	group[key] = newItem;
}

void GlobalVariables::SetValue(const std::string& groupName, const std::string& key, const Vector3& value) {
	Group& group = datas_[groupName];
	Item newItem = value;
	group[key] = newItem;
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, int32_t value) {
	// 未登録の項目だけ既定値を入れる（ファイルから読み込み済みの値は残す）
	Group& group = datas_[groupName];
	if (group.find(key) == group.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, float value) {
	Group& group = datas_[groupName];
	if (group.find(key) == group.end()) {
		SetValue(groupName, key, value);
	}
}

void GlobalVariables::AddItem(const std::string& groupName, const std::string& key, const Vector3& value) {
	Group& group = datas_[groupName];
	if (group.find(key) == group.end()) {
		SetValue(groupName, key, value);
	}
}

int32_t GlobalVariables::GetIntValue(const std::string& groupName, const std::string& key) const {
	// 指定のグループと項目があるか（登録漏れや名前の打ち間違い）
	assert(datas_.contains(groupName));
	const Group& group = datas_.at(groupName);
	assert(group.contains(key));
	const Item& item = group.at(key);

	// 型が違えば（CSV の型の列が間違っている）0 を返す
	assert(std::holds_alternative<int32_t>(item));
	const int32_t* value = std::get_if<int32_t>(&item);
	return value ? *value : 0;
}

float GlobalVariables::GetFloatValue(const std::string& groupName, const std::string& key) const {
	assert(datas_.contains(groupName));
	const Group& group = datas_.at(groupName);
	assert(group.contains(key));
	const Item& item = group.at(key);

	assert(std::holds_alternative<float>(item));
	const float* value = std::get_if<float>(&item);
	return value ? *value : 0.0f;
}

Vector3 GlobalVariables::GetVector3Value(const std::string& groupName, const std::string& key) const {
	assert(datas_.contains(groupName));
	const Group& group = datas_.at(groupName);
	assert(group.contains(key));
	const Item& item = group.at(key);

	assert(std::holds_alternative<Vector3>(item));
	const Vector3* value = std::get_if<Vector3>(&item);
	return value ? *value : Vector3{ 0.0f, 0.0f, 0.0f };
}

#pragma endregion

#pragma region 保存と読込

void GlobalVariables::SaveFile(const std::string& groupName) {
	// グループを検索（未登録なら間違い）
	std::map<std::string, Group>::iterator itGroup = datas_.find(groupName);
	assert(itGroup != datas_.end());
	if (itGroup == datas_.end()) {
		return;
	}

	// 保存先のフォルダが無ければ作る（resources/ → resources/GlobalVariables/ の順）
	if (!std::filesystem::exists(kResourcesPath)) {
		std::filesystem::create_directory(kResourcesPath);
	}
	if (!std::filesystem::exists(kDirectoryPath)) {
		std::filesystem::create_directory(kDirectoryPath);
	}

	// 書き込み用に開く
	const std::string filePath = kDirectoryPath + groupName + kExtension;
	std::ofstream ofs;
	ofs.open(filePath);
	if (ofs.fail()) {
		message_ = "failed to open for write: " + filePath;
		Log(message_);
		return;
	}

	// 1 行目は見出し、2 行目からは「項目名,型,値」（Vector3 は値が 3 つ並ぶ）
	ofs << kHeaderLine << "\n";
	for (std::map<std::string, Item>::iterator itItem = itGroup->second.begin(); itItem != itGroup->second.end(); ++itItem) {
		const std::string& itemName = itItem->first;
		Item& item = itItem->second;

		if (std::holds_alternative<int32_t>(item)) {
			ofs << itemName << "," << kTypeInt << "," << std::get<int32_t>(item) << "\n";
		} else if (std::holds_alternative<float>(item)) {
			ofs << itemName << "," << kTypeFloat << "," << std::get<float>(item) << "\n";
		} else if (std::holds_alternative<Vector3>(item)) {
			const Vector3 value = std::get<Vector3>(item);
			ofs << itemName << "," << kTypeVector3 << "," << value.x << "," << value.y << "," << value.z << "\n";
		}
	}
	ofs.close();

	message_ = "saved: " + filePath;
	Log(message_);
}

void GlobalVariables::SaveAllFiles() {
	for (std::map<std::string, Group>::iterator itGroup = datas_.begin(); itGroup != datas_.end(); ++itGroup) {
		SaveFile(itGroup->first);
	}
	message_ = "saved all groups to " + kDirectoryPath;
}

void GlobalVariables::LoadFiles() {
	// 保存先が無ければ何もしない（既定値のまま）
	const std::filesystem::path dir(kDirectoryPath);
	if (!std::filesystem::exists(dir)) {
		return;
	}

	// フォルダの中の .csv をすべて読む（ファイル名がグループ名）
	std::filesystem::directory_iterator dirIt(dir);
	for (const std::filesystem::directory_entry& entry : dirIt) {
		const std::filesystem::path& filePath = entry.path();
		if (filePath.extension().string().compare(kExtension) != 0) {
			continue;
		}
		LoadFile(filePath.stem().string());
	}
}

void GlobalVariables::LoadFile(const std::string& groupName) {
	// 読み込み用に開く（無ければ何もしない）
	const std::string filePath = kDirectoryPath + groupName + kExtension;
	std::ifstream file;
	file.open(filePath);
	if (file.fail()) {
		message_ = "not found: " + filePath;
		Log(message_);
		return;
	}

	// ファイルの中身を文字列ストリームへ移してから 1 行ずつ読む（マップチップの CSV と同じ読み方）
	std::stringstream csv;
	csv << file.rdbuf();
	file.close();

	std::string line;
	while (std::getline(csv, line)) {
		// 行末の CR と空行は飛ばす
		if (!line.empty() && line.back() == '\r') {
			line.pop_back();
		}
		if (line.empty()) {
			continue;
		}

		// 「項目名,型,値」に分ける。見出し行は飛ばす
		std::istringstream lineStream(line);
		std::string itemName;
		std::string type;
		std::getline(lineStream, itemName, ',');
		std::getline(lineStream, type, ',');
		if (itemName == "name") {
			continue;
		}

		if (type == kTypeInt) {
			std::string word;
			std::getline(lineStream, word, ',');
			if (!word.empty()) {
				SetValue(groupName, itemName, std::stoi(word));
			}
		} else if (type == kTypeFloat) {
			float value = 0.0f;
			if (ReadFloat(lineStream, value)) {
				SetValue(groupName, itemName, value);
			}
		} else if (type == kTypeVector3) {
			Vector3 value = { 0.0f, 0.0f, 0.0f };
			if (ReadFloat(lineStream, value.x) && ReadFloat(lineStream, value.y) && ReadFloat(lineStream, value.z)) {
				SetValue(groupName, itemName, value);
			}
		}
	}

	message_ = "loaded: " + filePath;
	Log(message_);
}

#pragma endregion

#pragma region ImGui

#ifdef USE_IMGUI
void GlobalVariables::DrawImGui() {
	// 項目名が切れないよう、中身に合わせて大きさを変えるウィンドウにする
	ImGui::Begin("Global Variables", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("Save writes %s<group>%s", kDirectoryPath.c_str(), kExtension.c_str());
	if (ImGui::Button("Save All")) {
		SaveAllFiles();
	}
	if (ImGui::Button("Reload All")) {
		LoadFiles();
	}
	if (!message_.empty()) {
		ImGui::Text("%s", message_.c_str());
	}
	ImGui::Separator();

	// 各グループをツリーで表示し、項目は型ごとの Drag で編集する
	for (std::map<std::string, Group>::iterator itGroup = datas_.begin(); itGroup != datas_.end(); ++itGroup) {
		const std::string& groupName = itGroup->first;
		Group& group = itGroup->second;
		if (!ImGui::TreeNode(groupName.c_str())) {
			continue;
		}

		for (std::map<std::string, Item>::iterator itItem = group.begin(); itItem != group.end(); ++itItem) {
			const std::string& itemName = itItem->first;
			Item& item = itItem->second;

			if (std::holds_alternative<int32_t>(item)) {
				int32_t* ptr = std::get_if<int32_t>(&item);
				ImGui::DragInt(itemName.c_str(), ptr, 1.0f);
			} else if (std::holds_alternative<float>(item)) {
				float* ptr = std::get_if<float>(&item);
				ImGui::DragFloat(itemName.c_str(), ptr, 0.01f, 0.0f, 0.0f, "%.4f");
			} else if (std::holds_alternative<Vector3>(item)) {
				Vector3* ptr = std::get_if<Vector3>(&item);
				ImGui::DragFloat3(itemName.c_str(), reinterpret_cast<float*>(ptr), 0.01f);
			}
		}

		// グループごとの保存と再読込
		if (ImGui::Button("Save")) {
			SaveFile(groupName);
		}
		if (ImGui::Button("Reload")) {
			LoadFile(groupName);
		}
		ImGui::TreePop();
	}

	ImGui::End();
}
#endif

#pragma endregion
