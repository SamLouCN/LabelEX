#ifndef UNICODE
#define UNICODE
#endif

#include "main.h"

INT_PTR CALLBACK DlgProc_InterfaceCfg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"ΩÁ√Ê…Ë÷√");
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
		SendMessage(GetDlgItem(hDlg, IDC_RECT_WIDTH), CB_SETCURSEL, 3, 0);
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"4");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"6");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"8");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"10");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"12");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_ADDSTRING, 0, (LPARAM)L"14");
		SendMessage(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), CB_SETCURSEL, 2, 0);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		return TRUE;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		UINT fontHeight = IDCForDpi(hDlg, 20);

		UINT firstColumnLeft = 2 * margin;
		UINT secondColumnLeft = 17 * margin;
		UINT thirdColumnLeft = 32 * margin;
		UINT firstRowTop = 2 * margin;
		UINT secondRowTop = 5 * margin;
		UINT thirdRowTop = 8 * margin;
		UINT fourthRowTop = 11 * margin;
		UINT fifthRowTop = 14 * margin;
		UINT sixthRowTop = 17 * margin;
		UINT seventhRowTop = 21 * margin;
		UINT eighthRowTop = 24 * margin;

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_NOTICE), NULL, firstColumnLeft, firstRowTop, 20 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_RECT_WIDTH), NULL, firstColumnLeft, secondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_RECT_WIDTH), NULL, secondColumnLeft + 4 * margin, secondRowTop, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_HANDLE_WIDTH), NULL, firstColumnLeft, thirdRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_HANDLE_WIDTH), NULL, secondColumnLeft + 4 * margin, thirdRowTop, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_OK), NULL, rcDlg.right - 9 * margin, sixthRowTop, 7 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		return TRUE;
	}
	case WM_COMMAND:
	{
		int WM_ID = HIWORD(wParam);
		switch (WM_ID)
		{
		case IDC_OK:
			return TRUE;
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