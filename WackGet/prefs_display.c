
#include "pch.h"
#include "main.h"
#include "resource.h"

static INT_PTR on_display_initdialog(HWND dlg_wnd)
{
  int i, j;

  /* View */
  if(get_prefs_int("ShowMenuBar", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_MENUBAR), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("ShowStatusBar", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_STATUSBAR), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("ShowColumnHeaders", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_COLUMNHEADERS), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("AlwaysOnTop", 0)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_ALWAYSONTOP), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("ShowGridLines", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_SHOWGRIDLINES), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("TrayStatus", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_TRAYSTATUS), BM_SETCHECK, BST_CHECKED, 0);
  }
  i= get_prefs_int("ShowInTaskbar", 1);
  j= get_prefs_int("ShowInSystray", 1);
  if(i && !j) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_SHOW_TASKBAR), BM_SETCHECK, BST_CHECKED, 0);
  } else if(!i && j) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_SHOW_SYSTRAY), BM_SETCHECK, BST_CHECKED, 0);
  } else {
    SendMessage(GetDlgItem(dlg_wnd, IDC_SHOW_BOTH), BM_SETCHECK, BST_CHECKED, 0);
  }

  SetWindowLongPtr(dlg_wnd, GWLP_USERDATA, 1); /* Initialization complete */
  return TRUE;
}

static INT_PTR on_display_clicked(HWND dlg_wnd, WPARAM wParam)
{
  if(GetWindowLongPtr(dlg_wnd, GWLP_USERDATA)) {
    switch(LOWORD(wParam)) {
    case IDC_MENUBAR:
    case IDC_STATUSBAR:
    case IDC_COLUMNHEADERS:
    case IDC_SHOWGRIDLINES:
    case IDC_TRAYSTATUS:
    case IDC_ALWAYSONTOP:
    case IDC_SHOW_TASKBAR:
    case IDC_SHOW_SYSTRAY:
    case IDC_SHOW_BOTH:
      SendMessage(GetParent(dlg_wnd), PSM_CHANGED, (WPARAM)dlg_wnd, 0);
      SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
      return TRUE;
    }
  }
  return FALSE;
}

static INT_PTR on_display_apply(HWND dlg_wnd)
{
  int i, j, k;

  if(!IsWindowEnabled(GetDlgItem(GetParent(dlg_wnd), ID_APPLY_NOW))) {
    return FALSE;
  }

  /* View */
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_MENUBAR), BM_GETCHECK, 0, 0);
  set_prefs_int("ShowMenuBar", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_STATUSBAR), BM_GETCHECK, 0, 0);
  set_prefs_int("ShowStatusBar", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_COLUMNHEADERS), BM_GETCHECK, 0, 0);
  set_prefs_int("ShowColumnHeaders", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_ALWAYSONTOP), BM_GETCHECK, 0, 0);
  set_prefs_int("AlwaysOnTop", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_SHOWGRIDLINES), BM_GETCHECK, 0, 0);
  set_prefs_int("ShowGridLines", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_TRAYSTATUS), BM_GETCHECK, 0, 0);
  set_prefs_int("TrayStatus", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_SHOW_TASKBAR), BM_GETCHECK, 0, 0);
  j= SendMessage(GetDlgItem(dlg_wnd, IDC_SHOW_SYSTRAY), BM_GETCHECK, 0, 0);
  k= SendMessage(GetDlgItem(dlg_wnd, IDC_SHOW_BOTH), BM_GETCHECK, 0, 0);
  set_prefs_int("ShowInTaskbar", i==BST_CHECKED || k==BST_CHECKED);
  set_prefs_int("ShowInSystray", j==BST_CHECKED || k==BST_CHECKED);

  PostMessage(GetParent(GetParent(dlg_wnd)), WG_PREFS_DISPLAYCHANGED, 0, 0);

  SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, PSNRET_NOERROR);
  return TRUE;
}

INT_PTR CALLBACK display_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_INITDIALOG:
    return on_display_initdialog(hwndDlg);
  case WM_COMMAND:
    switch(HIWORD(wParam)) {
    case BN_CLICKED:
      return on_display_clicked(hwndDlg, wParam);
    }
    break;
  case WM_NOTIFY:
    switch(((NMHDR *)lParam)->code) {
    case PSN_APPLY:
      return on_display_apply(hwndDlg);
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
