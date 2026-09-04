#ifndef UNICODE
#define UNICODE
#endif
#define WM_USER_REFRESH_LIST (WM_USER + 100)
#define WM_USER_UPDATE_ITEM (WM_USER + 101)
#define WM_USER_STOP_MONITOR (WM_USER + 102)
#define WM_USER_START_MONITOR (WM_USER + 103)
#define WM_USER_UPDATE_LISTVIEW (WM_USER + 200)
#define WM_USER_UPDATE_PROGRESS (WM_USER + 201) 
#define WM_USER_STOP_MARQUEE (WM_USER + 301)
#define TIMER_REFRESH_DEBOUNCE 1001

#include "main.h"

using namespace Gdiplus;

struct BBox {
	int left, top, right, bottom;
	int classId;
};

struct ImageFileInfo {
	std::wstring fileName;
	BOOL status;
};

Bitmap* pCurrentImage = nullptr;
HWND hImageCtrl = nullptr;
HWND hProgressDlg = nullptr;
std::vector<BBox> bboxes;
int currentClassId = 0;
int selectedIndex = -1;
int threshold = 6;
WNDPROC oldPicProc = NULL;
enum DragMode {None, Moving, Resizing, Creating};
DragMode dragMode = None;
int resizeHandle = -1;
POINT dragStart;
POINT dragOffset;
wchar_t szFolderPath[MAX_PATH] = { 0 };
std::wstring currentImagePath;
BOOL isProcessExist = false;
BOOL isPendingRefresh = false;

void DoSelectFolder(HWND hWnd);
BOOL IsImageFile(LPCWSTR szExt);
BOOL DoCreateListView(HWND hWnd);
void LoadBBoxesFromFile(const std::wstring& filePath, int imgWidth, int imgHeight);

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
					PostMessage(hPagePicture, WM_USER_REFRESH_LIST, 0, 0);
				}
				pItem->Release();
			}
		}
		pDialog->Release();
	}
	CoUninitialize();
}

DWORD WINAPI RefreshListThread(LPVOID lpParam)
{
	HWND hDlg = (HWND)lpParam;
	HWND hList = GetDlgItem(hDlg, IDC_LISTVIEW);
	if (szFolderPath[0] == 0)
	{
		PostMessage(hProgressDlg, WM_USER_STOP_MARQUEE, 0, 0);
		return 0;
	}

	std::vector<ImageFileInfo> fileList;
	wchar_t szSearch[MAX_PATH];
	StringCchPrintf(szSearch, _countof(szSearch), L"%s\\*.*", szFolderPath);

	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(szSearch, &fd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		PostMessage(hProgressDlg, WM_USER_STOP_MARQUEE, 0, 0);
		return 0;
	}

	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			continue;
		}
		wchar_t *pExt = PathFindExtension(fd.cFileName);
		if (!IsImageFile(pExt))
		{
			continue;
		}

		wchar_t szBaseName[MAX_PATH];
		StringCchCopy(szBaseName, _countof(szBaseName), fd.cFileName);
		PathRemoveExtension(szBaseName);

		wchar_t szTxtPath[MAX_PATH];
		StringCchPrintf(szTxtPath, _countof(szTxtPath), L"%s\\%s.txt", szFolderPath, szBaseName);

		ImageFileInfo info;
		info.fileName = fd.cFileName;
		info.status = PathFileExists(szTxtPath);
		fileList.push_back(info);
	} while (FindNextFile(hFind, &fd));
	FindClose(hFind);

	std::vector<ImageFileInfo>* pData = new std::vector<ImageFileInfo>(std::move(fileList));
	PostMessage(hPagePicture, WM_USER_UPDATE_LISTVIEW, (WPARAM)pData, 0);
	return 0;
}

void RefreshListUI(HWND hList, const std::vector<ImageFileInfo>& fileList)
{
	SendMessage(hList, WM_SETREDRAW, FALSE, 0);
	ListView_DeleteAllItems(hList);

	for (size_t i = 0; i < fileList.size(); ++i)
	{
		const auto& info = fileList[i];
		LVITEM item = { 0 };
		item.mask = LVIF_TEXT | LVIF_PARAM;
		item.pszText = (LPWSTR)L"";
		item.iItem = ListView_GetItemCount(hList);
		item.lParam = (LPARAM)info.status;
		int nIndex = ListView_InsertItem(hList, &item);
		if (nIndex == -1)
		{
			continue;
		}
		ListView_SetItemText(hList, nIndex, 0, (LPWSTR)(info.status ? L"\u2714" : L"    \u2716"));
		ListView_SetItemText(hList, nIndex, 1, (LPWSTR)info.fileName.c_str());
		if ((i % 50) == 0)
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT) break;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	SendMessage(hList, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(hList, NULL, TRUE);
	UpdateWindow(hList);
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
	col.cx = IDCForDpi(hList, 170);
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

	if (szFilePath && *szFilePath)
	{
		currentImagePath = szFilePath;
	}
	else
	{
		currentImagePath.clear();
	}

	if (pCurrentImage)
	{
		std::wstring txtPath = currentImagePath;
		size_t dotPos = txtPath.find_last_of(L'.');
		if (dotPos != std::wstring::npos)
		{
			txtPath = txtPath.substr(0, dotPos) + L".txt";
		}
		else
		{
			txtPath += L".txt";
		}
		LoadBBoxesFromFile(txtPath, pCurrentImage->GetWidth(), pCurrentImage->GetHeight());
	}
	else
	{
		bboxes.clear();
		selectedIndex = -1;
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

POINT ControlToImage(POINT ptCtrl)
{
	POINT result = { 0, 0 };
	if (!pCurrentImage)
	{
		return result;
	}
	int imgWidth = pCurrentImage->GetWidth();
	int imgHeight = pCurrentImage->GetHeight();
	RECT rcClient;
	GetClientRect(GetDlgItem(hPagePicture, IDC_PICTURE), &rcClient);
	int ctrlWidth = rcClient.right - rcClient.left;
	int ctrlHeight = rcClient.bottom - rcClient.top;
	float ratio = min((float)ctrlWidth / imgWidth, (float)ctrlHeight / imgHeight);
	int drawWidth = (int)(imgWidth * ratio);
	int drawHeight = (int)(imgHeight * ratio);
	int offsetX = (ctrlWidth - drawWidth) / 2;
	int offsetY = (ctrlHeight - drawHeight) / 2;
	result.x = (int)((ptCtrl.x - offsetX) / ratio);
	result.y = (int)((ptCtrl.y - offsetY) / ratio);
	result.x = max(0, min(imgWidth - 1, result.x));
	result.y = max(0, min(imgHeight - 1, result.y));
	return result;
}

RECT ImageToControl(const BBox& box)
{
	RECT rc = { 0 };
	if (!pCurrentImage)
	{
		return rc;
	}
	int imgWidth = pCurrentImage->GetWidth();
	int imgHeight = pCurrentImage->GetHeight();
	RECT rcClient;
	GetClientRect(GetDlgItem(hPagePicture, IDC_PICTURE), &rcClient);
	int ctrlWidth = rcClient.right - rcClient.left;
	int ctrlHeight = rcClient.bottom - rcClient.top;
	float ratio = min((float)ctrlWidth / imgWidth, (float)ctrlHeight / imgHeight);
	int drawWidth = (int)(imgWidth * ratio);
	int drawHeight = (int)(imgHeight * ratio);
	int offsetX = (ctrlWidth - drawWidth) / 2;
	int offsetY = (ctrlHeight - drawHeight) / 2;
	rc.left = (int)(box.left * ratio) + offsetX;
	rc.top = (int)(box.top * ratio) + offsetY;
	rc.right = (int)(box.right * ratio) + offsetX;
	rc.bottom = (int)(box.bottom * ratio) + offsetY;
	return rc;
}

void DrawHandles(Graphics& graphics, const RECT& rc)
{
	int handleSize = 8;
	SolidBrush brush(Color(255, 255, 255, 255));
	Pen pen(Color(255, 0, 0, 0));
	POINT pts[8] = {
		{ rc.left, rc.top },
		{ (rc.left + rc.right) / 2, rc.top },
		{ rc.right, rc.top },
		{ rc.right, (rc.top + rc.bottom) / 2 },
		{ rc.right, rc.bottom },
		{ (rc.left + rc.right) / 2, rc.bottom },
		{ rc.left, rc.bottom },
		{ rc.left, (rc.top + rc.bottom) / 2 }
	};
	for (int i = 0; i < 8; ++i)
	{
		graphics.FillRectangle(&brush, pts[i].x - handleSize / 2, pts[i].y - handleSize / 2, handleSize, handleSize);
		graphics.DrawRectangle(&pen, pts[i].x - handleSize / 2, pts[i].y - handleSize / 2, handleSize, handleSize);
	}
}

void ClampRect(BBox& box, int minX, int minY, int maxX, int maxY)
{
	box.left = max(minX, min(box.left, maxX));
	box.right = max(minX, min(box.right, maxX));
	box.top = max(minY, min(box.top, maxY));
	box.bottom = max(minY, min(box.bottom, maxY));
	if (box.left > box.right) std::swap(box.left, box.right);
	if (box.top > box.bottom) std::swap(box.top, box.bottom);
}

int HitTestHandle(HWND hWnd, POINT ptCtrl)
{
	if (selectedIndex == -1)
	{
		return -1;
	}
	const BBox& box = bboxes[selectedIndex];
	RECT rc = ImageToControl(box);
	POINT pts[8] = {
		{ rc.left, rc.top },
		{ (rc.left + rc.right) / 2, rc.top },
		{ rc.right, rc.top },
		{ rc.right, (rc.top + rc.bottom) / 2 },
		{ rc.right, rc.bottom },
		{ (rc.left + rc.right) / 2, rc.bottom },
		{ rc.left, rc.bottom },
		{ rc.left, (rc.top + rc.bottom) / 2 }
	};
	int handleSize = 8;
	for (int i = 0; i < 8; ++i)
	{
		RECT hr = { pts[i].x - handleSize / 2, pts[i].y - handleSize / 2,
			pts[i].x + handleSize / 2, pts[i].y + handleSize / 2
		};
		if (PtInRect(&hr, ptCtrl))
		{
			return i;
		}
	}
	return -1;
}

void SaveBBoxesToFile(HWND hWnd, const std::vector<BBox>& boxes, int imgWidth, int imgHeight, std::wstring& filePath) 
{
	if (boxes.empty()) {
		MessageBox(hWnd, L"No Label added!", L"Notice", MB_OK);
		return;
	}
	FILE* file = nullptr;
	errno_t err = _wfopen_s(&file, filePath.c_str(), L"w");
	if (err != 0 || file == nullptr)
	{
		MessageBox(hWnd, L"Failed to create txt file", L"Error", MB_OK);
		return;
	}

	for (const auto& box : boxes)
	{
		int w = box.right - box.left;
		int h = box.bottom - box.top;
		if (w <= 0 || h <= 0) continue;

		float xc = (box.left + w / 2.0f) / imgWidth;
		float yc = (box.top + h / 2.0f) / imgHeight;
		float wn = (float)w / imgWidth;
		float hn = (float)h / imgHeight;

		fprintf(file, "%d %.6f %.6f %.6f %.6f\n", box.classId, xc, yc, wn, hn);
	}
	fclose(file);
}

void LoadBBoxesFromFile(const std::wstring& filePath, int imgWidth, int imgHeight)
{
	bboxes.clear();
	selectedIndex = -1;
	FILE* file = nullptr;
	errno_t err = _wfopen_s(&file, filePath.c_str(), L"r");
	if (err != 0 || file == nullptr)
	{
		return;
	}
	int classId;
	float xc, yc, wn, hn;
	while (fscanf_s(file, "%d %f %f %f %f", &classId, &xc, &yc, &wn, &hn) == 5)
	{
		BBox box;
		box.classId = classId;
		box.left = (int)((xc - wn / 2.0f) * imgWidth + 0.5f);
		box.top = (int)((yc - hn / 2.0f) * imgHeight + 0.5f);
		box.right = (int)((xc + wn / 2.0f) * imgWidth + 0.5f);
		box.bottom = (int)((yc + hn / 2.0f) * imgHeight + 0.5f);
		box.left = max(0, min(box.left, imgWidth));
		box.right = max(0, min(box.right, imgWidth));
		box.top = max(0, min(box.top, imgHeight));
		box.bottom = max(0, min(box.bottom, imgHeight));
		if (box.left > box.right) std::swap(box.left, box.right);
		if (box.top > box.bottom) std::swap(box.top, box.bottom);
		bboxes.push_back(box);
	}
	fclose(file);
}

Color GetClassColor(int classId)
{
	switch (classId) {
	case 0: return Color(255, 59, 48);
	case 1: return Color(255, 149, 0);
	case 2: return Color(255, 204, 0);
	case 3: return Color(52, 199, 89);
	case 4: return Color(0, 200, 179);
	case 5: return Color(0, 122, 255);
	case 6: return Color(97, 85, 245);
	case 7: return Color(169, 83, 247);
	case 8: return Color(231, 55, 255);
	case 9: return Color(255, 163, 231);
	default: return Color(255, 0, 0);
	}
}

LRESULT CALLBACK PicSubclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rcClient;
		GetClientRect(hWnd, &rcClient);
		int w = rcClient.right - rcClient.left;
		int h = rcClient.bottom - rcClient.top;

		HDC hdcMem = CreateCompatibleDC(hdc);
		HBITMAP hbmMem = CreateCompatibleBitmap(hdc, w, h);
		HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

		HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hdcMem, &rcClient, hBrush);
		DeleteObject(hBrush);

		int fontSize = IDCForDpi(hPagePicture, 30);

		HFONT hFont = CreateFont(
			fontSize, 0, 0, 0,
			FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, L"Microsoft Yahei UI"
		);
		HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);

		if (pCurrentImage) 
		{
			Graphics graphics(hdcMem);
			int imgW = pCurrentImage->GetWidth();
			int imgH = pCurrentImage->GetHeight();
			float ratio = min((float)w / imgW, (float)h / imgH);
			int drawW = (int)(imgW * ratio);
			int drawH = (int)(imgH * ratio);
			int x = (w - drawW) / 2;
			int y = (h - drawH) / 2;
			graphics.DrawImage(pCurrentImage, x, y, drawW, drawH);
		}
		else 
		{
			SetBkMode(hdcMem, TRANSPARENT);
			DrawText(hdcMem, L"从列表中单击选择一张图片", -1, &rcClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}
		{
			Graphics graphics(hdcMem);
			Pen pen(Color(255, 59, 48, 0), threshold);
			for (size_t i = 0; i < bboxes.size(); ++i) 
			{
				const BBox& box = bboxes[i];
				RECT rc = ImageToControl(box);
				bool sel = (i == selectedIndex);
				Color color = GetClassColor(box.classId);
				pen.SetColor(color);
				graphics.DrawRectangle(&pen, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
				if (sel)
				{
					DrawHandles(graphics, rc);
				}
			}
		}
		BitBlt(hdc, rcClient.left, rcClient.top, w, h, hdcMem, 0, 0, SRCCOPY);

		SelectObject(hdcMem, hbmOld);
		DeleteObject(hbmMem);
		DeleteDC(hdcMem);
		EndPaint(hWnd, &ps);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		SetFocus(hWnd);
		if (IsDlgButtonChecked(hPagePicture, IDC_NAME_1) == BST_CHECKED)
		{
			currentClassId = 0;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_2) == BST_CHECKED)
		{
			currentClassId = 1;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_3) == BST_CHECKED)
		{
			currentClassId = 2;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_4) == BST_CHECKED)
		{
			currentClassId = 3;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_5) == BST_CHECKED)
		{
			currentClassId = 4;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_6) == BST_CHECKED)
		{
			currentClassId = 5;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_7) == BST_CHECKED)
		{
			currentClassId = 6;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_8) == BST_CHECKED)
		{
			currentClassId = 7;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_9) == BST_CHECKED)
		{
			currentClassId = 8;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_10) == BST_CHECKED)
		{
			currentClassId = 9;
		}
		POINT ptCtrl = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		POINT ptImg = ControlToImage(ptCtrl);
		if (!pCurrentImage)
		{
			return 0;
		}
		int imgWidth = pCurrentImage->GetWidth();
		int imgHeight = pCurrentImage->GetHeight();

		int handle = HitTestHandle(hWnd, ptCtrl);
		if (handle != -1)
		{
			dragMode = Resizing;
			resizeHandle = handle;
			SetCapture(hWnd);
			return 0;
		}

		bool found = FALSE;
		int expand = threshold;
		for (int i = (int)bboxes.size() - 1; i >= 0; --i)
		{
			const BBox& box = bboxes[i];
			if (ptImg.x >= box.left - expand && ptImg.x <= box.right + expand &&
				ptImg.y >= box.top - expand && ptImg.y <= box.bottom + expand)
			{
				selectedIndex = i;
				found = TRUE;
				dragMode = Moving;
				dragOffset.x = ptImg.x - box.left;
				dragOffset.y = ptImg.y - box.top;
				SetCapture(hWnd);
				InvalidateRect(hWnd, NULL, FALSE);
				break;
			}
		}
		if (found)
		{
			return 0;
		}

		if (ptImg.x >= 0 && ptImg.x < imgWidth && ptImg.y >= 0 && ptImg.y < imgHeight)
		{
			selectedIndex = -1;
			BBox newBox;
			newBox.left = newBox.right = ptImg.x;
			newBox.top = newBox.bottom = ptImg.y;
			newBox.classId = currentClassId;
			bboxes.push_back(newBox);
			selectedIndex = (int)bboxes.size() - 1;
			dragMode = Creating;
			SetCapture(hWnd);
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else
		{
			selectedIndex = -1;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		if (dragMode == Moving && selectedIndex != -1)
		{
			POINT ptCtrl = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			POINT ptImg = ControlToImage(ptCtrl);
			BBox& box = bboxes[selectedIndex];
			int width = box.right - box.left;
			int height = box.bottom - box.top;
			int newLeft = ptImg.x - dragOffset.x;
			int newTop = ptImg.y - dragOffset.y;
			if (newLeft < 0)
			{
				newLeft = 0;
			}
			if (newTop < 0)
			{
				newTop = 0;
			}
			if (newLeft + width >= pCurrentImage->GetWidth())
			{
				newLeft = pCurrentImage->GetWidth() - width;
			}
			if (newTop + height >= pCurrentImage->GetHeight())
			{
				newTop = pCurrentImage->GetHeight() - height;
			}
			box.left = newLeft;
			box.right = newLeft + width;
			box.top = newTop;
			box.bottom = newTop + height;
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else if (dragMode == Resizing && selectedIndex != -1)
		{
			POINT ptCtrl = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			POINT ptImg = ControlToImage(ptCtrl);
			BBox& box = bboxes[selectedIndex];
			int minSize = int(min(pCurrentImage->GetHeight(), pCurrentImage->GetWidth()) / 30);
			switch (resizeHandle)
			{
			case 0: 
				box.left = ptImg.x; 
				box.top = ptImg.y; 
				break;
			case 1: 
				box.top = ptImg.y; 
				break;
			case 2: 
				box.right = ptImg.x; 
				box.top = ptImg.y; 
				break;
			case 3: 
				box.right = ptImg.x; 
				break;
			case 4: 
				box.right = ptImg.x; 
				box.bottom = ptImg.y; 
				break;
			case 5: 
				box.bottom = ptImg.y; 
				break;
			case 6: 
				box.left = ptImg.x; 
				box.bottom = ptImg.y; 
				break;
			case 7: 
				box.left = ptImg.x; 
				break;
			}
			if (box.right - box.left < minSize) 
			{
				if (resizeHandle == 0 || resizeHandle == 7 || resizeHandle == 6)
				{
					box.left = box.right - minSize;
				}
				else
				{
					box.right = box.left + minSize;
				}
			}
			if (box.bottom - box.top < minSize) 
			{
				if (resizeHandle == 0 || resizeHandle == 1 || resizeHandle == 2)
				{
					box.top = box.bottom - minSize;
				}
				else
				{
					box.bottom = box.top + minSize;
				}
			}
			int imgWidth = pCurrentImage->GetWidth();
			int imgHeight = pCurrentImage->GetHeight();
			ClampRect(box, 0, 0, imgWidth, imgHeight);
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else if (dragMode == Creating && selectedIndex != -1)
		{
			POINT ptCtrl = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			POINT ptImg = ControlToImage(ptCtrl);
			BBox& box = bboxes[selectedIndex];
			box.right = ptImg.x;
			box.bottom = ptImg.y;
			
			int minSize = int(min(pCurrentImage->GetHeight(), pCurrentImage->GetWidth()) / 30);
			if (box.right - box.left < minSize)
			{
				box.right = box.left + minSize;
			}
			if (box.bottom - box.top < minSize)
			{
				box.bottom = box.top + minSize;
			}
			ClampRect(box, 0, 0, pCurrentImage->GetWidth(), pCurrentImage->GetHeight());
			InvalidateRect(hWnd, NULL, FALSE);
		}
		return 0;
	}
	case WM_LBUTTONUP:
	{
		if (dragMode == Creating && selectedIndex != -1)
		{
			BBox& box = bboxes[selectedIndex];
			if ((box.right - box.left) < 2 || (box.bottom - box.top) < 2)
			{
				bboxes.erase(bboxes.begin() + selectedIndex);
			}
			selectedIndex = -1;
			dragMode = None;
			ReleaseCapture();
			InvalidateRect(hWnd, NULL, FALSE);
			return 0;
		}
		if (dragMode != None)
		{
			dragMode = None;
			ReleaseCapture();
			InvalidateRect(hWnd, NULL, FALSE);
		}
		return 0;
	}
	case WM_KEYDOWN: 
	{
		if (wParam == VK_DELETE && selectedIndex != -1) 
		{
			bboxes.erase(bboxes.begin() + selectedIndex);
			selectedIndex = -1;
			InvalidateRect(hWnd, NULL, FALSE);
			return TRUE;
		}
		return FALSE;
	}
	}
	return CallWindowProc(oldPicProc, hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK DlgProc_Process(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		hProgressDlg = hDlg;
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
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETMARQUEE, TRUE, 30);
		HANDLE hThread = CreateThread(NULL, 0, RefreshListThread, hDlg, 0, NULL);
		if (hThread)
		{
			CloseHandle(hThread);
		}
		return TRUE;
	}
	case WM_USER_STOP_MARQUEE:
	{
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETMARQUEE, FALSE, 30);
		EndDialog(hDlg, IDOK);
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
	}
	return FALSE;
}

INT_PTR CALLBACK DlgProc_Picture(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		hImageCtrl = CreateWindow(L"BUTTON", L"",
			WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP,
			0, 0, 0, 0,
			hDlg, (HMENU)IDC_PICTURE, GetModuleHandle(NULL), NULL
		);
		hImageCtrl = GetDlgItem(hDlg, IDC_PICTURE);
		DoCreateListView(hDlg);
		PostMessage(hDlg, WM_SIZE, 0, 0);
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"1");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"2");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"3");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"4");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"5");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"6");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"7");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"8");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"9");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_ADDSTRING, 0, (LPARAM)L"10");
		SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_SETCURSEL, 5 ,0);

		ShowWindow(GetDlgItem(hDlg, IDC_NAME_7), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_NAMEEDIT_7), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_COLOR_7), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_NAME_8), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_NAMEEDIT_8), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_COLOR_8), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_NAME_9), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_NAMEEDIT_9), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_COLOR_9), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_NAME_10), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_NAMEEDIT_10), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, IDC_COLOR_10), SW_HIDE);

		SendMessage(GetDlgItem(hDlg, IDC_NAME_1), BM_SETCHECK, BST_CHECKED, 0);
		oldPicProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hDlg, IDC_PICTURE), GWLP_WNDPROC, (LONG_PTR)PicSubclassProc);
		return 0;
	}
	case WM_SIZE:
	{
		RECT rcDlg;
		GetClientRect(hDlg, &rcDlg);
		UINT margin = IDCForDpi(hDlg, 10);
		UINT minLen = IDCForDpi(hDlg, 1);
		UINT listViewWidth = IDCForDpi(hDlg, 220);
		UINT nameWidth = 9 * margin;
		UINT nameWidthWithMargin = nameWidth + margin;

		UINT workSpaceLeft = margin * 2 + listViewWidth;
		UINT workSpaceTop = rcDlg.bottom - rcDlg.top - 17 * margin;
		UINT wsSecondRowTop = workSpaceTop + 4 * margin;
		UINT wsThirdRowTop = workSpaceTop + 7 * margin;
		UINT wsFourthRowTop = workSpaceTop + 10 * margin;
		UINT wsFifthRowTop = workSpaceTop + 13 * margin;

		SetWindowPos(GetDlgItem(hDlg, IDC_LISTVIEW), NULL, margin, margin, listViewWidth, rcDlg.bottom - rcDlg.top - 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_PICTURE), NULL, workSpaceLeft, margin, rcDlg.right - rcDlg.left - 3 * margin - listViewWidth, rcDlg.bottom - rcDlg.top - 19 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_OK), NULL, rcDlg.right - rcDlg.left - 12 * margin, wsFifthRowTop, 11 * margin, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_ST_NUMBER), NULL, workSpaceLeft, workSpaceTop + margin, 9 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NUMBER), NULL, workSpaceLeft + 10 * margin, workSpaceTop + margin - minLen, 4 * margin, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_1), NULL, workSpaceLeft + minLen, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_2), NULL, workSpaceLeft + minLen + nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_3), NULL, workSpaceLeft + minLen + 2 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_4), NULL, workSpaceLeft + minLen + 3 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_5), NULL, workSpaceLeft + minLen + 4 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_6), NULL, workSpaceLeft + minLen + 5 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_7), NULL, workSpaceLeft + minLen + 6 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_8), NULL, workSpaceLeft + minLen + 7 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_9), NULL, workSpaceLeft + minLen + 8 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAME_10), NULL, workSpaceLeft + minLen + 9 * nameWidthWithMargin, wsSecondRowTop, nameWidth, 3 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_1), NULL, workSpaceLeft + minLen, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_2), NULL, workSpaceLeft + minLen + nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_3), NULL, workSpaceLeft + minLen + 2 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_4), NULL, workSpaceLeft + minLen + 3 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_5), NULL, workSpaceLeft + minLen + 4 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_6), NULL, workSpaceLeft + minLen + 5 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_7), NULL, workSpaceLeft + minLen + 6 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_8), NULL, workSpaceLeft + minLen + 7 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_9), NULL, workSpaceLeft + minLen + 8 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_NAMEEDIT_10), NULL, workSpaceLeft + minLen + 9 * nameWidthWithMargin, wsThirdRowTop, nameWidth, 2 * margin + 3 * minLen, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_1), NULL, workSpaceLeft + minLen, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_2), NULL, workSpaceLeft + minLen + nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_3), NULL, workSpaceLeft + minLen + 2 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_4), NULL, workSpaceLeft + minLen + 3 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_5), NULL, workSpaceLeft + minLen + 4 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_6), NULL, workSpaceLeft + minLen + 5 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_7), NULL, workSpaceLeft + minLen + 6 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_8), NULL, workSpaceLeft + minLen + 7 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_9), NULL, workSpaceLeft + minLen + 8 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);
		SetWindowPos(GetDlgItem(hDlg, IDC_COLOR_10), NULL, workSpaceLeft + minLen + 9 * nameWidthWithMargin, wsFourthRowTop, nameWidth, 2 * margin, SWP_NOZORDER);

		return 0;
	}
	case WM_USER_REFRESH_LIST:
	{
		if (isProcessExist)
		{
			return 0; 
		}
		SetTimer(hDlg, TIMER_REFRESH_DEBOUNCE, 500, NULL);
		return 0;
	}

	case WM_TIMER:
	{
		if (wParam == TIMER_REFRESH_DEBOUNCE)
		{
			KillTimer(hDlg, TIMER_REFRESH_DEBOUNCE);
			if (!isProcessExist)
			{
				isProcessExist = true;
				MSG msg;
				while (PeekMessage(&msg, NULL, WM_USER_REFRESH_LIST, WM_USER_REFRESH_LIST, PM_REMOVE)) {}
				DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_PAGEPROCESS), hDlg, DlgProc_Process);
				isProcessExist = false;
			}
		}
		return 0;
	}
	case WM_USER_UPDATE_LISTVIEW:
	{
		std::vector<ImageFileInfo>* pData = (std::vector<ImageFileInfo>*)wParam;
		if (pData)
		{
			HWND hList = GetDlgItem(hDlg, IDC_LISTVIEW);
			RefreshListUI(hList, *pData);
			delete pData;
			if (hProgressDlg)
			{
				PostMessage(hProgressDlg, WM_USER_STOP_MARQUEE, 0, 0);
			}
		}
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
	case WM_USER_STOP_MONITOR:
		StopFolderMonitor();
		return 0;
	case WM_COMMAND:
	{
		int WM_ID = LOWORD(wParam);
		int WM_CODE = HIWORD(wParam);
		switch (WM_ID)
		{
		case IDC_OK:
		{
			if (!pCurrentImage)
			{
				return 0;
			}
			std::wstring txtPath = currentImagePath;
			size_t dotPos = txtPath.find_last_of(L'.');
			if (dotPos != std::wstring::npos)
			{
				txtPath = txtPath.substr(0, dotPos) + L".txt";
			}
			else
			{
				txtPath += L".txt";
			}
			SaveBBoxesToFile(hDlg, bboxes, pCurrentImage->GetWidth(), pCurrentImage->GetHeight(), txtPath);
			SelectNextImage(GetDlgItem(hDlg, IDC_LISTVIEW));
			return 0;
		}
		case IDC_SWITCH_CLASS0:
			CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_6, IDC_NAME_1);
			return TRUE;
		case IDC_SWITCH_CLASS1:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_2)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_2);
			}
			return TRUE;
		case IDC_SWITCH_CLASS2:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_3)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_3);
			}
			return TRUE;
		case IDC_SWITCH_CLASS3:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_4)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_4);
			}
			return TRUE;
		case IDC_SWITCH_CLASS4:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_5)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_5);
			}
			return TRUE;
		case IDC_SWITCH_CLASS5:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_6)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_6);
			}
			return TRUE;
		case IDC_SWITCH_CLASS6:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_7)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_7);
			}
			return TRUE;
		case IDC_SWITCH_CLASS7:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_8)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_8);
			}
			return TRUE;
		case IDC_SWITCH_CLASS8:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_9)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_9);
			}
			return TRUE;
		case IDC_SWITCH_CLASS9:
			if (IsWindowVisible(GetDlgItem(hDlg, IDC_NAME_10)))
			{
				CheckRadioButton(hDlg, IDC_NAME_1, IDC_NAME_10, IDC_NAME_10);
			}
			return TRUE;
		case IDC_NUMBER:
		{
			int selIndex = (int)SendMessage(GetDlgItem(hDlg, IDC_NUMBER), CB_GETCURSEL, 0, 0);
			if (selIndex != CB_ERR)
			{
				for (int i = selIndex + 1; i < 10; ++i)
				{
					ShowWindow(GetDlgItem(hDlg, IDC_NAME_1 + i), SW_HIDE);
				}
				for (int i = selIndex + 1; i < 10; ++i)
				{
					ShowWindow(GetDlgItem(hDlg, IDC_NAMEEDIT_1 + i), SW_HIDE);
				}
				for (int i = selIndex + 1; i < 10; ++i)
				{
					ShowWindow(GetDlgItem(hDlg, IDC_COLOR_1 + i), SW_HIDE);
				}
				for (int i = 0; i < selIndex + 1; ++i)
				{
					ShowWindow(GetDlgItem(hDlg, IDC_NAME_1 + i), SW_SHOW);
				}
				for (int i = 0; i < selIndex + 1; ++i)
				{
					ShowWindow(GetDlgItem(hDlg, IDC_NAMEEDIT_1 + i), SW_SHOW);
				}
				for (int i = 0; i < selIndex + 1; ++i)
				{
					ShowWindow(GetDlgItem(hDlg, IDC_COLOR_1 + i), SW_SHOW);
				}
			}
			return TRUE;
		}
		}
		return 0;
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
			HBRUSH hBrush = CreateSolidBrush(RGB(0, 200, 179));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_6) {
			HBRUSH hBrush = CreateSolidBrush(RGB(0, 122, 255));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_7) {
			HBRUSH hBrush = CreateSolidBrush(RGB(97, 85, 245));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_8) {
			HBRUSH hBrush = CreateSolidBrush(RGB(169, 83, 247));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_9) {
			HBRUSH hBrush = CreateSolidBrush(RGB(231, 55, 255));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_COLOR_10) {
			HBRUSH hBrush = CreateSolidBrush(RGB(255, 163, 231));
			FillRect(lpDIS->hDC, &lpDIS->rcItem, hBrush);
			DeleteObject(hBrush);
			return TRUE;
		}
		else if (lpDIS->CtlID == IDC_PICTURE)
		{
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