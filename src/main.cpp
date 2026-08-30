#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#ifndef UNICODE
#define UNICODE
#endif
#define ID_OPEN_YAML 1001
#define ID_OPEN_FOLDER 1002
#define ID_CONVERT_VIDEO 1003
#define ID_EXPORT_CONFIG 2001
#define ID_VERSION 3001
#define WM_USER_REFRESH_LIST (WM_USER + 100)
#define WM_USER_UPDATE_ITEM (WM_USER + 101)

#include "main.h"

using namespace Gdiplus;

static wchar_t szWindowClass[] = L"LEX";
static wchar_t szTitle[] = L"LabelEX";
HINSTANCE hInst;
HANDLE hExitEvent = NULL;
HANDLE hMonitorThread = NULL;
HWND hPagePicture;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL DoCreateDialog(HWND hWnd, HWND* hPagePicture);
void StopFolderMonitor();
BOOL StartFolderMonitor(HWND hDlg);

int IDCForDpi(HWND hWnd, int oldIDC);

HWND DoCreateMenu(HWND hWnd)
{
	HMENU hMenu = CreateMenu();
	HMENU hSubMenuFile = CreatePopupMenu();
	HMENU hSubMenuOption = CreatePopupMenu();
	HMENU hSubMenuAbout = CreatePopupMenu();

	AppendMenu(hSubMenuFile, MF_STRING, ID_OPEN_YAML, L"打开数据集配置文件（暂不支持）");
	AppendMenu(hSubMenuFile, MF_STRING, ID_OPEN_FOLDER, L"打开文件夹");
	AppendMenu(hSubMenuFile, MF_STRING, ID_CONVERT_VIDEO, L"转换视频为图片集（暂不支持）");
	AppendMenu(hSubMenuOption, MF_STRING, ID_EXPORT_CONFIG, L"导出设置（暂不支持）");
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

DWORD WINAPI FolderMonitorThread(LPVOID lpParam)
{
	HWND hDlg = (HWND)lpParam;

	wchar_t szLocalPath[MAX_PATH];
	StringCchCopy(szLocalPath, _countof(szLocalPath), szFolderPath);

	HANDLE hDir = CreateFile(
		szLocalPath,
		FILE_LIST_DIRECTORY,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
		NULL
	);
	if (hDir == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	BYTE buffer[65536];
	DWORD dwBytesReturned;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	HANDLE hWaitArray[2] = {overlapped.hEvent, hExitEvent};

	while (TRUE)
	{
		if (!ReadDirectoryChangesW(
			hDir,
			buffer,
			sizeof(buffer),
			FALSE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE,
			&dwBytesReturned,
			&overlapped,
			NULL
		))
		{
			break;
		}
		DWORD dwWait = WaitForMultipleObjects(2, hWaitArray, FALSE, INFINITE);
		if (dwWait == WAIT_OBJECT_0)
		{
			if (GetOverlappedResult(hDir, &overlapped, &dwBytesReturned, FALSE))
			{
				BYTE* pBuffer = buffer;
				while (TRUE)
				{
					PFILE_NOTIFY_INFORMATION pNotify = (PFILE_NOTIFY_INFORMATION)pBuffer;
					wchar_t szFileName[MAX_PATH];
					DWORD len = pNotify->FileNameLength / sizeof(WCHAR);
					if (len < MAX_PATH)
					{
						StringCchCopyN(szFileName, _countof(szFileName), pNotify->FileName, len);
						szFileName[len] = L'\0';
					}

					wchar_t* pExt = PathFindExtension(szFileName);
					if (pExt && _wcsicmp(pExt, L".txt") == 0)
					{
						wchar_t szBaseName[MAX_PATH];
						StringCchCopy(szBaseName, _countof(szBaseName), szFileName);
						PathRemoveExtension(szBaseName);

						switch (pNotify->Action)
						{
						case FILE_ACTION_ADDED:
						case FILE_ACTION_MODIFIED:
						case FILE_ACTION_RENAMED_NEW_NAME:
							PostMessage(hPagePicture, WM_USER_UPDATE_ITEM, (WPARAM)_wcsdup(szBaseName), (LPARAM)TRUE);
							break;
						case FILE_ACTION_REMOVED:
						case FILE_ACTION_RENAMED_OLD_NAME:
							PostMessage(hPagePicture, WM_USER_UPDATE_ITEM, (WPARAM)_wcsdup(szBaseName), (LPARAM)FALSE);
							break;
						}
					}
					else if (pExt && IsImageFile(pExt))
					{
						switch (pNotify->Action)
						{
						case FILE_ACTION_ADDED:
						case FILE_ACTION_MODIFIED:
						case FILE_ACTION_RENAMED_NEW_NAME:
						case FILE_ACTION_REMOVED:
						case FILE_ACTION_RENAMED_OLD_NAME:
							PostMessage(hPagePicture, WM_USER_REFRESH_LIST, 0, 0);
							break;
						}
					}
					if (pNotify->NextEntryOffset == 0)
					{
						break;
					}
					pBuffer += pNotify->NextEntryOffset;
				}
			}
		}
		else if (dwWait == WAIT_OBJECT_0 + 1)
		{
			break;
		}
		else
		{
			break;
		}
		ResetEvent(overlapped.hEvent);
	}

	CloseHandle(overlapped.hEvent);
	CloseHandle(hDir);
	return 0;
}

BOOL StartFolderMonitor(HWND hDlg)
{
	if (szFolderPath[0] == 0)
	{
		return FALSE;
	}
	StopFolderMonitor();
	hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (!hExitEvent)
	{
		return FALSE;
	}
	hMonitorThread = CreateThread(NULL, 0, FolderMonitorThread, hDlg, 0, NULL);
	if (!hMonitorThread)
	{
		CloseHandle(hExitEvent);
		hExitEvent = NULL;
		return FALSE;
	}
	return TRUE;
}

void StopFolderMonitor()
{
	if (hExitEvent)
	{
		SetEvent(hExitEvent);
		CloseHandle(hExitEvent);
		hExitEvent = NULL;
	}
	if (hMonitorThread)
	{
		DWORD dwWait = WaitForSingleObject(hMonitorThread, 2000);
		if (dwWait == WAIT_TIMEOUT)
		{
			MessageBox(NULL, L"Folder Monitor is not responding", L"Error",NULL);
		}
		CloseHandle(hMonitorThread);
		hMonitorThread = NULL;
	}
}

int IDCForDpi(HWND hWnd, int oldIDC)
{
	UINT newDpi = GetDpiForWindow(hWnd);
	int newIDC = MulDiv(oldIDC, newDpi, 96);
	return newIDC;
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nCmdShow
)
{
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAIN_ICON));
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

	HACCEL hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	GdiplusShutdown(gdiplusToken);
	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		DoCreateMenu(hWnd);
		DoCreateDialog(hWnd, &hPagePicture);
		return 0;
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case ID_VERSION:
			MessageBox(hWnd,
				L"LabelEX - for Yolo\n版本: 0.0.0\n(Developer)Build 00007",
				L"关于",
				MB_OK);
			return 0;
		case ID_OPEN_FOLDER:
		{
			DoSelectFolder(hPagePicture);
			return 0;
		}
		default:
			if (hPagePicture)
				SendMessage(hPagePicture, WM_COMMAND, wParam, lParam);
			return 0;
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
		int minWidth = MulDiv(980, dpi, 96);
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
		StopFolderMonitor();
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	}
}