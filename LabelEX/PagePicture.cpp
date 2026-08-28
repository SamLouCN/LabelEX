#ifndef UNICODE
#define UNICODE
#endif
#define IDC_LISTVIEW 5001
#define WM_USER_REFRESH_LIST (WM_USER + 100)
#define WM_USER_UPDATE_ITEM (WM_USER + 101)

#include "main.h"

using namespace Gdiplus;

struct BBox {
	int left, top, right, bottom;
	int classId;
	bool selected;
};

Bitmap* pCurrentImage = nullptr;
HWND hImageCtrl = nullptr;
std::vector<BBox> bboxes;
int currentClassId = 0;
int selectedIndex = -1;
int threshold = 10;
BOOL bCreating = TRUE;
WNDPROC oldPicProc = NULL;
enum DragMode {None, Moving, Resizing, Creating};
DragMode dragMode = None;
int resizeHandle = -1;
POINT dragStart;
POINT dragOffset;
BBox originalBox;
wchar_t szFolderPath[MAX_PATH] = { 0 };
std::wstring currentImagePath;

void DoSelectFolder(HWND hWnd);
void RefreshList(HWND hWnd);
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
		box.selected = false;
		box.left = (int)((xc - wn / 2.0f) * imgWidth);
		box.top = (int)((yc - hn / 2.0f) * imgHeight);
		box.right = (int)((xc + wn / 2.0f) * imgWidth);
		box.bottom = (int)((yc + hn / 2.0f) * imgHeight);
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
	case 1: return Color(255, 59, 48);
	case 2: return Color(255, 149, 0);
	case 3: return Color(255, 204, 0);
	case 4: return Color(52, 199, 89);
	case 5: return Color(0, 122, 255);
	case 6: return Color(175, 82, 222);
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

		HFONT hFont = CreateFont(
			40, 0, 0, 0,
			FW_BOLD, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
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
			Pen pen(Color(255, 59, 48, 0), 10);
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
			currentClassId = 1;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_2) == BST_CHECKED)
		{
			currentClassId = 2;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_3) == BST_CHECKED)
		{
			currentClassId = 3;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_4) == BST_CHECKED)
		{
			currentClassId = 4;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_5) == BST_CHECKED)
		{
			currentClassId = 5;
		}
		else if (IsDlgButtonChecked(hPagePicture, IDC_NAME_6) == BST_CHECKED)
		{
			currentClassId = 6;
		}
		POINT ptCtrl = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		POINT ptImg = ControlToImage(ptCtrl);
		int imgWidth = pCurrentImage->GetWidth();
		int imgHeight = pCurrentImage->GetHeight();

		int handle = HitTestHandle(hWnd, ptCtrl);
		if (handle != -1)
		{
			dragMode = Resizing;
			resizeHandle = handle;
			if (selectedIndex != -1)
			{
				originalBox = bboxes[selectedIndex];
			}
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
			newBox.selected = false;
			bboxes.push_back(newBox);
			selectedIndex = (int)bboxes.size() - 1;
			bCreating = TRUE;
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
			box.left = ptImg.x - dragOffset.x;
			box.right = box.left + width;
			box.top = ptImg.y - dragOffset.y;
			box.bottom = box.top + height;
			ClampRect(box, 0, 0, pCurrentImage->GetWidth(), pCurrentImage->GetHeight());
			InvalidateRect(hWnd, NULL, FALSE);
		}
		else if (dragMode == Resizing && selectedIndex != -1)
		{
			POINT ptCtrl = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			POINT ptImg = ControlToImage(ptCtrl);
			BBox& box = bboxes[selectedIndex];
			int minSize = 5;
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
			int newLeft = min(box.left, ptImg.x);
			int newRight = max(box.left, ptImg.x);
			int newTop = min(box.top, ptImg.y);
			int newBottom = max(box.top, ptImg.y);
			box.left = newLeft;
			box.right = newRight;
			box.top = newTop;
			box.bottom = newBottom;
			int minSize = 5;
			if (box.right - box.left < minSize) 
			{
				if (ptImg.x < box.left)
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
				if (ptImg.y < box.top) 
				{ 
					box.top = box.bottom - minSize;
				}
				else 
				{ 
					box.bottom = box.top + minSize;
				}
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
			bCreating = FALSE;
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
		}
		return 0;
	}
	}
	return CallWindowProc(oldPicProc, hWnd, message, wParam, lParam);
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
		SendMessage(GetDlgItem(hDlg, IDC_ENABLE_EDIT), BM_SETCHECK, BST_CHECKED, 0);
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
			std::wstring txtPath = currentImagePath;
			size_t dotPos = txtPath.find_last_of(L'.');
			if (dotPos != std::wstring::npos) {
				txtPath = txtPath.substr(0, dotPos) + L".txt";
			}
			else {
				txtPath += L".txt";
			}
			SaveBBoxesToFile(hDlg, bboxes, pCurrentImage->GetWidth(), pCurrentImage->GetHeight(), txtPath);
			SelectNextImage(GetDlgItem(hDlg, IDC_LISTVIEW));
			return 0;
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