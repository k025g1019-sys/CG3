#include "Engine/Rendering/RenderContext.h"

#include "Engine/Core/DirectXCore.h"
#include "Engine/Core/WinApp.h"

namespace Engine {

RenderContext* RenderContext::GetInstance() {
	static RenderContext instance;
	return &instance;
}

RenderContext::RenderContext()
	: screenWidth_(float(WinApp::kClientWidth))
	, screenHeight_(float(WinApp::kClientHeight)) {
}

ID3D12GraphicsCommandList* RenderContext::GetCommandList() const {
	return DirectXCore::GetInstance()->GetCommandList();
}

} // namespace Engine
