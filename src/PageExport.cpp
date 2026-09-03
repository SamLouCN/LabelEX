#ifndef UNICODE
#define UNICODE
#endif

#include "main.h"

INT_PTR CALLBACK DlgProc_Dataset(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"导出数据集");
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
		UINT secondColumnLeft = 22 * margin;
		UINT thirdColumnLeft = 32 * margin;
		UINT firstRowTop = 2 * margin;
		UINT secondRowTop = 5 * margin;
		UINT thirdRowTop = 8 * margin;
		UINT fourthRowTop = 11 * margin;
		UINT fifthRowTop = 14 * margin;
		UINT sixthRowTop = 17 * margin;
		UINT seventhRowTop = 20 * margin;
		UINT eighthRowTop = 23 * margin;
		UINT ninthRowTop = 26 * margin;
		UINT tenthRowTop = 29 * margin;

		HFONT hFont = CreateFont(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Microsoft Yahei UI"));

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_CONFIG), NULL, firstColumnLeft, firstRowTop, rcDlg.right - rcDlg.left - 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_DIR), NULL, firstColumnLeft, secondRowTop, 49 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_SELECT_DATASET), NULL, 51 * margin + 2 * minLen, secondRowTop, 5 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_TRAIN_PERCENT), NULL, firstColumnLeft, thirdRowTop + 3 * minLen, 9 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_TRAIN_PERCENT), NULL, firstColumnLeft + 10 * margin, thirdRowTop + 1 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_YAML), NULL, firstColumnLeft, fourthRowTop - 2 * minLen, 15 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_1), NULL, firstColumnLeft, fifthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_2), NULL, firstColumnLeft, sixthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_3), NULL, firstColumnLeft, seventhRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_4), NULL, firstColumnLeft, eighthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_5), NULL, firstColumnLeft, ninthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_6), NULL, secondColumnLeft, fifthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_7), NULL, secondColumnLeft, sixthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_8), NULL, secondColumnLeft, seventhRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_9), NULL, secondColumnLeft, eighthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_10), NULL, secondColumnLeft, ninthRowTop, 3 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_1), NULL, firstColumnLeft + 3 * margin, fifthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_2), NULL, firstColumnLeft + 3 * margin, sixthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_3), NULL, firstColumnLeft + 3 * margin, seventhRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_4), NULL, firstColumnLeft + 3 * margin, eighthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_5), NULL, firstColumnLeft + 3 * margin, ninthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_6), NULL, secondColumnLeft + 3 * margin, fifthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_7), NULL, secondColumnLeft + 3 * margin, sixthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_8), NULL, secondColumnLeft + 3 * margin, seventhRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_9), NULL, secondColumnLeft + 3 * margin, eighthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_NAME_10), NULL, secondColumnLeft + 3 * margin, ninthRowTop - 2 * minLen, 14 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);

		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT), NULL, rcDlg.right - 9 * margin, tenthRowTop, 7 * margin, 3 * margin, SWP_NOZORDER);
		SendMessage(GetDlgItem(hDlg, IDC_ST_EXPORT_CONFIG), WM_SETFONT, (WPARAM)hFont, TRUE);
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