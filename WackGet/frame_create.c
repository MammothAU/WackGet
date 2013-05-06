
#include "pch.h"
#include "main.h"
#include "resource.h"

HWND create_frame(HINSTANCE inst)
{
  frame_info_t *info;
  HICON large_icon;
  MENUINFO mi;
  WNDCLASSEX wcx;
  char buf[255];
  HWND frame_wnd;
  int style, ex_style;

  info= MALLOC(sizeof(frame_info_t));
  memset(info, 0, sizeof(*info));
  info->inst= inst;

  /* Load some resources */
  large_icon= LoadImage(inst, MAKEINTRESOURCE(IDR_MAIN), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
  info->small_icon= LoadImage(inst, MAKEINTRESOURCE(IDR_MAIN), IMAGE_ICON,
                              GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CXSMICON),
                              LR_DEFAULTSIZE);
  info->busy_icon= (HICON)LoadImage(inst, MAKEINTRESOURCE(IDI_BUSY), IMAGE_ICON,
                                    GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CXSMICON),
                                    LR_DEFAULTCOLOR);
  info->menu= LoadMenu(inst, MAKEINTRESOURCE(IDR_MAIN));
  info->tray_menu= LoadMenu(inst, MAKEINTRESOURCE(IDM_TRAY));

  /* Set the appropriate callbacks to update each menu */
  mi.cbSize= sizeof(mi);
  mi.fMask= MIM_MENUDATA;
  mi.dwMenuData= (ULONG_PTR)update_file_menu;
  SetMenuInfo(GetSubMenu(info->menu, 0), &mi);
  mi.dwMenuData= (ULONG_PTR)update_item_menu;
  SetMenuInfo(GetSubMenu(info->menu, 1), &mi);
  mi.dwMenuData= (ULONG_PTR)update_queue_menu;
  SetMenuInfo(GetSubMenu(info->menu, 2), &mi);
  mi.dwMenuData= (ULONG_PTR)update_tray_menu;
  SetMenuInfo(info->tray_menu, &mi);

  /* Create invisible parent window to remove frame from task bar */
  memset(&wcx, 0, sizeof(wcx));
  wcx.cbSize= sizeof(wcx);
  wcx.lpfnWndProc= dummy_wndproc;
  wcx.hIcon= large_icon;
  wcx.lpszClassName= THE_DUMMY_CLASS;
  sprintf(buf, "%d", GetCurrentProcessId()); /* Set frame title to current process id (for external processes to send status messages) */
  info->dummy_wnd= CreateWindowEx(0,
                                  (const char *)RegisterClassEx(&wcx),
                                  buf,
                                  0,
                                  0, 0, 0, 0,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);

  /* Create main frame */
  memset(&wcx, 0, sizeof(wcx));
  wcx.cbSize= sizeof(wcx);
  wcx.style= CS_PARENTDC|CS_SAVEBITS;
  wcx.lpfnWndProc= frame_wndproc;
  wcx.hIcon= large_icon;
  wcx.lpszClassName= "WackGet Main Frame";
  wcx.hIconSm= info->small_icon;
  style= WS_OVERLAPPEDWINDOW;
  ex_style= WS_EX_RIGHTSCROLLBAR;
  if(get_prefs_int("AlwaysOnTop", 0)) {
    ex_style|= WS_EX_TOPMOST;
  }
  if(get_prefs_int("ShowInTaskbar", 1)) {
    ex_style|= WS_EX_APPWINDOW;
  } else {
    style&= ~WS_MINIMIZEBOX; /* Disable minimize button if not in taskbar */
  }
  frame_wnd= CreateWindowEx(ex_style,
                            (const char *)RegisterClassEx(&wcx),
                            "WackGet",
                            style,
                            get_prof_int("LastFrameX", CW_USEDEFAULT),
                            get_prof_int("LastFrameY", CW_USEDEFAULT),
                            get_prof_int("LastFrameWidth", 450),
                            get_prof_int("LastFrameHeight", 180),
                            info->dummy_wnd,
                            (get_prefs_int("ShowMenuBar", 1))? info->menu: NULL,
                            NULL,
                            NULL);
  SetWindowLongPtr(frame_wnd, GWLP_USERDATA, (LONG_PTR)info);

  /* HACK: Using dummy window to catch WM_COPYDATA events */
  SetWindowLongPtr(info->dummy_wnd, GWLP_USERDATA, (LONG_PTR)frame_wnd);

  create_list(frame_wnd);

  /* Create status bar */
  style= WS_CHILD;
  if(get_prefs_int("ShowStatusBar", 1)) {
    style|= WS_VISIBLE;
  }
  info->sb_wnd= CreateWindowEx(0,
                               STATUSCLASSNAME,
                               NULL,
                               style,
                               0, 0, 0, 0,
                               frame_wnd,
                               NULL,
                               NULL,
                               NULL);

  /* Load even more prefs */
  if(get_prefs_int("ShowInSystray", 1)) {
    add_default_tray_icon(frame_wnd);
  }
  if(get_prefs_int("HotKey", 0)) {
    register_hot_key(frame_wnd);
  }

  SetActiveWindow(info->list_wnd);
  ShowWindow(frame_wnd, get_prof_int("LastFrameShowCmd", SW_SHOWNORMAL));
  UpdateWindow(frame_wnd);

  process_command_line(frame_wnd, info, GetCommandLine(), 1); /* Add command line parms to list */

  PostMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Update list (start timer) */
  return frame_wnd;
}

LRESULT on_destroy(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  destroy_list(frame_wnd);

  do_log(NULL); /* Close log file */

  remove_tray_icon(frame_wnd); /* Always remove tray icon, even if it ain't there */
  UnregisterHotKey(frame_wnd, 0); /* Always unregister hot key, even if it ain't registered */

  DestroyIcon(info->small_icon);
  DestroyIcon(info->busy_icon);
  DestroyMenu(info->menu);
  DestroyMenu(info->tray_menu);
  FREE(info);
  return 0;
}
