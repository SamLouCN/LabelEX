#ifndef UNICODE
#define UNICODE
#endif

#include "main.h"

INT_PTR CALLBACK DlgProc_Video(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	{
		ShowWindow(hDlg, SW_HIDE);
		return TRUE;
	}
	}
	return FALSE;
}