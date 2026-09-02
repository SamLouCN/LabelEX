#ifndef UNICODE
#define UNICODE
#endif

#include "main.h"

INT_PTR CALLBACK DlgProc_About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"°æ±¾");
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
		SetWindowPos(GetDlgItem(hDlg, IDC_PICTURE_SL), NULL, rcDlg.left + margin, rcDlg.top + margin, rcDlg.right - rcDlg.left - 2 * margin, 8 * margin + 5 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_NAME), NULL, rcDlg.left + margin, rcDlg.top + 10 * margin, rcDlg.right - rcDlg.left - 2 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_VERSION), NULL, rcDlg.left + margin, rcDlg.top + 12 * margin, rcDlg.right - rcDlg.left - 2 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_COPYRIGHT), NULL, rcDlg.left + margin, rcDlg.top + 14 * margin, rcDlg.right - rcDlg.left - 2 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_ANNOUNCE), NULL, rcDlg.left + margin, rcDlg.top + 16 * margin, rcDlg.right - rcDlg.left - 2 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_OK), NULL, (rcDlg.right - rcDlg.left) / 2 - 5 * margin, rcDlg.top + 18 * margin, 10 * margin, 3 * margin, SWP_NOZORDER);
	}
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case IDC_OK:
			ShowWindow(hDlg, SW_HIDE);
			return TRUE;
		}
		return TRUE;
	}
	case WM_CLOSE:
	{
		ShowWindow(hDlg, SW_HIDE);
		return TRUE;
	}
	}
	return FALSE;
}

INT_PTR CALLBACK DlgProc_Mit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"MIT License");
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		LPCWSTR licenseText = L"MIT License\r\n\r\n"
			L"Copyright (c) 2026 SamLouCN\r\n\r\n"
			L"Permission is hereby granted, free of charge, to any person obtaining a copy "
			L"of this software and associated documentation files (the \"Software\"), to deal "
			L"in the Software without restriction, including without limitation the rights "
			L"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell "
			L"copies of the Software, and to permit persons to whom the Software is "
			L"furnished to do so, subject to the following conditions: \r\n\r\n"
			L"The above copyright notice and this permission notice shall be included in all "
			L"copies or substantial portions of the Software. \r\n\r\n"
			L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
			L"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
			L"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE "
			L"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
			L"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
			L"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
			L"SOFTWARE.";
		SetDlgItemText(hDlg, IDC_LICENSE_TEXT, licenseText);
		PostMessage(hDlg, WM_SIZE, 0, 0);

		return TRUE;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		SetWindowPos(GetDlgItem(hDlg, IDC_LICENSE_TEXT), NULL, rcDlg.left + margin, rcDlg.top + margin, rcDlg.right - rcDlg.left - 2 * margin, rcDlg.bottom - rcDlg.top - 2 * margin, SWP_NOZORDER);
	}
	case WM_CLOSE:
	{
		ShowWindow(hDlg, SW_HIDE);
		return TRUE;
	}
	}
	return FALSE;
}