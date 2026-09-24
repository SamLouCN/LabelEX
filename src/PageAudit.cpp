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
		return TRUE;
	}
	return FALSE;
}