
#include "pch.h"
#include "main.h"
#include "resource.h"

LRESULT update_queue_menu(HWND frame_wnd, frame_info_t *info, HMENU menu)
{
  MENUITEMINFO mii;
  int pref, idx, cnt;
  LVITEM lvi;
  item_info_t *iinfo;

  memset(&mii, 0, sizeof(mii));
  mii.cbSize= sizeof(mii);
  mii.fMask= MIIM_STATE;

  /* ID_EDIT_PASTE */
  mii.fState= 0;
  if(!paste(info->list_wnd, 1)) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_EDIT_PASTE, FALSE, &mii);

  /* ID_QUEUE_CLEARALLCOMPLETE */
  pref= get_prefs_int("ClearFailedToo", 0);
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_ALL, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    if(iinfo->status==STAT_DONE ||
       (pref && iinfo->status==STAT_FAILED)) {
      break;
    }
  }
  mii.fState= 0;
  if(idx==-1) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_QUEUE_CLEARALLCOMPLETE, FALSE, &mii);

  /* ID_EDIT_SELECT_ALL */
  cnt= SendMessage(info->list_wnd, LVM_GETITEMCOUNT, 0, 0);
  mii.fState= 0;
  if(cnt<1) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_EDIT_SELECT_ALL, FALSE, &mii);
  return 0;
}

LRESULT on_queue_clearallcomplete(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int pref, i;
  LVITEM lvi;
  item_info_t *iinfo;

  pref= get_prefs_int("ClearFailedToo", 0);
  SendMessage(info->list_wnd, WM_SETREDRAW, FALSE, 0);
  lvi.mask= LVIF_PARAM;
  for(i=SendMessage(info->list_wnd, LVM_GETITEMCOUNT, 0, 0)-1; i>=0; i--) { /* Reverse order */
    lvi.iItem= i;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    if(iinfo->status==STAT_DONE ||
       (pref && iinfo->status==STAT_FAILED)) {
      SendMessage(info->list_wnd, LVM_DELETEITEM, i, 0);
    }
  }
  SendMessage(info->list_wnd, WM_SETREDRAW, TRUE, 0);
  return 0;
}

LRESULT on_edit_paste(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  paste(info->list_wnd, 0);
  return 0;
}

LRESULT on_edit_selectall(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  LVITEM lvi;

  lvi.stateMask= LVIS_SELECTED;
  lvi.state= LVIS_SELECTED;
  SendMessage(info->list_wnd, LVM_SETITEMSTATE, -1, (LPARAM)&lvi);
  return 0;
}

