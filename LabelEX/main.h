#pragma once
#include <windows.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <strsafe.h>
#include "resource.h"

INT_PTR CALLBACK DlgProc_Picture(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
void RefreshList(HWND hWnd);
void DoSelectFolder(HWND hWnd);
void StopFolderMonitor();
BOOL StartFolderMonitor(HWND hDlg);
int IDCForDpi(HWND hWnd, int oldIDC);

extern wchar_t szFolderPath[MAX_PATH];
extern HWND hPagePicture;