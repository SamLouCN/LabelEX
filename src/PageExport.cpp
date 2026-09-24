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
	std::wstring testImgPath;
	std::wstring trainLabelPath;
	std::wstring valLabelPath;
	std::wstring testLabelPath;
	double ratioTrain;
	double ratioVal;
	HWND hDlg;
	BOOL bType;
};

HWND hDsProcessDlg;
BOOL isChanged = FALSE;

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

	int kTrain = (int)(total * params->ratioTrain);
	if (kTrain < 0) kTrain = 0;
	if (kTrain > total) kTrain = total;

	int kVal = (int)(total * params->ratioVal);
	if (kVal < 0) kVal = 0;
	if (kTrain + kVal > total) kVal = total - kTrain;
	int kSplit = kTrain + kVal;

	for (int i = 0; i < kSplit; ++i)
	{
		std::uniform_int_distribution<> dis(i, total - 1);
		std::swap(indices[i], indices[dis(gen)]);
	}

	SHCreateDirectoryExW(NULL, params->trainImgPath.c_str(), NULL);
	if (params->bType == 1)
	{
		SHCreateDirectoryExW(NULL, params->valImgPath.c_str(), NULL);
		SHCreateDirectoryExW(NULL, params->testImgPath.c_str(), NULL);
		SHCreateDirectoryExW(NULL, params->trainLabelPath.c_str(), NULL);
		SHCreateDirectoryExW(NULL, params->valLabelPath.c_str(), NULL);
		SHCreateDirectoryExW(NULL, params->testLabelPath.c_str(), NULL);
	}

	for (int i = 0; i < total; ++i)
	{
		const std::wstring& srcImg = params->imgPaths[indices[i]];
		LPCWSTR imgName = PathFindFileName(srcImg.c_str());

		std::wstring destImg;
		std::wstring destLabel;

		if (i < kTrain)
		{
			destImg = params->trainImgPath + L"\\" + imgName;
			if (params->bType == 1)
			{
				LPCWSTR labelName = PathFindFileName(params->labelPaths[indices[i]].c_str());
				destLabel = params->trainLabelPath + L"\\" + labelName;
			}
		}
		else if (i < kSplit)
		{
			destImg = params->valImgPath + L"\\" + imgName;
			if (params->bType == 1)
			{
				LPCWSTR labelName = PathFindFileName(params->labelPaths[indices[i]].c_str());
				destLabel = params->valLabelPath + L"\\" + labelName;
			}
		}
		else
		{
			destImg = params->testImgPath + L"\\" + imgName;
			if (params->bType == 1)
			{
				LPCWSTR labelName = PathFindFileName(params->labelPaths[indices[i]].c_str());
				destLabel = params->testLabelPath + L"\\" + labelName;
			}
		}

		if (params->bType == 0)
		{
			if (i < kTrain)
			{
				CopyFileW(srcImg.c_str(), destImg.c_str(), FALSE);
			}
		}
		else
		{
			const std::wstring& srcLabel = params->labelPaths[indices[i]];
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

bool WriteYaml(const std::wstring& path, const std::wstring& content)
{
	FILE* f = nullptr;
	if (_wfopen_s(&f, path.c_str(), L"w, ccs=UTF-8") != 0 || !f)
	{
		return false;
	}
	fputws(content.c_str(), f);
	fclose(f);
	return true;
}

bool IsEditEmpty(HWND hEdit)
{
	wchar_t buf[50] = { 0 };
	GetWindowTextW(hEdit, buf, _countof(buf));
	for (const wchar_t* p = buf; *p; ++p)
	{
		if (!iswspace(*p))
		{
			return FALSE;
		}
	}
	return TRUE;
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
		if (!params)
		{
			EndDialog(hDlg, IDOK);
			return TRUE;
		}
		params->hDlg = hDlg;
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
		SetWindowText(hDlg, L"导出校准集");
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		SetDlgItemInt(hDlg, IDC_CALI_PERCENT, 2, FALSE);
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
			params->ratioTrain = (ratioInt > 0 && ratioInt <= 100) ? (double)ratioInt / 100.0 : 0.02;
			params->ratioVal = 0;

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
		
			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGEVIDEOPROGRESS), GetParent(hDlg), DlgProc_DatasetProcess, (LPARAM)params);
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
	static HFONT hFont;
	switch (message)
	{
	case WM_INITDIALOG:
	{
		SetWindowText(hDlg, L"导出数据集");
		HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
		SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		SetDlgItemInt(hDlg, IDC_TRAIN_PERCENT, 70, FALSE);
		SetDlgItemInt(hDlg, IDC_VAL_PERCENT, 20, FALSE);
		SetDlgItemInt(hDlg, IDC_ST_TEST_PERCENT, 10, FALSE);
		wchar_t nameBuffer[50];
		for (int i = 0; i <= 9; ++i)
		{
			GetDlgItemText(hPagePicture, IDC_NAMEEDIT_1 + i, nameBuffer, _countof(nameBuffer));
			SetDlgItemText(hDlg, IDC_EXPORT_NAME_1 + i, nameBuffer);
		}
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

		if (hFont)
		{
			DeleteObject(hFont);
			hFont = NULL;
		}

		hFont = CreateFont(fontHeight, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Microsoft Yahei UI"));

		SetWindowPos(GetDlgItem(hDlg, IDC_ST_EXPORT_CONFIG), NULL, firstColumnLeft, firstRowTop + 2 * minLen, rcDlg.right - rcDlg.left - 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_DATASET_DIR), NULL, firstColumnLeft, secondRowTop + 2 * minLen, 49 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_TRAIN_PERCENT), NULL, firstColumnLeft, thirdRowTop + 3 * minLen, 9 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_TRAIN_PERCENT), NULL, firstColumnLeft + 10 * margin, thirdRowTop + 1 * minLen, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_VAL_PERCENT), NULL, firstColumnLeft + 15 * margin, thirdRowTop + 1 * minLen, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_TEST_PERCENT), NULL, firstColumnLeft + 20 * margin, thirdRowTop + 3 * minLen, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
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
			int ratioTrain = GetDlgItemInt(hDlg, IDC_TRAIN_PERCENT, NULL, FALSE);
			params->ratioTrain = (ratioTrain > 0 && ratioTrain <= 100) ? (double)ratioTrain / 100.0 : 0.7;
			int ratioVal = GetDlgItemInt(hDlg, IDC_VAL_PERCENT, NULL, FALSE);
			params->ratioVal = (ratioVal > 0 && ratioVal <= 100) ? (double)ratioVal / 100.0 : 0.2;

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
			wchar_t testImgPath[MAX_PATH];
			StringCchPrintf(testImgPath, _countof(testImgPath), L"%s\\dataset\\images\\test", szFolderPath);
			params->testImgPath = testImgPath;
			wchar_t trainLabelPath[MAX_PATH];
			StringCchPrintf(trainLabelPath, _countof(trainLabelPath), L"%s\\dataset\\labels\\train", szFolderPath);
			params->trainLabelPath = trainLabelPath;
			wchar_t valLabelPath[MAX_PATH];
			StringCchPrintf(valLabelPath, _countof(valLabelPath), L"%s\\dataset\\labels\\val", szFolderPath);
			params->valLabelPath = valLabelPath;
			wchar_t testLabelPath[MAX_PATH];
			StringCchPrintf(testLabelPath, _countof(testLabelPath), L"%s\\dataset\\labels\\test", szFolderPath);
			params->testLabelPath = testLabelPath;

			if (SendMessage(GetDlgItem(hDlg, IDC_EXPORT_YAML), BM_GETCHECK, 0, 0) == 1)
			{
				std::wstring yaml;
				yaml += L"train: ./dataset/images/train\n";
				yaml += L"val: ./dataset/images/val\n";
				yaml += L"test: ./dataset/images/test\n";
				int totalNames = 0;
				wchar_t nc[256];
				wchar_t name[50];
				for (int i = 0; i <= 9; ++i)
				{
					if (!IsEditEmpty(GetDlgItem(hDlg, IDC_EXPORT_NAME_1 + i)))
					{
						totalNames ++;
					}
				}
				StringCchPrintfW(nc, _countof(nc), L"nc: %d\n", totalNames);
				yaml += nc;
				yaml += L"names: [";
				for (int i = 0; i < totalNames; ++i)
				{
					GetDlgItemText(hDlg, IDC_EXPORT_NAME_1 + i, name, _countof(name));
					yaml += L"'";
					yaml += name;
					if (i == totalNames - 1)
					{
						yaml += L"'";
					}
					else
					{
						yaml += L"', ";
					}
				}
				yaml += L"]";
				std::wstring outPath = std::wstring(szFolderPath) + L"\\data.yaml";
				WriteYaml(outPath, yaml);
			}

			DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGEVIDEOPROGRESS), GetParent(hDlg), DlgProc_DatasetProcess, (LPARAM)params);
			return TRUE;
		}
		case IDC_TRAIN_PERCENT:
		{
			if (HIWORD(wParam) == EN_CHANGE && isChanged == FALSE)
			{
				isChanged = TRUE;
				int ratioTrain = GetDlgItemInt(hDlg, IDC_TRAIN_PERCENT, NULL, FALSE);
				if (ratioTrain >= 98)
				{
					SetDlgItemInt(hDlg, IDC_TRAIN_PERCENT, 98, FALSE);
					ratioTrain = 98;
				}
				else if (ratioTrain <= 0)
				{
					SetDlgItemInt(hDlg, IDC_TRAIN_PERCENT, 1, FALSE);
					ratioTrain = 1;
				}
				int ratioVal = GetDlgItemInt(hDlg, IDC_VAL_PERCENT, NULL, FALSE);
				if (ratioVal + ratioTrain >= 100)
				{
					SetDlgItemInt(hDlg, IDC_VAL_PERCENT, 99 - ratioTrain, FALSE);
					ratioVal = 99 - ratioTrain;
				}
				int ratioTest = 100 - ratioTrain - ratioVal;
				SetDlgItemInt(hDlg, IDC_ST_TEST_PERCENT, ratioTest, FALSE);
				isChanged = FALSE;
			}
			return TRUE;
		}
		case IDC_VAL_PERCENT:
		{
			if (HIWORD(wParam) == EN_CHANGE && isChanged == FALSE)
			{
				isChanged = TRUE;
				int ratioVal = GetDlgItemInt(hDlg, IDC_VAL_PERCENT, NULL, FALSE);
				if (ratioVal >= 98)
				{
					SetDlgItemInt(hDlg, IDC_VAL_PERCENT, 98, FALSE);
					ratioVal = 98;
				}
				else if (ratioVal <= 0)
				{
					SetDlgItemInt(hDlg, IDC_VAL_PERCENT, 1, FALSE);
					ratioVal = 1;
				}
				int ratioTrain = GetDlgItemInt(hDlg, IDC_TRAIN_PERCENT, NULL, FALSE);
				if (ratioVal + ratioTrain >= 100)
				{
					SetDlgItemInt(hDlg, IDC_TRAIN_PERCENT, 99 - ratioVal, FALSE);
					ratioTrain = 99 - ratioVal;
				}
				int ratioTest = 100 - ratioTrain - ratioVal;
				SetDlgItemInt(hDlg, IDC_ST_TEST_PERCENT, ratioTest, FALSE);
				isChanged = FALSE;
			}
			return TRUE;
		}
		}
		return TRUE;
	}
	case WM_DESTROY:
	{
		if (hFont)
		{
			DeleteObject(hFont);
			hFont = NULL;
		}
		return TRUE;
	}
	}
	return FALSE;
}