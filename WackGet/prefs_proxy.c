
#include "pch.h"
#include "main.h"
#include "resource.h"

static void update_proxy_controls(HWND dlg_wnd)
{
  int i;

  i= SendMessage(GetDlgItem(dlg_wnd, IDC_PROXY), BM_GETCHECK, 0, 0);
  EnableWindow(GetDlgItem(dlg_wnd, IDC_PROXY_SERVER), i==BST_CHECKED);
  EnableWindow(GetDlgItem(dlg_wnd, IDC_PROXY_USERNAME), i==BST_CHECKED);
  EnableWindow(GetDlgItem(dlg_wnd, IDC_PROXY_PASSWORD), i==BST_CHECKED);
}

static INT_PTR on_proxy_initdialog(HWND dlg_wnd)
{
  char buf[255];

  /* Proxy server */
  if(get_prefs_int("Proxy", 0)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_PROXY), BM_SETCHECK, BST_CHECKED, 0);
  }
  get_prefs_str("Proxy_Server", buf, sizeof(buf), "");
  SetWindowText(GetDlgItem(dlg_wnd, IDC_PROXY_SERVER), buf);
  get_prefs_str("Proxy_Username", buf, sizeof(buf), "");
  SetWindowText(GetDlgItem(dlg_wnd, IDC_PROXY_USERNAME), buf);
  get_prefs_str("Proxy_Password", buf, sizeof(buf), "");
  SetWindowText(GetDlgItem(dlg_wnd, IDC_PROXY_PASSWORD), buf);
  update_proxy_controls(dlg_wnd);

  SetWindowLongPtr(dlg_wnd, GWLP_USERDATA, 1); /* Initialization complete */
  return TRUE;
}

static INT_PTR on_proxy_clicked(HWND dlg_wnd, WPARAM wParam)
{
  if(GetWindowLongPtr(dlg_wnd, GWLP_USERDATA)) {
    switch(LOWORD(wParam)) {
    case IDC_PROXY:
      update_proxy_controls(dlg_wnd);
      SendMessage(GetParent(dlg_wnd), PSM_CHANGED, (WPARAM)dlg_wnd, 0);
      SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
      return TRUE;
    }
  }
  return FALSE;
}

static INT_PTR on_proxy_change(HWND dlg_wnd, WPARAM wParam)
{
  if(GetWindowLongPtr(dlg_wnd, GWLP_USERDATA)) {
    switch(LOWORD(wParam)) {
    case IDC_PROXY_SERVER:
    case IDC_PROXY_USERNAME:
    case IDC_PROXY_PASSWORD:
      SendMessage(GetParent(dlg_wnd), PSM_CHANGED, (WPARAM)dlg_wnd, 0);
      SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
      return TRUE;
    }
  }
  return FALSE;
}

static INT_PTR on_proxy_apply(HWND dlg_wnd)
{
  int i;
  char buf[MAX_PATH];

  if(!IsWindowEnabled(GetDlgItem(GetParent(dlg_wnd), ID_APPLY_NOW))) {
    return FALSE;
  }

  /* Proxy server */
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_PROXY), BM_GETCHECK, 0, 0);
  set_prefs_int("Proxy", i==BST_CHECKED);
  GetWindowText(GetDlgItem(dlg_wnd, IDC_PROXY_SERVER), buf, sizeof(buf));
  set_prefs_str("Proxy_Server", buf);
  GetWindowText(GetDlgItem(dlg_wnd, IDC_PROXY_USERNAME), buf, sizeof(buf));
  set_prefs_str("Proxy_Username", buf);
  GetWindowText(GetDlgItem(dlg_wnd, IDC_PROXY_PASSWORD), buf, sizeof(buf));
  set_prefs_str("Proxy_Password", buf);

  SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, PSNRET_NOERROR);
  return TRUE;
}

INT_PTR CALLBACK proxy_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_INITDIALOG:
    return on_proxy_initdialog(hwndDlg);
  case WM_COMMAND:
    switch(HIWORD(wParam)) {
    case BN_CLICKED:
      return on_proxy_clicked(hwndDlg, wParam);
    case EN_CHANGE:
      return on_proxy_change(hwndDlg, wParam);
    }
    break;
  case WM_NOTIFY:
    switch(((NMHDR *)lParam)->code) {
    case PSN_APPLY:
      return on_proxy_apply(hwndDlg);
    case PSN_HELP:
      do_help();
      return TRUE;
    }
    break;
  case WM_HELP:
    do_help();
    break;
  }
  return FALSE;
}
