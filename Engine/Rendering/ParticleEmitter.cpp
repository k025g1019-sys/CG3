#include "Engine/Rendering/ParticleEmitter.h"

#include "Engine/Core/Time.h"

namespace Engine {

bool ParticleEmitter::Update() {
	// 時刻を進める
	frequencyTime += Time::GetDeltaTime();

	// 頻度より小さい間は、まだ発生させない
	if (frequencyTime < frequency) {
		return false;
	}

	// 余計に過ぎた時間も加味して、次の発生までの時間を計る
	frequencyTime -= frequency;

	// 頻度が1フレームより短いときは、溜まった時間を持ち越さない（発生は1フレームに1回まで）
	if (frequencyTime >= frequency) {
		frequencyTime = 0.0f;
	}

	return true;
}

AABB3D ParticleEmitter::CalcRangeAABB() const {
	AABB3D aabb;
	aabb.min = translate - range;
	aabb.max = translate + range;
	return aabb;
}

} // namespace Engine
