#ifndef UNICODE
#define UNICODE
#endif
#define IDC_LISTVIEW 5001
#define WM_USER_REFRESH_LIST (WM_USER + 100)
#define WM_USER_UPDATE_ITEM (WM_USER + 101)

#include "main.h"

using namespace Gdiplus;

Bitmap* pCurrentImage = nullptr;
HWND hImageCtrl = nullptr;

void DoSelectFolder(HWND hWnd);
void RefreshList(HWND hWnd);
BOOL IsImageFile(LPCWSTR szExt);
BOOL DoCreateListView(HWND hWnd);

wchar_t szFolderPath[MAX_PATH] = { 0 };

void DoSelectFolder(HWND hWnd)
{
	CoInitialize(NULL);
	IFileOpenDialog *pDialog = NULL;
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
		IID_IFileOpenDialog, (void**)&pDialog
	);
	if (SUCCEEDED(hr))
	{
		DWORD opts;
		pDialog->GetOptions(&opts);
		pDialog->SetOptions(opts | FOS_PICKFOLDERS);
		hr = pDialog->Show(hWnd);
		if (SUCCEEDED(hr))
		{
			IShellItem *pItem = NULL;
			pDialog->GetResult(&pItem);
			if (pItem)
			{
				PWSTR pszPath = NULL;
				pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszPath);
				if (pszPath)
				{
					wcscpy_s(szFolderPath, pszPath);
					CoTaskMemFree(pszPath);
					StartFolderMonitor(hPagePicture);
					RefreshList(hWnd);
				}
				pItem->Release();
			}
		}
		pDialog->Release();
	}
	CoUninitialize();
}

void RefreshList(HWND hWnd)
{
	HWND hList = GetDlgItem(hWnd, IDC_LISTVIEW);
	if (!hList)
	{
		return;
	}
	if (szFolderPath[0] == 0)
	{
		return;
	}

	ListView_DeleteAllItems(hList);

	wchar_t szSearch[MAX_PATH];
	StringCchPrintf(szSearch, _countof(szSearch), L"%s\\*.*", szFolderPath);

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(szSearch, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}
		wchar_t *pExt = PathFindExtension(fd.cFileName);
		if (!pExt)
		{
			continue;
		}
		if (!IsImageFile(pExt))
		{
			continue;
		}

		wchar_t szBaseName[MAX_PATH];
		StringCchCopy(szBaseName, _countof(szBaseName), fd.cFileName);
		PathRemoveExtension(szBaseName);

		wchar_t szTxtPath[MAX_PATH];
		StringCchPrintf(szTxtPath, _countof(szTxtPath), L"%s\\%s.txt", szFolderPath, szBaseName);

		BOOL bTxtExists = PathFileExists(szTxtPath);

		LVITEM item = { 0 };
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.pszText = (LPWSTR)L"";
		item.iItem = ListView_GetItemCount(hList);
		item.lParam = (LPARAM)bTxtExists;
		int nIndex = ListView_InsertItem(hList, &item);
		if (nIndex == -1)
		{
			continue;
		}

		ListView_SetItemText(hList, nIndex, 0, (LPWSTR)(bTxtExists ? L"\u2714" : L"    \u2716"));
		ListView_SetItemText(hList, nIndex, 1, fd.cFileName);
		
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);
}

void UpdateSingleItemStatus(HWND hDlg, LPCWSTR szBaseName, BOOL bExist)
{
	HWND hList = GetDlgItem(hPagePicture, IDC_LISTVIEW);
	
	int nCount = ListView_GetItemCount(hList);
	wchar_t szCurrentName[MAX_PATH];
	wchar_t szCurrentBase[MAX_PATH];

	for (int i = 0; i < nCount; i++)
	{
		ListView_GetItemText(hList, i, 1, szCurrentName, MAX_PATH);

		StringCchCopy(szCurrentBase, _countof(szCurrentBase), szCurrentName);
		PathRemoveExtension(szCurrentBase);

		if (_wcsicmp(szCurrentBase, szBaseName) == 0)
		{
			ListView_SetItemText(hList, i, 0, (LPWSTR)(bExist ? L"\u2714" : L"    \u2716"));
		}
	}
}

BOOL IsImageFile(LPCWSTR szExt)
{
	return (_wcsicmp(szExt, L".jpg") == 0 ||
		_wcsicmp(szExt, L".jpeg") == 0 ||
		_wcsicmp(szExt, L".png") == 0 ||
		_wcsicmp(szExt, L".bmp") == 0 ||
		_wcsicmp(szExt, L".gif") == 0 ||
		_wcsicmp(szExt, L".tiff") == 0
	);
}

BOOL DoCreateListView(HWND hWnd)
{
	HWND hList = CreateWindowEx(0, WC_LISTVIEW, L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		5, 5, 5, 5,
		hWnd, (HMENU)IDC_LISTVIEW, GetModuleHandle(NULL), NULL
	);
	if (hList == NULL)
	{
		MessageBox(NULL, L"Failed to create ListView", L"Error", NULL);
		return FALSE;
	}

	ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

	LVCOLUMN col = { 0 };
	col.mask = LVCF_TEXT | LVCF_WIDTH;

	col.pszText = (LPWSTR)L"状态";
	col.cx = IDCForDpi(hList, 50);
	ListView_InsertColumn(hList, 0 ,&col);

	col.pszText = (LPWSTR)L"文件名";
	col.cx = IDCForDpi(hList, 230);
	ListView_InsertColumn(hList, 1, &col);

	return TRUE;
}

void LoadImageToDisplay(LPCWSTR szFilePath)
{
	if (pCurrentImage)
	{
		delete pCurrentImage;
		pCurrentImage = nullptr;
	}

	pCurrentImage = Bitmap::FromFile(szFilePath);
	if (pCurrentImage && pCurrentImage->GetLastStatus() != Ok)
	{
		delete pCurrentImage;
		pCurrentImage = nullptr;
	}

	if (hImageCtrl)
	{
		InvalidateRect(hImageCtrl, NULL, TRUE);
	}
}

void SelectImageByIndex(HWND hList, int index)
{
	int itemCount = ListView_GetItemCount(hList);
	if (index < 0 || index >= itemCount) {
		return;
	}
	ListView_SetItemState(hList, -1, 0, LVIS_SELECTED);
	ListView_SetItemState(hList, index, LVIS_SELECTED, LVIS_SELECTED);
	ListView_EnsureVisible(hList, index, FALSE);
}

void SelectNextImage(HWND hList)
{
	int itemCount = ListView_GetItemCount(hList);
	if (itemCount == 0)
	{
		return;
	}

	int currentSel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
	int nextIndex;
	if (currentSel == -1)
	{
		nextIndex = 0;
	}
	else
	{
		nextIndex = currentSel + 1;
		if (nextIndex >= itemCount)
		{
			nextIndex = 0;
		}
	}
	SelectImageByIndex(hList, nextIndex);
}

INT_PTR CALLBACK DlgProc_Picture(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		hImageCtrl = GetDlgItem(hDlg, IDC_PICTURE);
		DoCreateListView(hDlg);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		SendMessage(GetDlgItem(hDlg, IDC_ENABLE_EDIT), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(hDlg, IDC_NAME_1), BM_SETCHECK, BST_CHECKED, 0);
		return 0;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		UINT listViewWidth = IDCForDpi(hDlg, 280);

		UINT workSpaceLeft = margin * 2 + listViewWidth;
		UINT workSpaceTop = rcDlg.bottom - rcDlg.top - 16 * margin;
		UINT wsSecondRowTop = workSpaceTop + 4 * margin;
		UINT wsThirdRowTop = workSpaceTop + 8 * margin;
		UINT wsFourthRowTop = workSpaceTop + 12 * margin;

		SetWindowPos(GetDlgItem(hDlg, IDC_LISTVIEW), NULL, margin, margin, listViewWidth, rcDlg.bottom - rcDlg.top - 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_PICTURE), NULL, workSpaceLeft, margin, rcDlg.right - rcDlg.left - 3 * margin - listViewWidth, rcDlg.bottom - rcDlg.top - 18 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_OK), NULL, rcDlg.right - rcDlg.left - 12 * margin, wsFourthRowTop, 11 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ENABLE_EDIT), NULL, workSpaceLeft, workSpaceTop + margin, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_1), NULL, workSpaceLeft + minLen, wsSecondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_2), NULL, workSpaceLeft + minLen + 11 * margin, wsSecondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_3), NULL, workSpaceLeft + minLen + 22 * margin, wsSecondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_4), NULL, workSpaceLeft + minLen + 33 * margin, wsSecondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_5), NULL, workSpaceLeft + minLen + 44 * margin, wsSecondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_6), NULL, workSpaceLeft + minLen + 55 * margin, wsSecondRowTop, 10 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_1), NULL, workSpaceLeft + minLen, wsThirdRowTop, 10 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_2), NULL, workSpaceLeft + minLen + 11 * margin, wsThirdRowTop, 10 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_3), NULL, workSpaceLeft + minLen + 22 * margin, wsThirdRowTop, 10 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_4), NULL, workSpaceLeft + minLen + 33 * margin, wsThirdRowTop, 10 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_5), NULL, workSpaceLeft + minLen + 44 * margin, wsThirdRowTop, 10 * margin, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_6), NULL, workSpaceLeft + minLen + 55 * margin, wsThirdRowTop, 10 * margin, 2 * margin, SWP_NOZORDER);

		return 0;
	}
	case WM_USER_REFRESH_LIST:
	{
		RefreshList(hDlg);
		return 0;
	}
	case WM_USER_UPDATE_ITEM:
	{
		LPWSTR szBaseName = (LPWSTR)wParam;
		BOOL bExist = (BOOL)lParam;
		UpdateSingleItemStatus(hDlg, szBaseName, bExist);
		free(szBaseName);
		return 0;
	}
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case IDC_OK:
			SelectNextImage(GetDlgItem(hDlg, IDC_LISTVIEW));
			return 0;
		}
	}
	case WM_NOTIFY:
	{
		NMHDR* pnmh = (NMHDR*)lParam;
		if (pnmh->idFrom == IDC_LISTVIEW && pnmh->code == LVN_ITEMCHANGED)
		{
			NMLISTVIEW* pnmv = (NMLISTVIEW*)lParam;
			if ((pnmv->uChanged & LVIF_STATE) && (pnmv->uNewState & LVIS_SELECTED))
			{
				wchar_t szFileName[MAX_PATH];
				ListView_GetItemText(pnmh->hwndFrom, pnmv->iItem, 1, szFileName, MAX_PATH);
				wchar_t szFullPath[MAX_PATH];
				StringCchPrintf(szFullPath, _countof(szFullPath), L"%s\\%s", szFolderPath, szFileName);
				LoadImageToDisplay(szFullPath);
			}
		}
		return 0;
	}
	case WM_DRAWITEM:
	{
		LPDRAWITEMSTRUCT lpDIS = (LPDRAWITEMSTRUCT)lParam;
		if (lpDIS->CtlID == IDC_COLOR_1) {
			HBRUSH hBrush = CreateSolidBrush(RGB(255, 59, 48));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_2) {
			HBRUSH hBrush = CreateSolidBrush(RGB(255, 149, 0));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_3) {
			HBRUSH hBrush = CreateSolidBrush(RGB(255, 204, 0));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_4) {
			HBRUSH hBrush = CreateSolidBrush(RGB(52, 199, 89));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_5) {
			HBRUSH hBrush = CreateSolidBrush(RGB(0, 122, 255));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_6) {
			HBRUSH hBrush = CreateSolidBrush(RGB(175, 82, 222));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_PICTURE)
		{
			HDC hdc = lpDIS->hDC;
			RECT rc = lpDIS->rcItem;

			HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
			HFONT hFont = CreateFont(
				40, 0, 0, 0,
				FW_BOLD, FALSE, FALSE, FALSE,
				DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
				DEFAULT_PITCH | FF_DONTCARE, L"Microsoft Yahei UI"
			);
			HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
			FillRect(hdc, &rc, hBrush);
			DeleteObject(hBrush);

			if (pCurrentImage)
			{
				Graphics graphics(hdc);
				int imageWidth = pCurrentImage->GetWidth();
				int imageHeight = pCurrentImage->GetHeight();
				if (imageHeight > 0 && imageWidth > 0)
				{
					float ratio = min((float)(rc.right - rc.left) / imageWidth, (float)(rc.bottom - rc.top) / imageHeight);
					int drawWidth = (int)(imageWidth * ratio);
					int drawHeight = (int)(imageHeight * ratio);
					int x = rc.left + (rc.right - rc.left - drawWidth) / 2;
					int y = rc.top + (rc.bottom - rc.top - drawHeight) / 2;
					graphics.DrawImage(pCurrentImage, x, y, drawWidth, drawHeight);
				}
			}
			else
			{
				SetBkMode(hdc, TRANSPARENT);
				DrawText(hdc, L"从列表中单击选择一张图片", -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
			}
			SelectObject(hdc, hOldFont);
			DeleteObject(hFont);
			return TRUE;
		}
		break;
	}
	case WM_DESTROY:
	{
		if (pCurrentImage)
		{
			delete pCurrentImage;
			pCurrentImage = nullptr;
		}
	}
	}
	return FALSE;
}

