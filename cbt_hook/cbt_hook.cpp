// cbt_hook.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "cbt_hook.h"
#include <map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define DLG_CLASS_NAME L"#32770"

#define WM_DIALOG_UI_BEG WM_USER + 1
#define WM_DIALOG_UI_END WM_USER + 2

HINSTANCE g_hinstance = NULL;
HHOOK g_hhook = NULL;

TCHAR *g_is_ui_title[] =
{
	L"target_mfc_dialog",
};

std::map<CString, HWND> g_map_hwnd;

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// Ccbt_hookApp

BEGIN_MESSAGE_MAP(Ccbt_hookApp, CWinApp)
END_MESSAGE_MAP()


// Ccbt_hookApp construction

Ccbt_hookApp::Ccbt_hookApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only Ccbt_hookApp object

Ccbt_hookApp theApp;


// Ccbt_hookApp initialization

BOOL Ccbt_hookApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

int try_insert_map(HWND hwnd, const TCHAR *wnd_name)
{
	TCHAR class_name[MAX_PATH] = {0};
	if (!GetClassName(hwnd, class_name, MAX_PATH))
	{
		return -1;
	}

	if (_tcsicmp(class_name, DLG_CLASS_NAME))
	{
		return -1;
	}

	int size = sizeof(g_is_ui_title) / sizeof(g_is_ui_title[0]);
	for (int i = 0; i < size; ++i)
	{
		if (!_tcsicmp(wnd_name, g_is_ui_title[i]))
		{
			TCHAR temp[MAX_PATH] = {0};
			CString str(wnd_name);

			std::pair<CString, HWND> map_pair = std::make_pair(str, hwnd);
			if (false == g_map_hwnd.insert(std::make_pair(str, hwnd)).second)
			{
				// key is equal, update value
				g_map_hwnd[str] = hwnd;
			}

			_stprintf_s(temp, L"target wnd created: %s, %08x, map size: %d", str, hwnd, g_map_hwnd.size());
			OutputDebugString(temp);

			if (!_tcsicmp(wnd_name, g_is_ui_title[0]))
			{
				HWND win32_dll_loader_hwnd = FindWindow(NULL, L"win32_dll_loader");
				_stprintf_s(temp, L"wparam: %x", hwnd);
				OutputDebugString(temp);
				SendMessage(win32_dll_loader_hwnd, WM_DIALOG_UI_BEG, (WPARAM)hwnd, 0);
			}
		}
	}

	return 0;
}

int try_update_map(HWND hwnd)
{
	if (g_map_hwnd.empty())
	{
		return -1;
	}

	for (std::map<CString, HWND>::iterator iter = g_map_hwnd.begin();
		iter != g_map_hwnd.end(); ++iter)
	{
		if (iter->second == hwnd)
		{
			TCHAR temp[MAX_PATH] = {0};
			_stprintf_s(temp, L"target wnd destroy: %s, %08x", iter->first, iter->second);
			OutputDebugString(temp);
			iter->second = 0;

			if (!_tcsicmp(iter->first, g_is_ui_title[0]))
			{
				HWND win32_dll_loader_hwnd = FindWindow(NULL, L"win32_dll_loader");
				_stprintf_s(temp, L"wparam: %x", hwnd);
				OutputDebugString(temp);
				SendMessage(win32_dll_loader_hwnd, WM_DIALOG_UI_END, 0, 0);
			}
		}
	}

	return 0;
}

LRESULT CALLBACK CBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HCBT_CREATEWND)
	{
		CBT_CREATEWND *pcc = (CBT_CREATEWND *)lParam;
		try_insert_map((HWND)wParam, pcc->lpcs->lpszName);

		return 0;
	}

	if (nCode == HCBT_DESTROYWND)
	{
		try_update_map((HWND)wParam);

		return 0;
	}

	return CallNextHookEx(g_hhook, nCode, wParam, lParam);
}

extern "C" __declspec(dllexport) void WINAPI BegCbtHook()
{
	g_hinstance = GetModuleHandle(L"cbt_hook");
	g_hhook = SetWindowsHookEx(WH_CBT, CBTProc, g_hinstance, 0);
}

extern "C" __declspec(dllexport) void WINAPI EndCbtHook()
{
	if (NULL != g_hhook)
		UnhookWindowsHookEx(g_hhook);
}
