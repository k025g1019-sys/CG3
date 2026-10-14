#include "Engine/Rendering/ParticleSystem.h"

#include <cassert>

#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/Time.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Rendering/MeshManager.h"
#include "Engine/Rendering/ParticleEmitter.h"
#include "Engine/Rendering/RenderContext.h"

namespace Engine {

namespace {

// 色の成分を0～1におさめる
float Saturate(float value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

} // namespace

void ParticleSystem::Initialize(const std::string& objFilePath, uint32_t maxParticleCount) {
	assert(maxParticleCount > 0);

	ID3D12Device* device = DirectXCore::GetInstance()->GetDevice();

	// --- メッシュ（同じOBJは1回だけ読み込まれ、Object3Dとも共有される）---
	mesh_ = MeshManager::GetInstance()->Load(objFilePath);

	// --- テクスチャ（最初のサブメッシュのmtl由来。map_Kdが無ければ白）---
	TextureManager* textureManager = TextureManager::GetInstance();
	textureHandle_ = textureManager->GetWhiteTexture();
	if (mesh_->GetSubMeshCount() > 0 && !mesh_->GetSubMesh(0).textureFilePath.empty()) {
		textureHandle_ = textureManager->Load(mesh_->GetSubMesh(0).textureFilePath);
	}

	// --- マテリアル（全パーティクル共通。パーティクルはライティングしない）---
	material_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	material_.lightingMode = LightingMode::kNone;
	material_.uvTransform = MakeIdentity4x4();
	materialCB_.Create(device, DirectXCore::kFramesInFlight);

	// --- 乱数生成器の初期化（random_deviceで実行のたびに違う種を作る）---
	std::random_device seedGenerator;
	randomEngine_.seed(seedGenerator());

	// --- パーティクルは最初は1個も出ていない（エミッタからEmitで発生させる）---
	particles_.clear();
	instanceCount_ = 0;

	// --- Instancing用にParticleForGPUをパーティクルの最大数ぶん格納できるResourceを作る ---
	// （フレームインフライト数ぶんのスロットを持ち、スロットごとにSRVを作る）
	instancingBuffer_.Create(device, maxParticleCount, DirectXCore::kFramesInFlight);

	// 各フレームのインスタンシングバッファを初期化
	for (uint32_t slot = 0; slot < DirectXCore::kFramesInFlight; ++slot) {
		ParticleForGPU* instancingData = instancingBuffer_.GetData(slot);

		for (uint32_t index = 0; index < maxParticleCount; ++index) {
			instancingData[index].World = MakeIdentity4x4();
			instancingData[index].color = { 1.0f, 1.0f, 1.0f, 1.0f };
		}
	}
}

void ParticleSystem::Emit(const ParticleEmitter& emitter) {
	for (uint32_t count = 0; count < emitter.count; ++count) {
		// 最大数を超えるぶんは発生させない（描画できないパーティクルを作らない）
		if (IsFull()) {
			break;
		}

		particles_.push_back(MakeNewParticle(randomEngine_, emitter));
	}
}

void ParticleSystem::Update(const Matrix4x4& viewMatrix) {
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- ビルボード行列（カメラの回転と同じ回転をさせて、パーティクルの表を常にカメラへ向ける）---
	// カメラのワールド行列（ビュー行列の逆行列）から、平行移動成分を消して回転だけにする。
	// （板ポリ（plane.obj）の表は最初からカメラ側（-Z方向）を向いているため、裏返す回転は掛けない）
	Matrix4x4 billboardMatrix = Inverse(viewMatrix);
	billboardMatrix.m[3][0] = 0.0f;
	billboardMatrix.m[3][1] = 0.0f;
	billboardMatrix.m[3][2] = 0.0f;

	// --- 各パーティクルのワールド行列と色を、このフレームのスロットへ書き込む ---
	// （ビュー射影は視点ごとの共有CBuffer（VSのb1）で渡すため、ここではワールド行列だけを計算する）
	ParticleForGPU* instancingData = instancingBuffer_.GetData(frameIndex);

	const float deltaTime = Time::GetDeltaTime();

	// 確保したResourceの数（これを超えて書き込むと、バッファの外を壊してしまう）
	const uint32_t maxInstanceCount = instancingBuffer_.GetElementCount();

	// 描画すべきインスタンス数（書き込んだ数を数える）
	uint32_t numInstance = 0;

	for (std::list<Particle>::iterator particleIterator = particles_.begin();
		particleIterator != particles_.end();) {
		Particle& particle = *particleIterator;

		// 速度を使って移動させる
		particle.transform_.translate += particle.velocity_ * deltaTime;

		// 経過時間を足す
		particle.currentTime_ += deltaTime;

		// 生存時間を過ぎたパーティクルは消滅させる（eraseは、消した要素の次を指すイテレータを返す）
		if (particle.IsDead()) {
			particleIterator = particles_.erase(particleIterator);
			continue;
		}

		// 最大数を超えて描画しないようにする（バッファオーバーラン対策）
		if (numInstance < maxInstanceCount) {
			ParticleForGPU& instancing = instancingData[numInstance];

			if (isBillboardEnabled_) {
				// ビルボードを使う：回転の代わりにビルボード行列を掛ける（Scale * Billboard * Translate）
				Matrix4x4 scaleMatrix = MakeScaleMatrix(particle.transform_.scale);
				Matrix4x4 translateMatrix = MakeTranslateMatrix(particle.transform_.translate);
				instancing.World = scaleMatrix * billboardMatrix * translateMatrix;
			} else {
				// ビルボードを使わない：パーティクルのTransformの回転のまま
				instancing.World =
					MakeAffineMatrix(particle.transform_.scale, particle.transform_.rotate, particle.transform_.translate);
			}

			// 経過時間が生存時間に近づくほど、α値を下げて薄くしていく（発生時は初期色のα、消滅時は0）
			const float alpha = 1.0f - (particle.currentTime_ / particle.lifeTime_);
			instancing.color = particle.color_;
			instancing.color.w = particle.color_.w * alpha;

			// 生きているパーティクルの数を1つカウントする
			++numInstance;
		}

		// 次のパーティクルへ進める
		++particleIterator;
	}

	// このフレームに描画するインスタンス数
	instanceCount_ = numInstance;

	// --- マテリアル（UV変換行列を組み立てて書き込む）---
	Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransform_.scale);
	uvTransformMatrix *= MakeRotateZMatrix(uvTransform_.rotate.z);
	uvTransformMatrix *= MakeTranslateMatrix(uvTransform_.translate);
	material_.uvTransform = uvTransformMatrix;
	materialCB_.Write(frameIndex, material_);
}

void ParticleSystem::Draw() const {
	// 描画するのは、UpdateでStructuredBufferへ書き込んだ数だけ
	if (instanceCount_ == 0) {
		return;
	}
	// 確保したResourceの数を超えて描くと、GPUが範囲外を読んでしまう
	assert(instanceCount_ <= instancingBuffer_.GetElementCount());

	RenderContext* context = RenderContext::GetInstance();
	ID3D12GraphicsCommandList* commandList = context->GetCommandList();
	PipelineManager* pipelineManager = PipelineManager::GetInstance();
	const uint32_t frameIndex = DirectXCore::GetInstance()->GetFrameIndex();

	// --- パーティクル用のRootSignature・PSOへ切り替える ---
	// （RootSignatureを変えると設定済みのCBV等は無効になるため、使うものはすべて設定し直す）
	commandList->SetGraphicsRootSignature(pipelineManager->GetParticleRootSignature());
	pipelineManager->SetPipeline(commandList, PipelineManager::Pipeline::kParticle);

	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kParticleRootMaterial, materialCB_.GetGPUAddress(frameIndex));
	// instancing用のDataを読むためにStructuredBufferのSRVを設定する
	commandList->SetGraphicsRootDescriptorTable(
		PipelineManager::kParticleRootInstancing, instancingBuffer_.GetSrvHandleGPU(frameIndex));
	commandList->SetGraphicsRootDescriptorTable(
		PipelineManager::kParticleRootTexture, TextureManager::GetInstance()->GetSrvHandleGPU(textureHandle_));
	// この視点のビュー射影（立体視では視点ごとに違う）
	commandList->SetGraphicsRootConstantBufferView(
		PipelineManager::kParticleRootViewProjection, context->GetViewProjectionAddress());

	// --- 描画！パーティクルの数ぶんを1回の描画命令でまとめて描く ---
	mesh_->Draw(commandList, instanceCount_);

	// --- 後に続く3D描画のために、標準のRootSignature・CBV・PSOへ戻す ---
	context->BindStandardState(commandList);
}

void ParticleSystem::SetTexture(const std::string& textureFilePath) {
	SetTextureHandle(TextureManager::GetInstance()->Load(textureFilePath));
}

ParticleSystem::Particle ParticleSystem::MakeNewParticle(std::mt19937& randomEngine, const ParticleEmitter& emitter) {
	// 「基準の値 ± 幅」に使う乱数（-1.0～1.0。幅を掛けて使うので、幅が0なら基準の値そのままになる）
	std::uniform_real_distribution<float> distribution(-1.0f, 1.0f);
	// 生存時間に使う乱数（0.0～1.0。最小～最大の間のどこにするかを決める）
	std::uniform_real_distribution<float> distTime(0.0f, 1.0f);

	Particle particle;
	particle.transform_.scale = { 1.0f, 1.0f, 1.0f };
	particle.transform_.rotate = { 0.0f, 0.0f, 0.0f };

	// 位置：エミッタの基準となる位置から、発生範囲の中でランダムにずらす
	particle.transform_.translate.x = emitter.translate.x + emitter.range.x * distribution(randomEngine);
	particle.transform_.translate.y = emitter.translate.y + emitter.range.y * distribution(randomEngine);
	particle.transform_.translate.z = emitter.translate.z + emitter.range.z * distribution(randomEngine);

	// 速度：基準 ± 幅
	particle.velocity_.x = emitter.velocity.x + emitter.velocityRange.x * distribution(randomEngine);
	particle.velocity_.y = emitter.velocity.y + emitter.velocityRange.y * distribution(randomEngine);
	particle.velocity_.z = emitter.velocity.z + emitter.velocityRange.z * distribution(randomEngine);

	// 初期色：基準色 ± 幅（0～1におさめる。幅が0なら全パーティクルが基準色になる）
	particle.color_.x = Saturate(emitter.color.x + emitter.colorRange.x * distribution(randomEngine));
	particle.color_.y = Saturate(emitter.color.y + emitter.colorRange.y * distribution(randomEngine));
	particle.color_.z = Saturate(emitter.color.z + emitter.colorRange.z * distribution(randomEngine));
	particle.color_.w = Saturate(emitter.color.w + emitter.colorRange.w * distribution(randomEngine));

	// 生存時間：最小～最大の間
	particle.lifeTime_ = emitter.lifeTimeMin + (emitter.lifeTimeMax - emitter.lifeTimeMin) * distTime(randomEngine);
	particle.currentTime_ = 0.0f;
	return particle;
}

} // namespace Engine
