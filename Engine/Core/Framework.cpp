#include "Engine/Core/Framework.h"

#include <cassert>

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#include <wrl.h>

#include "Engine/Audio/Audio.h"
#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/Time.h"
#include "Engine/Core/WinApp.h"
#include "Engine/Graphics/DescriptorHeapManager.h"
#include "Engine/Graphics/PipelineManager.h"
#include "Engine/Graphics/ShaderCompiler.h"
#include "Engine/Graphics/StereoRenderer.h"
#include "Engine/Graphics/TextureManager.h"
#include "Engine/Input/Input.h"
#include "Engine/Diagnostics/CrashHandler.h"
#include "Engine/Diagnostics/Log.h"
#include "Engine/Rendering/MeshManager.h"
#include "Engine/Scene/SceneManager.h"

#ifdef USE_IMGUI
#include "Engine/Core/ImGuiManager.h"
#endif

using Microsoft::WRL::ComPtr;

namespace Engine {

void Framework::Run() {

	Initialize();

	WinApp* winApp = WinApp::GetInstance();
	DirectXCore* dxCore = DirectXCore::GetInstance();
	ID3D12GraphicsCommandList* commandList = dxCore->GetCommandList();

	StereoRenderer* stereo = StereoRenderer::GetInstance();
	SceneManager* sceneManager = SceneManager::GetInstance();

	// --- メインループ（ウィンドウの×ボタンが押されるまで）---
	while (winApp->ProcessMessage()) {

		// ウィンドウサイズが変わっていたら、スワップチェーン・深度バッファ・ビューポートを作り直す
		if (winApp->IsSizeChanged()) {
			dxCore->Resize(winApp->GetClientWidth(), winApp->GetClientHeight());
			// DirectXCore::ResizeでGPU完了待ち済みなので、続けて立体視のオフスクリーンも作り直す
			stereo->Resize(winApp->GetClientWidth(), winApp->GetClientHeight());
			winApp->ClearSizeChangedFlag();
		}

		// 予約されたシーンへ切り替える（最初のシーンもここで生成される）
		sceneManager->ApplySceneChange();

#ifdef USE_IMGUI
		ImGuiManager::GetInstance()->BeginFrame();
		DrawImGui();
		ImGuiManager::GetInstance()->Render();
#endif

		// 入力状態を更新（全キーを取得し、前フレームと比較できるようにする）
		Input::GetInstance()->Update();

		// F11でボーダレス全画面を切り替え（物理シート整列用。サイズ変更は次フレーム冒頭で反映される）
		if (Input::GetInstance()->IsTrigger(DIK_F11)) {
			winApp->ToggleFullscreen();
		}

		// ゲームの描画先矩形をシーンへ伝える（投影のアスペクト比・スプライト・ピッキングの基準になる）。
		// ImGuiビルドではドッキングで空いた中央領域、それ以外ではウィンドウ全体。
#ifdef USE_IMGUI
		{
			const ImGuiManager::GameArea area = ImGuiManager::GetInstance()->GetGameArea();
			sceneManager->SetRenderArea(area.x, area.y, area.width, area.height);
		}
#else
		sceneManager->SetRenderArea(
			0.0f, 0.0f, float(winApp->GetClientWidth()), float(winApp->GetClientHeight()));
#endif

		Update();

		dxCore->BeginFrame();

		// シーン描画前のフック（Webカメラテクスチャの転送コマンド発行など）
		PreDraw(commandList);

		// ゲームの表示先（立体視合成／通常描画の両方で使う）。
		// ImGuiビルドではドッキングで空いた中央領域に合わせ、ゲームがUIに隠れないようにする
		const D3D12_VIEWPORT fullViewport = dxCore->GetViewport();
		const D3D12_RECT fullScissor = dxCore->GetScissorRect();
		D3D12_VIEWPORT gameViewport = fullViewport;
		D3D12_RECT gameScissor = fullScissor;
#ifdef USE_IMGUI
		{
			const ImGuiManager::GameArea area = ImGuiManager::GetInstance()->GetGameArea();
			if (area.width > 0.0f && area.height > 0.0f) {
				gameViewport.TopLeftX = area.x;
				gameViewport.TopLeftY = area.y;
				gameViewport.Width = area.width;
				gameViewport.Height = area.height;
				gameScissor.left = static_cast<LONG>(area.x);
				gameScissor.top = static_cast<LONG>(area.y);
				gameScissor.right = static_cast<LONG>(area.x + area.width);
				gameScissor.bottom = static_cast<LONG>(area.y + area.height);
			}
		}
#endif

		if (stereo->IsEnabled()) {
			// --- 立体視：各視点をオフスクリーンへ描画し、バックバッファへ合成する ---
			const uint32_t viewCount = stereo->GetActiveViewCount();
			for (uint32_t view = 0; view < viewCount; ++view) {
				stereo->BeginView(commandList, view);
				Draw(commandList, view);
			}
			stereo->Composite(
				commandList, dxCore->GetCurrentRTVHandle(), gameViewport, gameScissor);
		} else {
			// --- 通常描画：バックバッファへ直接1回描画（オフスクリーン・合成のコストなし）---
			// ゲームの描画先を表示領域へ絞る（RT/クリアはBeginFrame済み）
			commandList->RSSetViewports(1, &gameViewport);
			commandList->RSSetScissorRects(1, &gameScissor);
			Draw(commandList, 0);
		}

#ifdef USE_IMGUI
		// ImGuiは画面全体へ描く（合成・分割の後）
		{
			D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV = dxCore->GetCurrentRTVHandle();
			commandList->OMSetRenderTargets(1, &backBufferRTV, false, nullptr);
			commandList->RSSetViewports(1, &fullViewport);
			commandList->RSSetScissorRects(1, &fullScissor);
		}
		ImGuiManager::GetInstance()->Draw(commandList);
#endif

		dxCore->EndFrame();

		// 毎秒60回に固定する（1フレームの時間が経つまで待ち、次のフレームの経過時間を確定する）
		Time::WaitForNextFrame();
	}

	// 実行中のフレームが参照しているリソースを解放する前にGPU完了を待つ
	dxCore->WaitForGPU();

	Finalize();
}

void Framework::Initialize() {

	// COMの初期化
	[[maybe_unused]] HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	// 未捕捉の例外時にクラッシュダンプを出力する関数を登録
	SetUnhandledExceptionFilter(ExportDump);

	// ログファイルを用意（以降のLogは出力ウィンドウとファイルの両方へ出る）
	InitializeLogFile();

	// フレームの時間管理（60FPS固定・経過時間）
	Time::Initialize();

	// --- ウィンドウ生成 ---
	WinApp* winApp = WinApp::GetInstance();
	winApp->Initialize();

	// --- DirectX12初期化 ---
	DirectXCore* dxCore = DirectXCore::GetInstance();
	dxCore->Initialize(
		winApp->GetHwnd(),
		WinApp::kClientWidth,
		WinApp::kClientHeight);

	Log("Complete create D3D12Device!!!");

	// --- 各サブシステム初期化 ---
	ShaderCompiler::GetInstance()->Initialize();
	DescriptorHeapManager::GetInstance()->Initialize(dxCore->GetDevice(), dxCore->GetSRVDescriptorHeap());
	PipelineManager::GetInstance()->Initialize(dxCore->GetDevice());
	TextureManager::GetInstance()->Initialize(dxCore->GetDevice(), dxCore->GetCommandList());
	StereoRenderer::GetInstance()->Initialize(
		dxCore->GetDevice(), winApp->GetClientWidth(), winApp->GetClientHeight());
	Audio::GetInstance()->Initialize();
	Input::GetInstance()->Initialize(winApp->GetHwnd());

#ifdef USE_IMGUI
	ImGuiManager::GetInstance()->Initialize();
#endif
}

void Framework::Finalize() {

	// シーンのGPUリソースを、エンジンの終了処理（リークチェック）より先に解放する
	SceneManager::GetInstance()->Finalize();

#ifdef USE_IMGUI
	// ImGui終了処理（SRVヒープ解放より前に行う）
	ImGuiManager::GetInstance()->Finalize();
#endif

	// --- 終了処理（生成と逆順で解放する）---
	MeshManager::GetInstance()->Finalize();
	Audio::GetInstance()->Finalize();
	Input::GetInstance()->Finalize();
	StereoRenderer::GetInstance()->Finalize();
	TextureManager::GetInstance()->Finalize();
	PipelineManager::GetInstance()->Finalize();
	ShaderCompiler::GetInstance()->Finalize();
	DirectXCore::GetInstance()->Finalize();

	CloseWindow(WinApp::GetInstance()->GetHwnd());

	// --- リソースリークチェック ---
	ComPtr<IDXGIDebug1> debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
	}

	Time::Finalize();

	CoUninitialize();
}

void Framework::Update() {
	SceneManager::GetInstance()->Update();
}

void Framework::Draw(ID3D12GraphicsCommandList* commandList, uint32_t viewIndex) {
	SceneManager::GetInstance()->Draw(commandList, viewIndex);
}

#ifdef USE_IMGUI
void Framework::DrawImGui() {
	SceneManager::GetInstance()->DrawImGui();
}
#endif

void Framework::SetWindowTitle(const std::wstring& title) {
	WinApp::GetInstance()->SetTitle(title);
}

} // namespace Engine
