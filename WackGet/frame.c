
#include "pch.h"
#include "main.h"
#include "resource.h"
#include <windows.h>

static LRESULT on_setfocus(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  SetFocus(info->list_wnd);
  return 0;
}

static LRESULT on_size(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  RECT rc;

  GetClientRect(frame_wnd, &rc);

  /* Compensate for status bar if visible */
  if(IsWindowVisible(info->sb_wnd)) {
    RECT rc1;

    SendMessage(info->sb_wnd, WM_SIZE, wParam, lParam); /* Tell status bar to resize itself first */
    GetWindowRect(info->sb_wnd, &rc1);
    ScreenToClient(frame_wnd, (POINT *)&rc1.left);
    rc.bottom= rc1.top;
  }

  MoveWindow(info->list_wnd, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, TRUE);
  return 0;
}

static LRESULT on_setidlemessage(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  if(!SendMessage(info->sb_wnd, SB_ISSIMPLE, 0, 0)) {
    SendMessage(info->sb_wnd, SB_SETTEXT, 0, (LPARAM)info->idle_msg);
  }
  return 0;
}

static LRESULT on_menuselect(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  /* TODO: Investgate MenuHelp() and make sure this is the most efficient way */

  if(HIWORD(wParam)==0xffff && !lParam) { /* If menu is closed, revert to non-simple mode */
    if(SendMessage(info->sb_wnd, SB_ISSIMPLE, 0, 0)) {
      SendMessage(info->sb_wnd, SB_SIMPLE, FALSE, 0);
      SendMessage(info->sb_wnd, SB_SETTEXT, 0, (LPARAM)info->idle_msg);
    }
  } else {
    char buf[255];

    if(LoadString(info->inst, LOWORD(wParam), buf, sizeof(buf))) {
      if(!SendMessage(info->sb_wnd, SB_ISSIMPLE, 0, 0)) {
        SendMessage(info->sb_wnd, SB_SIMPLE, TRUE, 0);
      }
      SendMessage(info->sb_wnd, SB_SETTEXT, SB_SIMPLEID|SBT_NOBORDERS, (LPARAM)buf);
    } else if(SendMessage(info->sb_wnd, SB_ISSIMPLE, 0, 0)) { /* If no help text, revert to non-simple mode */
      SendMessage(info->sb_wnd, SB_SIMPLE, FALSE, 0);
      SendMessage(info->sb_wnd, SB_SETTEXT, 0, (LPARAM)info->idle_msg);
    }
  }
  return 0;
}

static LRESULT on_view_alwaysontop(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  /* Toggle always on top & apply */
  set_prefs_int("AlwaysOnTop", !get_prefs_int("AlwaysOnTop", 0));
  on_prefs_displaychanged_alwaysontop(frame_wnd);
  return 0;
}

static LRESULT on_app_about(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  DialogBox(info->inst, MAKEINTRESOURCE(IDD_ABOUTBOX), frame_wnd, about_dlgproc);
  return 0;
}

static LRESULT on_file_opendownloadfolder(HWND frame_wnd)
{
  char pn[MAX_PATH];

  get_prefs_str("DownloadDir", pn, sizeof(pn), "");
  ShellExecute(NULL, NULL, pn, NULL, NULL, SW_SHOWNORMAL);
  return 0;
}

static LRESULT on_close(HWND frame_wnd)
{
  if(get_prefs_int("ShowInSystray", 1)) { /* Close means hide if tray icon exists */
    ShowWindow(frame_wnd, SW_HIDE);
  } else {
    SendMessage(frame_wnd, WM_COMMAND, ID_APP_EXIT, 0);
  }
  return 0;
}

static LRESULT on_app_exit(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  WINDOWPLACEMENT wp;

  /* Prompt to save queue if unfinished */
  if(might_save_queue(frame_wnd)) {
    if(!IsWindowVisible(frame_wnd)) {
      ShowWindow(frame_wnd, SW_SHOW);
    }
    switch(MessageBox(frame_wnd, "Save current queue before exiting?",
                      "Confirm", MB_YESNOCANCEL|MB_ICONQUESTION)) {
    case IDCANCEL:
      return 0;
    case IDYES:
      if(save_current_queue(frame_wnd)!=0) {
        return 0; /* If saving of queue failed, don't exit */
      }
    }
  }

  /* Save window state */
  /* HACK: Must be done here and not in on_destroy() for IsWindowVisible() */
  wp.length= sizeof(wp);
  GetWindowPlacement(frame_wnd, &wp);
  if(!IsWindowVisible(frame_wnd)) {
    wp.showCmd= SW_HIDE; /* HACK: Manually save SW_HIDE */
  }
  set_prof_int("LastFrameShowCmd", wp.showCmd);
  set_prof_int("LastFrameX", wp.rcNormalPosition.left);
  set_prof_int("LastFrameY", wp.rcNormalPosition.top);
  set_prof_int("LastFrameWidth", wp.rcNormalPosition.right-wp.rcNormalPosition.left);
  set_prof_int("LastFrameHeight", wp.rcNormalPosition.bottom-wp.rcNormalPosition.top);

  DestroyWindow(info->dummy_wnd);
  PostQuitMessage(0);
  return 0;
}

static LRESULT on_initmenupopup(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  if(!HIWORD(lParam)) {
    HMENU menu;
    MENUINFO mi;

    menu= (HMENU)wParam;
    mi.cbSize= sizeof(mi);
    mi.fMask= MIM_MENUDATA;
    GetMenuInfo(menu, &mi);
    if(mi.dwMenuData) {
      return ((update_menu_t)mi.dwMenuData)(frame_wnd, info, menu);
    }
  }
  return 0;
}

static LRESULT on_taskbarcreated(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  if(get_prefs_int("ShowInSystray", 1)) { /* Re-add icon to tray */
    add_default_tray_icon(frame_wnd);
  }
  if(get_prefs_int("ShowInTaskbar", 1)) { /* Re-add to taskbar */
    ShowWindow(frame_wnd, SW_HIDE);
    ShowWindow(frame_wnd, (info->prefs_wnd)? SW_SHOWNA: SW_SHOW);
  }
  return 0;
}

LRESULT CALLBACK frame_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_MENUSELECT:
    return on_menuselect(hwnd, wParam, lParam);
  case WM_MOUSEMOVE:
    on_mousemove(hwnd, wParam, lParam);
    break;
  case WM_INITMENUPOPUP:
    return on_initmenupopup(hwnd, wParam, lParam);
  case WM_SETFOCUS:
    return on_setfocus(hwnd);
  case WM_SIZE:
    return on_size(hwnd, wParam, lParam);
  case WM_TIMER:
    switch(wParam) {
    case TIMER_UPDATE:
      return on_timer_update(hwnd);
    case TIMER_DELETEFILES: /* HACK: Pending file delete timer */
      return on_timer_deletefiles(hwnd);
    }
    break;
  case WG_SETIDLEMESSAGE:
    return on_setidlemessage(hwnd);
  case WM_SYSCOMMAND:
    switch(wParam) {
    case SC_CLOSE:
      return on_close(hwnd);
    }
    break;
  case WM_HOTKEY:
    do_showhide(hwnd);
    break;
  case WG_TRAY:
    switch(lParam) {
    case WM_LBUTTONUP:
      return on_tray_lbuttonup(hwnd);
    case WM_RBUTTONUP:
      return on_tray_rbuttonup(hwnd);
    }
    break;
  case WG_PREFS_GENERALCHANGED:
    return on_prefs_generalchanged(hwnd);
  case WG_PREFS_INTEGRATIONCHANGED:
    return on_prefs_integrationchanged(hwnd);
  case WG_PREFS_DISPLAYCHANGED:
    return on_prefs_displaychanged(hwnd);
  case WM_COMMAND:
    switch(LOWORD(wParam)) {
      /* File menu */
    case ID_FILE_OPEN:
      return on_file_open(hwnd);
    case ID_FILE_SAVE:
      return on_file_save(hwnd);
    case ID_FILE_PREFERENCES:
      return on_file_preferences(hwnd);
    case ID_FILE_OPENDOWNLOADFOLDER:
      return on_file_opendownloadfolder(hwnd);
    case ID_APP_EXIT:
      return on_app_exit(hwnd);
      /* Item menu */
    case ID_ITEM_MOVEUP:
      return on_item_moveup(hwnd);
    case ID_ITEM_MOVEDOWN:
      return on_item_movedown(hwnd);
    case ID_ITEM_PAUSE:
      return on_item_pause(hwnd);
    case ID_ITEM_FORCE:
      return on_item_force(hwnd);
    case ID_ITEM_REQUEUE:
      return on_item_requeue(hwnd);
    case ID_EDIT_CLEAR:
      return on_edit_clear(hwnd);
    case ID_ITEM_RUN:
      return on_item_run(hwnd);
    case ID_EDIT_COPY:
      return on_edit_copy(hwnd);
      /* Queue menu */
    case ID_FILE_NEW:
      return on_file_new(hwnd);
    case ID_EDIT_PASTE:
      return on_edit_paste(hwnd);
    case ID_QUEUE_CLEARALLCOMPLETE:
      return on_queue_clearallcomplete(hwnd);
    case ID_EDIT_SELECT_ALL:
      return on_edit_selectall(hwnd);
      /* Help menu */
    case ID_APP_ABOUT:
      return on_app_about(hwnd);
    case ID_HELP:
      do_help();
      return 0;
    case ID_VIEW_ALWAYSONTOP:
      return on_view_alwaysontop(hwnd);
    }
    break;
  case WM_HELP:
    do_help();
    break;
  case WM_NOTIFY:
    switch(((NMHDR *)lParam)->code) {
    case LVN_BEGINDRAG:
      on_begindrag(hwnd, wParam, lParam);
      break;
    case LVN_DELETEITEM:
      on_deleteitem(hwnd, wParam, lParam);
      break;
    }
    break;
  case WM_LBUTTONUP:
    on_lbuttonup(hwnd, wParam, lParam);
    break;
  case WM_CONTEXTMENU:
    on_contextmenu(hwnd, wParam, lParam);
    break;
  case WM_DESTROY:
    return on_destroy(hwnd);
  default:
    if(uMsg==wm_taskbarcreated) {
      return on_taskbarcreated(hwnd);
    }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK dummy_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_COPYDATA:
    return on_copydata((HWND)GetWindowLongPtr(hwnd, GWLP_USERDATA), wParam, lParam);
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void do_showhide(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  HWND wnd;

  wnd= GetForegroundWindow();
  if(wnd==frame_wnd ||
     (wnd==info->prefs_wnd &&
      IsWindowVisible(frame_wnd))) {
    ShowWindow(frame_wnd, SW_HIDE);
  } else {
    ShowWindow(frame_wnd, SW_SHOW);
    if(!info->prefs_wnd) {
      SetForegroundWindow(frame_wnd);
    }
  }
  if(info->prefs_wnd) {
    SetForegroundWindow(info->prefs_wnd);
  }
}

void do_help()
{
  char pn[MAX_PATH];

  find_whole_name(pn, sizeof(pn), "WackGet.html");
  ShellExecute(NULL, NULL, pn, NULL, NULL, SW_SHOWNORMAL);
}
