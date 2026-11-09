#include <Windows.h>

#include "MyGame.h"

// Windowsアプリのエントリーポイント
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	MyGame game;
	game.Run();

	return 0;
}
