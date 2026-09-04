#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <gdiplus.h>
#include <vector>
#include "resource.h"

INT_PTR CALLBACK DlgProc_Picture(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Mit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Video(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Dataset(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_InterfaceCfg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_ExportCfg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void DoSelectFolder(HWND hWnd);
void StopFolderMonitor();
BOOL StartFolderMonitor(HWND hDlg);
BOOL IsImageFile(LPCWSTR szExt);
void UpdateSingleItemStatus(HWND hDlg, LPCWSTR szBaseName, BOOL bExist);
int IDCForDpi(HWND hWnd, int oldIDC);

extern wchar_t szFolderPath[MAX_PATH];
extern HWND hPagePicture, hPageAbout, hPageMit, hPageVideo, hPageProcess, hPageDataset, hPageExportCfg, hPageInterfaceCfg;