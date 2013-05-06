
#include "pch.h"
#include "main.h"
#include "resource.h"

void on_contextmenu(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  LVHITTESTINFO lvhti;
  HMENU menu;

  lvhti.pt.x= LOWORD(lParam);
  lvhti.pt.y= HIWORD(lParam);
  ScreenToClient(info->list_wnd, &lvhti.pt);
  lvhti.flags= 0;
  if(SendMessage(info->list_wnd, LVM_HITTEST, 0, (LPARAM)&lvhti)!=-1) {
    menu= GetSubMenu(info->menu, 1); /* List item context menu */
  } else {
    menu= GetSubMenu(info->menu, 2); /* List background context menu */
  }
  ClientToScreen(info->list_wnd, &lvhti.pt);
  TrackPopupMenu(menu, 0, lvhti.pt.x, lvhti.pt.y, 0, frame_wnd, NULL);
}

LRESULT update_file_menu(HWND frame_wnd, frame_info_t *info, HMENU menu)
{
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(mii));
  mii.cbSize= sizeof(mii);
  mii.fMask= MIIM_STATE;

  /* ID_FILE_MENU */
  mii.fState= 0;
  if(SendMessage(info->list_wnd, LVM_GETITEMCOUNT, 0, 0)<1) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_FILE_SAVE, FALSE, &mii);
  return 0;
}

LRESULT update_tray_menu(HWND frame_wnd, frame_info_t *info, HMENU menu)
{
  MENUITEMINFO mii;

  memset(&mii, 0, sizeof(mii));
  mii.cbSize= sizeof(mii);
  mii.fMask= MIIM_STATE;

  /* ID_EDIT_PASTE */
  mii.fState= 0;
  if(!paste(info->list_wnd, 1)) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_EDIT_PASTE, FALSE, &mii);
  return 0;
}
