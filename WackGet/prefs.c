
#include "pch.h"
#include "main.h"
#include "resource.h"

static int CALLBACK sheet_cb(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
  if(uMsg==PSCB_INITIALIZED) {
    frame_info_t *info= (void *)GetWindowLongPtr(GetParent(hwndDlg), GWLP_USERDATA);

    info->prefs_wnd= hwndDlg;
  }
  return 0;
}

LRESULT on_file_preferences(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  PROPSHEETPAGE psps[4];
  PROPSHEETHEADER psh;

  if(info->prefs_wnd) {
    SetForegroundWindow(info->prefs_wnd);
    return 0;
  }
  memset(psps, 0, sizeof(psps));
  psps[0].dwSize= sizeof(psps[0]);
  psps[0].dwFlags= PSP_HASHELP;
  psps[0].pszTemplate= MAKEINTRESOURCE(IDD_PREFS_GENERAL);
  psps[0].pfnDlgProc= general_dlgproc;
  psps[1].dwSize= sizeof(psps[1]);
  psps[1].dwFlags= PSP_HASHELP;
  psps[1].pszTemplate= MAKEINTRESOURCE(IDD_PREFS_INTEGRATION);
  psps[1].pfnDlgProc= integration_dlgproc;
  psps[2].dwSize= sizeof(psps[2]);
  psps[2].dwFlags= PSP_HASHELP;
  psps[2].pszTemplate= MAKEINTRESOURCE(IDD_PREFS_DISPLAY);
  psps[2].pfnDlgProc= display_dlgproc;
  psps[3].dwSize= sizeof(psps[3]);
  psps[3].dwFlags= PSP_HASHELP;
  psps[3].pszTemplate= MAKEINTRESOURCE(IDD_PREFS_PROXY);
  psps[3].pfnDlgProc= proxy_dlgproc;

  memset(&psh, 0, sizeof(psh));
  psh.dwSize= sizeof(psh);
  psh.dwFlags= PSH_PROPSHEETPAGE|PSH_USEHICON|PSH_USECALLBACK|PSH_HASHELP|PSH_NOCONTEXTHELP;
  psh.hwndParent= frame_wnd;
  psh.hIcon= info->small_icon;
  psh.pszCaption= "WackGet Preferences";
  psh.nPages= sizeof(psps)/sizeof(psps[0]);
  psh.ppsp= psps;
  psh.pfnCallback= sheet_cb;
  PropertySheet(&psh);
  info->prefs_wnd= NULL;
  return 0;
}

LRESULT on_prefs_generalchanged(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int i;

  /* Log */
  i= get_prefs_int("Log", 0);
  if(!i) {
    do_log(NULL); /* Close log file */
  }
  return 0;
}

LRESULT on_prefs_integrationchanged(HWND frame_wnd)
{
  /* Hot key */
  UnregisterHotKey(frame_wnd, 0); /* Always unregister first (to avoid double-registering) */
  if(get_prefs_int("HotKey", 0)) {
    register_hot_key(frame_wnd);
  }
  return 0;
}

void on_prefs_displaychanged_alwaysontop(HWND frame_wnd)
{
  int i;

  i= get_prefs_int("AlwaysOnTop", 0);
  SetWindowPos(frame_wnd, (i)? HWND_TOPMOST: HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}

LRESULT on_prefs_displaychanged(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int i, j, k;

  /* Column headers */
  i= get_prefs_int("ShowColumnHeaders", 1);
  j= GetWindowLongPtr(info->list_wnd, GWL_STYLE);
  if(BOOLCMP(i, !(j&LVS_NOCOLUMNHEADER))) {
    SetWindowLongPtr(info->list_wnd, GWL_STYLE, (i)? j&~LVS_NOCOLUMNHEADER: j|LVS_NOCOLUMNHEADER);
    SetWindowPos(info->list_wnd, NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
  }

  /* Grid lines */
  i= get_prefs_int("ShowGridLines", 1);
  j= SendMessage(info->list_wnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
  if(BOOLCMP(i, j&LVS_EX_GRIDLINES)) {
    SendMessage(info->list_wnd, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_GRIDLINES,
                (i)? j|LVS_EX_GRIDLINES: 0);
  }

  /* Menu bar */
  i= get_prefs_int("ShowMenuBar", 1);
  if(BOOLCMP(i, GetMenu(frame_wnd)!=NULL)) {
    SetMenu(frame_wnd, (i)? info->menu: NULL);
  }

  /* Status bar */
  i= get_prefs_int("ShowStatusBar", 1);
  if(BOOLCMP(i, IsWindowVisible(info->sb_wnd))) {
    RECT rc;

    ShowWindow(info->sb_wnd, (i)? SW_SHOW: SW_HIDE);
    GetWindowRect(frame_wnd, &rc);
    PostMessage(frame_wnd, WM_SIZE, 0, MAKELPARAM(rc.right-rc.left, rc.bottom-rc.top));
  }

  /* Always on top */
  on_prefs_displaychanged_alwaysontop(frame_wnd);

  /* Show/hide in taskbar */
  i= get_prefs_int("ShowInTaskbar", 1);
  j= GetWindowLongPtr(frame_wnd, GWL_EXSTYLE);
  if(BOOLCMP(i, j&WS_EX_APPWINDOW)) {
    if(k= IsWindowVisible(frame_wnd)) {
      ShowWindow(frame_wnd, SW_HIDE); /* HACK: Must hide & show the window to apply style */
    }
    SetWindowLongPtr(frame_wnd, GWL_EXSTYLE, (i)? j|WS_EX_APPWINDOW: j&~WS_EX_APPWINDOW);
    if(k) {
      ShowWindow(frame_wnd, (info->prefs_wnd)? SW_SHOWNA: SW_SHOW); /* Don't activate window if prefs dialog is still up */
    }

    /* Enable minimize button if in taskbar */
    j= GetWindowLongPtr(frame_wnd, GWL_STYLE);
    SetWindowLongPtr(frame_wnd, GWL_STYLE, (i)? j|WS_MINIMIZEBOX: j&~WS_MINIMIZEBOX);
    SetWindowPos(frame_wnd, NULL, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_FRAMECHANGED);
  }

  /* Show/hide in systray */
  i= get_prefs_int("ShowInSystray", 1);
  if(i) {
    add_default_tray_icon(frame_wnd);
  } else {
    remove_tray_icon(frame_wnd);
    if(!IsWindowVisible(frame_wnd)) {
      ShowWindow(frame_wnd, SW_SHOWNA); /* Can't remain hidden if not in systray! */
    }
  }
  return 0;
}
