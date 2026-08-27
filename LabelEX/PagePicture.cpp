#ifndef UNICODE
#define UNICODE
#endif
#define IDC_LISTVIEW 5001
#define WM_USER_REFRESH_LIST (WM_USER + 100)
#define WM_USER_UPDATE_ITEM (WM_USER + 101)

#include "main.h"

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
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		5, 5, 5, 5,
		hWnd, (HMENU)IDC_LISTVIEW, GetModuleHandle(NULL), NULL
	);
	if (hList == NULL)
	{
		MessageBox(NULL, L"Failed to create ListView", L"Error", NULL);
		return FALSE;
	}

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

INT_PTR CALLBACK DlgProc_Picture(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		DoCreateListView(hDlg);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		return 0;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT listViewWidth = IDCForDpi(hDlg, 280);
		SetWindowPos(GetDlgItem(hDlg, IDC_LISTVIEW), NULL, margin, margin, listViewWidth, rcDlg.bottom - rcDlg.top - 2 * margin, SWP_NOZORDER);

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
	}
	return FALSE;
}

