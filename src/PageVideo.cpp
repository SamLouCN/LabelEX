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

		HFONT hFont = CreateFont(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Microsoft Yahei UI"));

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE), NULL, firstColumnLeft, firstRowTop, rcDlg.right - rcDlg.left - 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE), NULL, firstColumnLeft, secondRowTop, 49 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SELECT_SOURCE), NULL, 51 * margin + 2 * minLen, secondRowTop, 5 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_INFO), NULL, firstColumnLeft, thirdRowTop, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_FORM), NULL, firstColumnLeft, fourthRowTop, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE_FORM), NULL, firstColumnLeft + 5 * margin, fourthRowTop - 2 * minLen, 10 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_RES), NULL, secondColumnLeft + 1 * margin, fourthRowTop, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE_RES), NULL, secondColumnLeft + 7 * margin, fourthRowTop - 2 * minLen, 12 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_SOURCE_FPS), NULL, thirdColumnLeft + 5 * margin, fourthRowTop, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SOURCE_FPS), NULL, thirdColumnLeft + 10 * margin, fourthRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_DIR), NULL, firstColumnLeft, fifthRowTop, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_DIR), NULL, firstColumnLeft, sixthRowTop, 49 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SELECT_EXPORT), NULL, 51 * margin + 2 * minLen, sixthRowTop, 5 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_FORM), NULL, firstColumnLeft, seventhRowTop, 7 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_FORM), NULL, firstColumnLeft + 7 *margin, seventhRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_QUALITY), NULL, secondColumnLeft, seventhRowTop, 7 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_QUALITY), NULL, secondColumnLeft + 7 * margin, seventhRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_FPS), NULL, thirdColumnLeft, seventhRowTop, 11 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_FPS), NULL, thirdColumnLeft + 11 * margin, seventhRowTop - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT), NULL, rcDlg.right - 9 * margin, eighthRowTop, 7 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_CALI), NULL, firstColumnLeft, eighthRowTop, 11 * margin, 3 * margin, SWP_NOZORDER);

		SendMessage(GetDlgItem(hDlg, IDC_ST_SOURCE), WM_SETFONT, (WPARAM)hFont, TRUE);
		SendMessage(GetDlgItem(hDlg, IDC_ST_SOURCE_INFO), WM_SETFONT, (WPARAM)hFont, TRUE);
		SendMessage(GetDlgItem(hDlg, IDC_ST_EXPORT_DIR), WM_SETFONT, (WPARAM)hFont, TRUE);
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