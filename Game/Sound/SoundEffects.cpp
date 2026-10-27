#include "Game/Sound/SoundEffects.h"

#include "Engine/Audio/Audio.h"
#include "Game/Util/GlobalVariables.h"

#include <string>

using namespace Engine;

namespace {

// 効果音ごとのファイル名と調整項目の名前（SoundEffects::Id の順）
struct SoundInfo {
	const char* fileName;  // resources/sounds/ の中のファイル名
	const char* itemName;  // 調整項目の名前（音量）
};

const SoundInfo kSoundInfos[] = {
	{ "jump.wav", "JumpVolume" },
	{ "swing.wav", "SwingVolume" },
	{ "throw.wav", "ThrowVolume" },
	{ "catch.wav", "CatchVolume" },
	{ "hit.wav", "HitVolume" },
	{ "coreHit.wav", "CoreHitVolume" },
	{ "guard.wav", "GuardVolume" },
	{ "damage.wav", "DamageVolume" },
	{ "land.wav", "LandVolume" },
	{ "decide.wav", "DecideVolume" },
};

const char* kGroupName = "Sound";  // 調整項目のグループ名

} // namespace

#pragma region 読み込み

void SoundEffects::Load() {
	// resources/sounds/ の wav を Id の順に読み込む（kSoundInfos の並びと Id の並びはそろえておく）
	Audio* audio = Audio::GetInstance();
	for (int32_t i = 0; i < kSoundCount; ++i) {
		handles_[i] = audio->LoadWave(std::string("resources/sounds/") + kSoundInfos[i].fileName);
	}
	isLoaded_ = true;
}

#pragma endregion

#pragma region 再生

void SoundEffects::Play(Id id) {
	if (!isLoaded_ || id == Id::kCount) {
		return;
	}

	// 鳴らす直前に音量を合わせる（調整値の変更をすぐ反映する）
	const int32_t index = static_cast<int32_t>(id);
	Audio* audio = Audio::GetInstance();
	audio->SetVolume(handles_[index], kMasterVolume * kVolumes[index]);
	audio->Play(handles_[index]);
}

#pragma endregion

#pragma region 調整値

void SoundEffects::RegisterGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	globalVariables->CreateGroup(kGroupName);

	// 項目の追加（ファイルから読み込み済みの項目は既定値で上書きされない）
	globalVariables->AddItem(kGroupName, "MasterVolume", kMasterVolume);
	for (int32_t i = 0; i < kSoundCount; ++i) {
		globalVariables->AddItem(kGroupName, kSoundInfos[i].itemName, kVolumes[i]);
	}
}

void SoundEffects::ApplyGlobalVariables() {
	GlobalVariables* globalVariables = GlobalVariables::GetInstance();
	kMasterVolume = globalVariables->GetFloatValue(kGroupName, "MasterVolume");
	for (int32_t i = 0; i < kSoundCount; ++i) {
		kVolumes[i] = globalVariables->GetFloatValue(kGroupName, kSoundInfos[i].itemName);
	}
}

#pragma endregion
