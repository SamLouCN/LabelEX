#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifndef UNICODE
#define UNICODE
#endif
#define ID_OPEN_IMG 1001
#define ID_OPEN_FOLDER 1002
#define ID_OPEN_VIDEO 1003
#define ID_EXPORT_CONFIG 2001
#define ID_VERSION 3001

#include "main.h"

static wchar_t szWindowClass[] = L"LEX";
static wchar_t szTitle[] = L"LabelEX";
HINSTANCE hInst;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL DoCreateDialog(HWND hWnd, HWND* hPagePicture);
HWND hPagePicture;

HWND DoCreateMenu(HWND hWnd)
{
	HMENU hMenu = CreateMenu();
	HMENU hSubMenuFile = CreatePopupMenu();
	HMENU hSubMenuOption = CreatePopupMenu();
	HMENU hSubMenuAbout = CreatePopupMenu();

	AppendMenu(hSubMenuFile, MF_STRING, ID_OPEN_IMG, L"打开图片");
	AppendMenu(hSubMenuFile, MF_STRING, ID_OPEN_FOLDER, L"打开文件夹");
	AppendMenu(hSubMenuFile, MF_STRING, ID_OPEN_VIDEO, L"打开视频");
	AppendMenu(hSubMenuOption, MF_STRING, ID_EXPORT_CONFIG, L"导出设置");
	AppendMenu(hSubMenuAbout, MF_STRING, ID_VERSION, L"版本");

	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenuFile, L"文件(&L)");
	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenuOption, L"选项(&O)");
	AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenuAbout, L"关于(&A)");
	SetMenu(hWnd, hMenu);
	DrawMenuBar(hWnd);
	return 0;
}

BOOL DoCreateDialog(HWND hWnd, HWND* hPagePicture)
{
	*hPagePicture = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGEPICTURE), hWnd, DlgProc_Picture);
	if (*hPagePicture == NULL)
	{
		MessageBox(NULL, L"Failed to create the dialog", L"Error", NULL);
		return FALSE;
	}
	ShowWindow(*hPagePicture, SW_SHOW);
	return TRUE;
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nCmdShow
)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);
	
	InitCommonControls();

	if (!RegisterClassEx(&wcex))
	{
		MessageBox(NULL,
			L"Call to RegisterClassEx failed!",
			L"error",
			NULL);
		return 1;
	}

	hInst = hInstance;

	HWND hWnd = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW &~WS_EX_CLIENTEDGE,
		szWindowClass,
		szTitle,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1500, 1000,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		DoCreateMenu(hWnd);
		DoCreateDialog(hWnd, &hPagePicture);
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_COMPOSITED);
		return 0;
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case ID_VERSION:
			MessageBox(hWnd,
				L"LabelEX - for Yolo\n版本: 0.0.0\n(Developer)Build 00000",
				L"关于",
				MB_OK);
		}
		return 0;
	}
	case WM_DPICHANGED:
	{
		RECT* prcNewWindow = (RECT*)lParam;
		SetWindowPos(hWnd, NULL,
			prcNewWindow->left, prcNewWindow->top,
			prcNewWindow->right - prcNewWindow->left,
			prcNewWindow->bottom - prcNewWindow->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
		UINT dpi = GetDpiForWindow(hWnd);
		int minWidth = MulDiv(800, dpi, 96);
		int minHeight = MulDiv(600, dpi, 96);
		pInfo->ptMinTrackSize.x = minWidth;
		pInfo->ptMinTrackSize.y = minHeight;
		return 0;
	}
	case WM_SIZE:
	{
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		int rcClientHeight;
		int rcClientWidth;
		rcClientHeight = rcClient.bottom - rcClient.top;
		rcClientWidth = rcClient.right - rcClient.left;
		if (hPagePicture)
		{
			SetWindowPos(hPagePicture, NULL, rcClient.left, rcClient.top, rcClientWidth, rcClientHeight, SWP_NOZORDER);
		}
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
}