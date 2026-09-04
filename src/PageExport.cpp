#ifndef UNICODE
#define UNICODE
#endif

#define WM_USER_UPDATE_PROGRESS (WM_USER + 403)
#define WM_USER_BUILD_DONE (WM_USER + 404)

#include "main.h"

struct SplitParams
{
	std::vector<std::wstring> imgPaths;
	std::vector<std::wstring> labelPaths;
	std::wstring trainImgPath;
	std::wstring valImgPath;
	std::wstring trainLabelPath;
	std::wstring valLabelPath;
	double ratio;
	HWND hDlg;
	BOOL bType;
};

HWND hDsProcessDlg;

DWORD WINAPI BuildDataset(LPVOID lpParam)
{
	SplitParams* params = (SplitParams*)lpParam;

	int total = (int)params->imgPaths.size();
	if (total == 0)
	{
		delete params;
		return 0;
	}

	std::vector<int> indices(total);
	for (int i = 0; i < total; i++) indices[i] = i;

	std::random_device rd;
	std::mt19937 gen(rd());
	int k = (int)(total * params->ratio);
	if (k > total) k = total;

	for (int i = 0; i < k; ++i)
	{
		std::uniform_int_distribution<> dis(i, total - 1);
		std::swap(indices[i], indices[dis(gen)]);
	}

	SHCreateDirectoryExW(NULL, params->trainImgPath.c_str(), NULL);
	if (params->bType == 1) 
	{
		SHCreateDirectoryExW(NULL, params->valImgPath.c_str(), NULL);
		SHCreateDirectoryExW(NULL, params->trainLabelPath.c_str(), NULL);
		SHCreateDirectoryExW(NULL, params->valLabelPath.c_str(), NULL);
	}

	for (int i = 0; i < total; ++i)
	{
		const std::wstring& srcImg = params->imgPaths[indices[i]];
		LPCWSTR imgName = PathFindFileName(srcImg.c_str());

		std::wstring destImg;
		if (i < k)
		{
			destImg = params->trainImgPath + L"\\" + imgName;
		}
		else
		{
			if (params->bType == 1)
			{
				destImg = params->valImgPath + L"\\" + imgName;
			}
		}

		if (params->bType == 0)
		{
			MoveFileExW(srcImg.c_str(), destImg.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED);
		}
		else
		{
			const std::wstring& srcLabel = params->labelPaths[indices[i]];
			LPCWSTR labelName = PathFindFileName(srcLabel.c_str());

			std::wstring destLabel;
			if (i < k)
			{
				destLabel = params->trainLabelPath + L"\\" + labelName;
			}
			else
			{
				destLabel = params->valLabelPath + L"\\" + labelName;
			}
			CopyFileW(srcImg.c_str(), destImg.c_str(), FALSE);
			CopyFileW(srcLabel.c_str(), destLabel.c_str(), FALSE);
		}

		int progress = (int)(((double)(i + 1) / total) * 100);
		PostMessage(params->hDlg, WM_USER_UPDATE_PROGRESS, progress, 0);
	}
	PostMessage(params->hDlg, WM_USER_BUILD_DONE, 0, 0);
	delete params;
	return 0;
}

INT_PTR CALLBACK DlgProc_DatasetProcess(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		hDsProcessDlg = hDlg;
		int baseWidth = 280;
		int baseHeight = 100;
		int scaledWidth = IDCForDpi(hDlg, baseWidth);
		int scaledHeight = IDCForDpi(hDlg, baseHeight);
		RECT rcParent;
		HWND hParent = GetParent(hDlg);
		hParent && GetWindowRect(hParent, &rcParent);
		int x = rcParent.left + (rcParent.right - rcParent.left - scaledWidth) / 2;
		int y = rcParent.top + (rcParent.bottom - rcParent.top - scaledHeight) / 2;
		SetWindowPos(hDlg, NULL, x, y, scaledWidth, scaledHeight, SWP_NOZORDER);
		SplitParams* params = (SplitParams*)lParam;
		params->hDlg = hDlg;
		if (!params)
		{
			EndDialog(hDlg, IDOK);
			return TRUE;
		}
		HANDLE hThread = CreateThread(NULL, 0, BuildDataset, params, 0, NULL);
		if (hThread)
		{
			CloseHandle(hThread);
		}
		return TRUE;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		SetWindowPos(GetDlgItem(hDlg, IDC_PROGRESS), NULL, rcDlg.left + 2 * margin, rcDlg.top + 2 * margin, rcDlg.right - rcDlg.left - 4 * margin, rcDlg.bottom - rcDlg.top - 4 * margin, SWP_NOZORDER);
		return TRUE;
	}
	case WM_USER_UPDATE_PROGRESS:
	{
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, wParam, 0);
		return TRUE;
	}
	case WM_USER_BUILD_DONE:
	{
		EndDialog(hDlg, IDOK);
		return TRUE;
	}
	}
	return FALSE;
}

INT_PTR CALLBACK DlgProc_Cali(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"导出校验集");
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		SetDlgItemInt(hDlg, IDC_CALI_PERCENT, 10, FALSE);
		return TRUE;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		UINT fontHeight = IDCForDpi(hDlg, 19);

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_CALI_DIR), NULL, 2 * margin, 2 * margin, rcDlg.right - rcDlg.left - 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_CALI_PERCENT), NULL, 2 * margin, 5 * margin, 8 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_CALI_PERCENT), NULL, 11 * margin, 5 * margin - 2 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT), NULL, rcDlg.right - 8 * margin, rcDlg.bottom - 5 * margin, 6 * margin, 3 * margin, SWP_NOZORDER);

		return TRUE;
	}
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case IDC_EXPORT:
		{
			HWND hList = GetDlgItem(hPagePicture, IDC_LISTVIEW);
			int listCount = ListView_GetItemCount(hList);
			if (listCount == 0) { MessageBox(hDlg, L"Nothing in the list", L"error", MB_OK); return TRUE; }

			SplitParams* params = new SplitParams();
			params->bType = 0;
			int ratioInt = GetDlgItemInt(hDlg, IDC_CALI_PERCENT, NULL, FALSE);
			params->ratio = (ratioInt > 0 && ratioInt <= 100) ? (double)ratioInt / 100.0 : 0.1;

			wchar_t szName[MAX_PATH];
			for (int i = 0; i < listCount; ++i)
			{
				ListView_GetItemText(hList, i, 1, szName, MAX_PATH);
				std::wstring fullPath = std::wstring(szFolderPath) + L"\\" + szName;
				params->imgPaths.push_back(fullPath);
			}

			wchar_t trainImgPath[MAX_PATH];
			StringCchPrintf(trainImgPath, _countof(trainImgPath), L"%s\\Calibration", szFolderPath);
			params->trainImgPath = trainImgPath;
		
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGEVIDEOPROGRESS), hDlg, DlgProc_DatasetProcess, (LPARAM)params);
			return TRUE;
		}
		}
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
		UINT fontHeight = IDCForDpi(hDlg, 19);

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
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Microsoft Yahei UI"));

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_CONFIG), NULL, firstColumnLeft, firstRowTop + 2 * minLen, rcDlg.right - rcDlg.left - 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_DATASET_DIR), NULL, firstColumnLeft, secondRowTop + 2 * minLen, 49 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_TRAIN_PERCENT), NULL, firstColumnLeft, thirdRowTop + 3 * minLen, 9 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_TRAIN_PERCENT), NULL, firstColumnLeft + 10 * margin, thirdRowTop + 1 * minLen, 6 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_EXPORT_YAML), NULL, firstColumnLeft, fourthRowTop - 2 * minLen, 25 * margin, 3 * margin, SWP_NOZORDER);
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
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		switch (WM_ID)
		{
		case IDC_EXPORT:
		{
			HWND hList = GetDlgItem(hPagePicture, IDC_LISTVIEW);
			int listCount = ListView_GetItemCount(hList);
			if (listCount == 0) { MessageBox(hDlg, L"Nothing in the list", L"error", MB_OK); return TRUE; }

			SplitParams* params = new SplitParams();
			params->bType = 1;
			int ratioInt = GetDlgItemInt(hDlg, IDC_ST_TRAIN_PERCENT, NULL, FALSE);
			params->ratio = (ratioInt > 0 && ratioInt <= 100) ? (double)ratioInt / 100.0 : 0.8;

			wchar_t szName[MAX_PATH];
			for (int i = 0; i < listCount; ++i)
			{
				ListView_GetItemText(hList, i, 1, szName, MAX_PATH);
				std::wstring fullPath = std::wstring(szFolderPath) + L"\\" + szName;

				params->imgPaths.push_back(fullPath);

				std::wstring baseName = szName;
				size_t dotPos = baseName.find_last_of(L'.');
				if (dotPos != std::wstring::npos)
				{
					baseName = baseName.substr(0, dotPos);
				}
				params->labelPaths.push_back(std::wstring(szFolderPath) + L"\\" + baseName + L".txt");
			}

			for (int i = 0; i < listCount; ++i)
			{
				if (PathFileExists(params->labelPaths[i].c_str()) == FALSE)
				{
					MessageBox(hDlg, L"Photos without any label were detected", L"error", MB_ICONERROR);
					delete params;
					return TRUE;
				}
			}

			wchar_t trainImgPath[MAX_PATH];
			StringCchPrintf(trainImgPath, _countof(trainImgPath), L"%s\\dataset\\images\\train", szFolderPath);
			params->trainImgPath = trainImgPath;
			wchar_t valImgPath[MAX_PATH];
			StringCchPrintf(valImgPath, _countof(valImgPath), L"%s\\dataset\\images\\val", szFolderPath);
			params->valImgPath = valImgPath;
			wchar_t trainLabelPath[MAX_PATH];
			StringCchPrintf(trainLabelPath, _countof(trainLabelPath), L"%s\\dataset\\labels\\train", szFolderPath);
			params->trainLabelPath = trainLabelPath;
			wchar_t valLabelPath[MAX_PATH];
			StringCchPrintf(valLabelPath, _countof(valLabelPath), L"%s\\dataset\\labels\\val", szFolderPath);
			params->valLabelPath = valLabelPath;

			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGEVIDEOPROGRESS), hDlg, DlgProc_DatasetProcess, (LPARAM)params);
			return TRUE;
		}
		}
		return TRUE;
	}
	}
	return FALSE;
}