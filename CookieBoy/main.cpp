#include "CookieboyDefs.h"
#include <Windows.h>
#include <SDL.h>
#include "CookieboyEmulator.h"
#include <io.h>
#include <Fcntl.h>
#include "render.h"
#include "AudioQueue.h"

bool Running = true;
bool FullSpeed = false;
HWND hWnd;
IDirect3DTexture9 *backbuffer;
Cookieboy::Joypad::ButtonsState Joypad;

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	BYTE keyState = 0;

	switch (uMessage)
	{
	case WM_CLOSE:
	case WM_QUIT:
		Running = false;
		break;

	case WM_KEYDOWN:
		keyState = 1;
	case WM_KEYUP:
		switch (wParam)
		{
		case 'Z':
			Joypad.A = keyState;
			break;

		case 'X':
			Joypad.B = keyState;
			break;

		case 'A':
			Joypad.select = keyState;
			break;

		case 'S':
			Joypad.start = keyState;
			break;

		case VK_RIGHT:
			Joypad.right = keyState;
			break;

		case VK_LEFT:
			Joypad.left = keyState;
			break;

		case VK_UP:
			Joypad.up = keyState;
			break;

		case VK_DOWN:
			Joypad.down = keyState;
			break;
			
		case VK_SPACE:
			FullSpeed = keyState == 1;
			break;
		}
		break;

	case WM_EXITSIZEMOVE:
		OnWindowSizeChange(hWnd);
		break;

	case WM_SIZE:
		if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
		{
			OnWindowSizeChange(hWnd);
		}
		break;

	default:
		break;
	}

    return DefWindowProc(hWnd,uMessage,wParam,lParam);
}

bool MakeWindow(HINSTANCE hInstance)
{
    WNDCLASSEX wc;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProcedure;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "Cookieboy";
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(5));

    if (!RegisterClassEx(&wc))
	{
        return true;
	}

	DWORD wndStyle = WS_CAPTION | WS_VISIBLE | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;

	RECT rect;
	rect.top = 0;
	rect.left = 0;
	rect.right = 160 * 4;
	rect.bottom = 144 * 4;
	AdjustWindowRect(&rect, wndStyle, FALSE);
	
	int x = (GetSystemMetrics(SM_CXSCREEN) - rect.right)/2;
	int y = (GetSystemMetrics(SM_CYSCREEN) - rect.bottom)/2;

	hWnd = CreateWindowEx(NULL, "Cookieboy", "Cookieboy", wndStyle, x, y, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);

    if (!hWnd)
	{
        return true;
	}

    return false;
}

int main(int argc, char *argv[])
{
	AllocConsole();
	HANDLE lStdHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	*stdout = *_fdopen(hConHandle, "w");
	setvbuf(stdout, NULL, _IONBF, 0);

	SDL_Init(SDL_INIT_AUDIO);

	if (MakeWindow(NULL))
	{
		return 0;
	}

	InitD3D(hWnd);

	backbuffer = CreateBackbuffer(160, 144);
	
	AudioQueue audioQueue = AudioQueue(44100, 1024, 5);
	Cookieboy::Emulator emulator = Cookieboy::Emulator(Cookieboy::GPU::RGBPALETTE_REAL, 44100, 1024);
	emulator.Reset();
	emulator.UseBIOS(false);

	OPENFILENAME ofn;
	char szFile[1024] = {0};
	
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = "*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;

	if (GetOpenFileName(&ofn) == FALSE)
	{
		return 0;
	}
	
	const Cookieboy::ROMInfo *ROM = emulator.LoadROM(ofn.lpstrFile);
	//const Cookieboy::ROMInfo *ROM = emulator.LoadROM("D:\\dev\\gamebuyemu_refactored\\ROMs\\Pokemon Yellow (U) [C][!].gbc");
	//const Cookieboy::ROMInfo *ROM = emulator.LoadROM("D:\\dev\\gamebuyemu_refactored\\ROMs\\Ant Soldiers (Sachen) [!].gb");
	if (ROM == NULL)
	{
		return 0;
	}

	printf("Game title: %s\n", ROM->gameTitle);
	printf("Color Gameboy: %s\n", ROM->colorGB ? "yes" : "no");
	printf("Super Gameboy: %s\n", ROM->superGB ? "yes" : "no");
	printf("Cartridge type: %s\n", ROM->CartTypeToString(ROM->cartType));
	printf("Old license: %s\n", ROM->OldLicenseToString(ROM->oldLicense));
	printf("New license: %s\n", ROM->NewLicenseToString(ROM->newLicense));
	printf("Destination: %s\n", ROM->DestinationCodeToString(ROM->destinationCode));
	printf("ROM size: %d banks\n", ROM->ROMSize);
	printf("RAM size: %d banks\n", ROM->RAMSize);

	SDL_PauseAudio(0);

	memset(&Joypad, 0, sizeof(Joypad));
	while (Running)
	{
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

		emulator.UpdateJoypad(Joypad);
		do
		{
			do
			{
				emulator.Step();
			}while (!emulator.IsNewSoundFrameReady());
			emulator.WaitForNewSoundFrame();

			audioQueue.Append(emulator.SoundFrameBuffer(), 1024, !FullSpeed);

		}while (!emulator.IsNewGPUFrameReady());
		emulator.WaitForNewGPUFrame();

		void *pixels = TextureLock(backbuffer);

		memcpy(pixels, emulator.GPUFramebuffer(), sizeof(DWORD) * 160 * 144);

		TextureUnlock(backbuffer);

		Render(backbuffer);
	}

	backbuffer->Release();
	CloseD3D();

	return 0;
}
