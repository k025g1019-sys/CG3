#ifdef USE_IMGUI

#include "Engine/Core/ImGuiManager.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/WinApp.h"
#include "Engine/Graphics/DescriptorHeapManager.h"

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_internal.h"  // ドッキングのDockBuilder API
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"

namespace Engine {

ImGuiManager* ImGuiManager::GetInstance() {
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(WinApp::GetInstance()->GetHwnd());

	// フォント用にSRVヒープのスロットを1つ確保して使う
	DescriptorHeapManager* heapManager = DescriptorHeapManager::GetInstance();
	uint32_t srvIndex = heapManager->AllocateSrv();
	ImGui_ImplDX12_Init(
		DirectXCore::GetInstance()->GetDevice(),
		DirectXCore::GetSwapChainBufferCount(),
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
		DirectXCore::GetInstance()->GetSRVDescriptorHeap(),
		heapManager->GetSrvCPUHandle(srvIndex),
		heapManager->GetSrvGPUHandle(srvIndex));

	ImGui::GetIO().Fonts->Build();
}

void ImGuiManager::SetDefaultDockLayout(
	std::vector<std::string> topRight, std::vector<std::string> bottomRight) {
	defaultDockTopRight_ = std::move(topRight);
	defaultDockBottomRight_ = std::move(bottomRight);
}

void ImGuiManager::BeginFrame() {
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// --- 画面全体を覆うドックスペース ---
	// 非表示のホストウィンドウを画面いっぱいに固定し、その中をドッキング先にする。
	// 中央ノードは透過（Passthru）で、ゲーム画面の表示とマウス入力を妨げない。
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	const ImGuiWindowFlags hostFlags =
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
		ImGuiWindowFlags_NoBackground;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("##DockHost", nullptr, hostFlags);
	ImGui::PopStyleVar(3);

	// ドックスペースのID（ウィンドウ名から決まり、起動をまたいで同一）。
	// ノードが無い＝imgui.iniに保存済みレイアウトが無い初回起動なので、デフォルト配置を組む
	const ImGuiID dockspaceId = ImGui::GetID("EngineDockSpace");
	if (ImGui::DockBuilderGetNode(dockspaceId) == nullptr) {
		BuildDefaultDockLayout(dockspaceId, viewport->WorkSize.x, viewport->WorkSize.y);
	}
	ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::End();

	// 中央ノード（どのウィンドウにも占有されていない領域）＝ゲームの表示先を記録する
	if (const ImGuiDockNode* central = ImGui::DockBuilderGetCentralNode(dockspaceId)) {
		gameArea_ = { central->Pos.x, central->Pos.y, central->Size.x, central->Size.y };
	}
}

void ImGuiManager::BuildDefaultDockLayout(unsigned int dockspaceId, float width, float height) {
	// 左＝ゲーム（中央ノード）／右上＝デバッグUI（タブ結合）／右下＝Webカメラ の初期配置。
	// 以降のレイアウト変更はimgui.iniへ保存されるため、ここは初回起動時のみ通る
	ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
	ImGui::DockBuilderSetNodeSize(dockspaceId, ImVec2(width, height));

	ImGuiID centralId = dockspaceId;
	ImGuiID rightTopId =
		ImGui::DockBuilderSplitNode(centralId, ImGuiDir_Right, 0.5f, nullptr, &centralId);
	ImGuiID rightBottomId =
		ImGui::DockBuilderSplitNode(rightTopId, ImGuiDir_Down, 0.5f, nullptr, &rightTopId);

	for (const std::string& name : defaultDockTopRight_) {
		ImGui::DockBuilderDockWindow(name.c_str(), rightTopId);
	}
	for (const std::string& name : defaultDockBottomRight_) {
		ImGui::DockBuilderDockWindow(name.c_str(), rightBottomId);
	}
	ImGui::DockBuilderFinish(dockspaceId);
}

void ImGuiManager::Render() {
	ImGui::Render();
}

void ImGuiManager::Draw(ID3D12GraphicsCommandList* commandList) {
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void ImGuiManager::Finalize() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

} // namespace Engine

#endif  // USE_IMGUI
