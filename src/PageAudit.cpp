#ifndef UNICODE
#define UNICODE
#endif

#include <main.h>

INT_PTR CALLBACK DlgProc_Audit(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
		DestroyWindow(hDlg);
		if (hPagePicture && IsWindow(hPagePicture))
		{
			SetFocus(GetDlgItem(hPagePicture, IDC_PICTURE));
		}
		return TRUE;
	}
	return FALSE;
}