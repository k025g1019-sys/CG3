#include <Windows.h>

#include "Sandbox/SandboxApp.h"

// Windowsアプリのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	SandboxApp game;
	game.Run();

	return 0;
}
