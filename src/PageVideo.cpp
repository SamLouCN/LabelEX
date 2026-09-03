#ifndef UNICODE
#define UNICODE
#endif

#include "main.h"

INT_PTR CALLBACK DlgProc_Video(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"‘§¥¶¿Ì");
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		return TRUE;
	}
	case WM_CLOSE:
	{
		DestroyWindow(hDlg);
		return TRUE;
	}
	}
	return FALSE;
}