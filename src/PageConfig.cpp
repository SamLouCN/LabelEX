#ifndef UNICODE
#define UNICODE
#endif

#include "main.h"

IniFile ini(L".\\config.ini");

INT_PTR CALLBACK DlgProc_InterfaceCfg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"界面设置");
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"1");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"2");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"3");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"4");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"5");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"6");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"7");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"8");
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_SETCURSEL, ini.GetInt(L"Interface", L"box_width", 4) - 1, 0);
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"2");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"4");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"6");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"8");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"10");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"12");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"14");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_SETCURSEL, ini.GetInt(L"Interface", L"handle_width", 8)/2 - 1, 0);
		SendMessage(GetDlgItem(hDlg, IDC_THEME), CB_ADDSTRING, 0, (LPARAM)L"跟随系统");
		SendMessage(GetDlgItem(hDlg, IDC_THEME), CB_ADDSTRING, 0, (LPARAM)L"浅色");
		SendMessage(GetDlgItem(hDlg, IDC_THEME), CB_ADDSTRING, 0, (LPARAM)L"深色");
		SendMessage(GetDlgItem(hDlg, IDC_THEME), CB_SETCURSEL, 0, 0);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		dmlib::setDarkWndNotifySafeEx(hDlg, true, true);
		return TRUE;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		UINT fontHeight = IDCForDpi(hDlg, 20);

		UINT firstColumnLeft = 1 * margin;
		UINT secondColumnLeft = 17 * margin;
		UINT thirdColumnLeft = 32 * margin;
		UINT firstRowTop = 1 * margin;
		UINT secondRowTop = 4 * margin;
		UINT thirdRowTop = 7 * margin;
		UINT fourthRowTop = 10 * margin;
		UINT fifthRowTop = 12 * margin;
		UINT sixthRowTop = 16 * margin;
		UINT seventhRowTop = 20 * margin;
		UINT eighthRowTop = 24 * margin;

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_NOTICE), NULL, firstColumnLeft, firstRowTop, 20 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_THEME), NULL, firstColumnLeft, secondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_THEME), NULL, secondColumnLeft + 4 * margin, secondRowTop, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_RECT_WIDTH), NULL, firstColumnLeft, thirdRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_RECT_WIDTH), NULL, secondColumnLeft + 4 * margin, thirdRowTop, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_HANDLE_WIDTH), NULL, firstColumnLeft, fourthRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), NULL, secondColumnLeft + 4 * margin, fourthRowTop, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_OK), NULL, rcDlg.right - 8 * margin, rcDlg.bottom - 3 * margin - 3 * minLen, 7 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		return TRUE;
	}
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case IDC_OK:
		{
			int Darkmode = SendMessage(GetDlgItem(hDlg, IDC_THEME), CB_GETCURSEL, 0, 0);
			int threshold = GetDlgItemInt(hDlg, IDC_RECT_WIDTH, NULL, FALSE);
			int thresholdHandle = GetDlgItemInt(hDlg, IDC_HANDLE_WIDTH, NULL, FALSE);
			ini.SetInt(L"Interface", L"theme_mode", Darkmode);
			ini.SetInt(L"Interface", L"box_width", threshold);
			ini.SetInt(L"Interface", L"handle_width", thresholdHandle);
			DestroyWindow(hDlg);
			return TRUE;
		}
		}
		return FALSE;
	}
	case WM_CLOSE:
	{
		DestroyWindow(hDlg);
		return TRUE;
	}
	}
	return FALSE;
}