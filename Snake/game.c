#include <Windows.h>
#include <stdio.h>
#include "math_custom.h"
#include "drawing.h"
#include "game.h"
#include "queue.h"

BOOL Running = TRUE;

static Snake_t player = { 0 };

POINT food = { .x = 50,.y = 50 };

int* BackBuffer;

BITMAPINFO BitMapInfo = { 0 };

HDC dcWindow = 0;

static double GTimePassed = 0;
static float SecondsPerTick = 0;
static __int64 GTimeCount = 0;

float Sys_InitFloatTime()
{
	LARGE_INTEGER Frequency;
	QueryPerformanceFrequency(&Frequency);
	SecondsPerTick = 1.0f / (float)Frequency.QuadPart;

	LARGE_INTEGER Counter;
	QueryPerformanceCounter(&Counter);
	GTimeCount = Counter.QuadPart;
	return SecondsPerTick;
}

float Sys_FloatTime()
{
	LARGE_INTEGER Counter;
	QueryPerformanceCounter(&Counter);

	__int64 Interval = Counter.QuadPart - GTimeCount;

	GTimeCount = Counter.QuadPart;
	double SecondsGoneBy = (double)Interval*SecondsPerTick;
	GTimePassed += SecondsGoneBy;

	return (float)GTimePassed;
}

BOOL CheckCollission()
{
	//border check

	if (player.pos.x < 0 || player.pos.x > BufferWidth - FoodWidth)
		return FALSE;

	if (player.pos.y < 0 || player.pos.y > BufferHeight - FoodWidth)
		return FALSE;

	BOOL skipNextCollission = FALSE;
	//food check
	if (player.pos.x == food.x && player.pos.y == food.y)
	{
		push(player.pos);
		food.x = RandomInt10(0, BufferWidth - FoodWidth);
		food.y = RandomInt10(0, BufferHeight - FoodHeight);
		skipNextCollission = TRUE;
		player.length++;
	}

	//check for collision

	if (!skipNextCollission)
	{
		for (int i = 1; i < queue.count+1; i++)
		{
			if (queue.count == 1) break;
			int index = CalculateIndex(i)-1;
			if (index == queue.front) continue;
			if (player.pos.x == queue.stackArray[index].x && 
				player.pos.y == queue.stackArray[index].y)
			{
				return FALSE;
			}
		}
	}
	return TRUE;
}

int CalculateScreen(float timestep)
{
	wchar_t buf[100];
	swprintf_s(buf, 20, L"Score: %d", player.length);
	int len = lstrlenW((LPCWSTR)&buf);
	TextOutW(dcWindow, 0, 0, (LPCWSTR)&buf, len);

	if (timestep < 0.15f) 
		return TRUE;

	memset(BackBuffer, 0xFF, BufferWidth * BufferHeight * 4); //4 = size of integer

	switch (player.direction)
	{
	case Down:
		player.pos.y += FoodHeight;
		break;
	case Up:
		player.pos.y -= FoodHeight;
		break;
	case Left:
		player.pos.x -= FoodWidth;
		break;
	case Right:
		player.pos.x += FoodWidth;
		break;
	}
	push(player.pos);
	//draw
	for (int i = 1; i < queue.count + 1; i++)
	{
		int index = CalculateIndex(i);
		DrawRect(queue.stackArray[index].x, queue.stackArray[index].y, FoodWidth, FoodHeight, 0, BackBuffer, BufferWidth, BufferHeight);
	}

	BOOL lost = CheckCollission();

	pop();
	//draw food :)
	DrawRect(food.x, food.y, FoodWidth, FoodHeight, 0, BackBuffer, BufferWidth, BufferHeight);

	//display image
	StretchDIBits(dcWindow,
		0, 0, BufferWidth, BufferHeight,
		0, 0, BufferWidth, BufferHeight,
		BackBuffer, &BitMapInfo,
		DIB_RGB_COLORS, SRCCOPY);

	return lost;
}

//TA DA! It took me THREE days to get the snake body to work... all because I was iterating the wrong way on my queue... haha. The smallest mistakes... oh well! It's complete now. Maybe
//later I'll make a tutorial on a basic snake game in C, and that's C not C++ :)
//have a good day, I'll start recording these with voice once I feel like it. 
//source is on GitHub in the description

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;

	switch (uMsg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			player.direction = Left;
			break;
		case VK_RIGHT:
			player.direction = Right;
			break;
		case VK_UP:
			player.direction = Up;
			break;
		case VK_DOWN:
			player.direction = Down;
			break;
		case VK_ESCAPE:
			Running = FALSE;
			return Result;
		}
		return Result;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_CLOSE:
		exit(0);
	case WM_QUIT:
		exit(0);
		break;
	case WM_COMMAND:
		return Result;
	case WM_PAINT:
	default:
		Result = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return Result;
}

BOOL CreateGameWindow(HINSTANCE hInstance, int nShowCmd, int width, int height)
{
	RECT r = { 0 };
	r.right = width;
	r.bottom = height;
	AdjustWindowRect(&r, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

	//define window
	WNDCLASSEX wc = { 0 };
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	wc.lpszClassName = L"snakegame";

	if (!RegisterClassEx(&wc))
		return FALSE;

	HWND hwndWindow = CreateWindowExW(
		0,
		L"snakegame",
		L"Snake Game",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		r.right - r.left,
		r.bottom - r.top,
		NULL,
		NULL,
		hInstance,
		0);

	if (!hwndWindow) 
		return FALSE;

	BitMapInfo.bmiHeader.biSize = sizeof(BitMapInfo.bmiHeader);
	BitMapInfo.bmiHeader.biWidth = width;
	BitMapInfo.bmiHeader.biHeight = -height;
	BitMapInfo.bmiHeader.biPlanes = 1;
	BitMapInfo.bmiHeader.biBitCount = 32;
	BitMapInfo.bmiHeader.biCompression = BI_RGB;

	dcWindow = GetDC(hwndWindow);

	BackBuffer = (int*)malloc(width*height*4);

	return TRUE;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	#if DEBUG
		OpenLog();
	#endif

	if (!CreateGameWindow(hInstance, nShowCmd, BufferWidth, BufferHeight))
		return EXIT_FAILURE;

	food.x = RandomInt10(0, BufferWidth - FoodWidth);
	food.y = RandomInt10(0, BufferHeight - FoodHeight);

	Sys_InitFloatTime();

	float PrevTime = Sys_InitFloatTime();
	float TimeAccumulated = 0;

	MSG msg;

	while (Running)
	{
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		float NewTime = Sys_FloatTime();
		Running = CalculateScreen(NewTime - PrevTime);

		//put check inside CalculateScreen() later... v
		if(NewTime-PrevTime>0.15f)PrevTime = NewTime;
	}

	return EXIT_SUCCESS;
}

