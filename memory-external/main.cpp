#include <iostream>
#include <thread>
#include <chrono>
#include <Windows.h>

#include "memory/memory.hpp"
#include "classes/vector.hpp"
#include "hacks/hack.hpp"
#include "classes/globals.hpp"

// https://www.unknowncheats.me/forum/3846642-post734.html

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		g::hdcBuffer = CreateCompatibleDC(NULL);
		g::hbmBuffer = CreateCompatibleBitmap(GetDC(hWnd), screen_x, screen_y);
		SelectObject(g::hdcBuffer, g::hbmBuffer);

		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

		SetLayeredWindowAttributes(hWnd, RGB(255, 255, 255), 0, LWA_COLORKEY);

		std::cout << "[overlay] Window created successfully" << std::endl;
		break;
	}
	case WM_ERASEBKGND: // We handle this message to avoid flickering
		return TRUE;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		//DOUBLE BUFFERING
		FillRect(g::hdcBuffer, &ps.rcPaint, (HBRUSH)GetStockObject(WHITE_BRUSH));


		if (GetForegroundWindow() == hack::process->hwnd_) {
			hack::loop();
		}

		BitBlt(hdc, 0, 0, screen_x, screen_y, g::hdcBuffer, 0, 0, SRCCOPY);

		EndPaint(hWnd, &ps);
		InvalidateRect(hWnd, NULL, TRUE);
		break;
	}
	case WM_DESTROY:
		DeleteDC(g::hdcBuffer);
		DeleteObject(g::hbmBuffer);

		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int main() {
	SetConsoleTitle("cs2-external-esp");

	hack::process = std::make_shared<pProcess>();

	std::cout << "[cs2] Waiting for cs2.exe..." << std::endl;

	while (!hack::process->AttachProcess("cs2.exe"))
		std::this_thread::sleep_for(std::chrono::seconds(1));

	std::cout << "[cs2] Attached to cs2.exe" << std::endl;

	do {
		hack::base_module = hack::process->GetModule("client.dll");
		if (hack::base_module.base == 0) {
			std::this_thread::sleep_for(std::chrono::seconds(1));
			std::cout << "[cs2] Failed to find module client.dll" << std::endl;
		}
	} while (hack::base_module.base == 0);

	std::cout << "[overlay] Creating window overlay..." << std::endl;

	WNDCLASSEXA wc = { 0 };
	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.lpfnWndProc = WndProc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground = WHITE_BRUSH;
	wc.hInstance = reinterpret_cast<HINSTANCE>(GetWindowLongA(hack::process->hwnd_, (-6))); // GWL_HINSTANCE));
	wc.lpszMenuName = " ";
	wc.lpszClassName = " ";

	RegisterClassExA(&wc);

	RECT gameBounds;
	GetClientRect(hack::process->hwnd_, &gameBounds);

	// Create the window
	HINSTANCE hInstance = NULL;
	HWND hWnd = CreateWindowExA(WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, " ", "cs2-external-esp", WS_POPUP,
		gameBounds.left, gameBounds.top, gameBounds.right - gameBounds.left, gameBounds.bottom + gameBounds.left, NULL, NULL, hInstance, NULL); // NULL, NULL);

	if (hWnd == NULL)
		return 0;

	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	ShowWindow(hWnd, TRUE);
	SetActiveWindow(hack::process->hwnd_);

	// Message loop
	MSG msg;
	while (!(GetAsyncKeyState(VK_END) & 0x8000) && GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	std::cout << "[overlay] Destroying overlay window." << std::endl;
	DeleteDC(g::hdcBuffer);
	DeleteObject(g::hbmBuffer);

	std::cout << "[cs2] Deataching from process" << std::endl;
	hack::process->Close();

#ifdef NDEBUG
	std::cout << "[cs2] Press any key to close" << std::endl;
	std::cin.get();
#endif

	return 1;
}