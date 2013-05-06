
#include "pch.h"
#include "main.h"

int add_default_tray_icon(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  NOTIFYICONDATA nid;

  memset(&nid, 0, sizeof(nid));
  nid.cbSize= sizeof(nid);
  nid.hWnd= frame_wnd;
  nid.uFlags= NIF_ICON|NIF_MESSAGE|NIF_TIP;
  nid.uCallbackMessage= WG_TRAY;
  nid.hIcon= info->small_icon;
  Shell_NotifyIcon(NIM_ADD, &nid);
  info->tray_icon= info->small_icon;
  return 0;
}

int remove_tray_icon(HWND frame_wnd)
{
  NOTIFYICONDATA nid;

  memset(&nid, 0, sizeof(nid));
  nid.cbSize= sizeof(nid);
  nid.hWnd= frame_wnd;
  Shell_NotifyIcon(NIM_DELETE, &nid);
  return 0;
}

LRESULT on_tray_lbuttonup(HWND frame_wnd)
{
  do_showhide(frame_wnd);
  return 0;
}

LRESULT on_tray_rbuttonup(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  POINT pt;
  HMENU submenu;
  NOTIFYICONDATA nid;

  GetCursorPos(&pt);
  SetForegroundWindow(frame_wnd);
  submenu= GetSubMenu(info->tray_menu, 0);
  TrackPopupMenu(submenu, 0, pt.x, pt.y, 0, frame_wnd, NULL);
  Shell_NotifyIcon(NIM_SETFOCUS, &nid); /* Docs say I should do this */
  return 0;
}
