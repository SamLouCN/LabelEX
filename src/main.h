#pragma once

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}
#include <windows.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <string.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <Shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#include <gdiplus.h>
#include <vector>
#include <random>
#include <numeric>
#include "resource.h"
#include "history.h"
#include "ini.h"

#ifndef _LISTVIEW
#define IDC_LISTVIEW 5001
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef DWMWA_SYSTEMBACKDROP_TYPE
#define DWMWA_SYSTEMBACKDROP_TYPE 38
#endif
#ifndef DWMWA_COLOR_DEFAULT
#define DWMWA_COLOR_DEFAULT 0xFFFFFFFF
#endif

#ifndef DWMWCP_DEFAULT
typedef enum _DWM_WINDOW_CORNER_PREFERENCE
{
	DWMWCP_DEFAULT = 0, DWMWCP_DONOTROUND = 1,
	DWMWCP_ROUND = 2, DWMWCP_ROUNDSMALL = 3
} DWM_WINDOW_CORNER_PREFERENCE;
#endif
#ifndef DWMSBT_AUTO
typedef enum _DWM_SYSTEMBACKDROP_TYPE
{
	DWMSBT_AUTO = 0, DWMSBT_NONE = 1,
	DWMSBT_MAINWINDOW = 2, DWMSBT_TRANSIENTWINDOW = 3,
	DWMSBT_TABBEDWINDOW = 4
} DWM_SYSTEMBACKDROP_TYPE;
#endif

#include "Darkmodelib.h"

INT_PTR CALLBACK DlgProc_Picture(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Mit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Video(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Cali(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Dataset(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_InterfaceCfg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DlgProc_Audit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void DoSelectFolder(HWND hWnd);
void StopFolderMonitor();
BOOL StartFolderMonitor(HWND hDlg);
BOOL IsImageFile(LPCWSTR szExt);
void UpdateSingleItemStatus(HWND hDlg, LPCWSTR szBaseName, BOOL bExist);
int IDCForDpi(HWND hWnd, int oldIDC);
extern int selectedIndex;

extern wchar_t szFolderPath[MAX_PATH];
extern HWND hPagePicture, hPageAbout, hPageMit, hPageCali, hPageVideo, hPageProcess, hPageDataset, hPageExportCfg, hPageInterfaceCfg;