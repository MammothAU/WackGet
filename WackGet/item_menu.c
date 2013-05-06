
#include "pch.h"
#include "main.h"
#include "resource.h"

LRESULT update_item_menu(HWND frame_wnd, frame_info_t *info, HMENU menu)
{
  MENUITEMINFO mii;
  int idx, going_cnt, paused_cnt, failed_cnt, done_cnt, cnt;
  LVITEM lvi;
  item_info_t *iinfo;

  memset(&mii, 0, sizeof(mii));
  mii.cbSize= sizeof(mii);
  mii.fMask= MIIM_STATE;

  /* ID_ITEM_MOVEUP */
  mii.fState= 0;
  if(!can_move_selection(info->list_wnd, -1)) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_ITEM_MOVEUP, FALSE, &mii);

  /* ID_ITEM_MOVEDOWN */
  mii.fState= 0;
  if(!can_move_selection(info->list_wnd, 1)) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_ITEM_MOVEDOWN, FALSE, &mii);

  /* ID_ITEM_PAUSE, ID_ITEM_CLEAR, ID_ITEM_FORCE, ID_ITEM_REQUEUE, ID_ITEM_RUN, ID_EDIT_COPY, ID_FILE_SAVE */
  going_cnt= paused_cnt= failed_cnt= done_cnt= 0;
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    switch(iinfo->status) {
    case STAT_GOING:
      going_cnt++;
      break;
    case STAT_PAUSED:
      paused_cnt++;
      break;
    case STAT_FAILED:
      failed_cnt++;
      break;
    case STAT_DONE:
      done_cnt++;
      break;
    }
  }
  cnt= SendMessage(info->list_wnd, LVM_GETSELECTEDCOUNT, 0, 0);
  mii.fState= 0;
  if(paused_cnt>0) {
    mii.fState|= MFS_CHECKED;
    if(cnt!=paused_cnt) {
      mii.fState|= MFS_DISABLED;
    }
  } else if(going_cnt<1 ||
            cnt!=going_cnt) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_ITEM_PAUSE, FALSE, &mii);
  mii.fState= 0;
  if(cnt<1 ||
     going_cnt>0 ||
     done_cnt>0) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_ITEM_FORCE, FALSE, &mii);
  mii.fState= 0;
  if(cnt<1 ||
     cnt!=failed_cnt) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_ITEM_REQUEUE, FALSE, &mii);
  mii.fState= 0;
  if(cnt<1) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_EDIT_CLEAR, FALSE, &mii);
  SetMenuItemInfo(menu, ID_EDIT_COPY, FALSE, &mii);
  mii.fState= 0;
  if(done_cnt<1 ||
     cnt!=done_cnt) {
    mii.fState= MFS_DISABLED;
  }
  SetMenuItemInfo(menu, ID_ITEM_RUN, FALSE, &mii);
  return 0;
}

LRESULT on_edit_copy(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int idx, sz;
  node_t *head, *cur, *next;
  LVITEM lvi;
  item_info_t *iinfo;
  HGLOBAL glob;
  char *ptr;
  static const char sep[]= " "; /* TODO: Figure out best separator */

  /* Make a list of everything to be copied */
  head= cur= NULL;
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    if(cur) {
      cur->next= MALLOC(sizeof(node_t));
      cur= cur->next;
    } else {
      head= cur= MALLOC(sizeof(node_t));
    }
    iinfo= (item_info_t *)lvi.lParam;
    cur->data= iinfo->src_url;
    cur->next= NULL;
  }
  if(head==NULL) {
    return 0;
  }

  /* Calculate size of buffer to hold all urls */
  sz= 0;
  for(cur= head; cur; cur= cur->next) {
    if(cur!=head) {
      sz+= sizeof(sep)-1;
    }
    sz+= strlen(cur->data);
  }
  sz++; /* NULL-terminator */

  /* Make url buffer and put on clipboard */
  glob= GlobalAlloc(GMEM_MOVEABLE, sz);
  ptr= GlobalLock(glob);
  for(cur= head; cur; cur= next) {
    next= cur->next;
    if(cur!=head) {
      memcpy(ptr, sep, sizeof(sep)-1);
      ptr+= sizeof(sep)-1;
    }
    sz= strlen(cur->data);
    memcpy(ptr, cur->data, sz);
    ptr+= sz;
    FREE(cur);
  }
  *ptr= '\0';
  GlobalUnlock(glob);
  if(OpenClipboard(NULL)) {
    EmptyClipboard();
    if(SetClipboardData(CF_TEXT, glob)) {
      glob= NULL;
    }
    CloseClipboard();
  }
  if(glob) {
    GlobalFree(glob);
  }
  return 0;
}

LRESULT on_item_pause(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int idx;
  LVITEM lvi;
  item_info_t *iinfo;

  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    switch(iinfo->status) {
    case STAT_GOING:
      iinfo->status= STAT_PAUSED;
      break;
    case STAT_PAUSED:
      iinfo->status= STAT_GOING;
      break;
    }
  }
  if(iinfo) {
    PostMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Update list */
  }
  return 0;
}

LRESULT on_edit_clear(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int cnt, del, i, idx, *items;
  LVITEM lvi;
  item_info_t *iinfo;

  if((cnt= SendMessage(info->list_wnd, LVM_GETSELECTEDCOUNT, 0, 0))<1) {
    return 0; /* Nothing to do! */
  }

  /* See if user wants to delete partials */
  if((del= any_partials(info->list_wnd))>0) {
    int pref;

    if((pref= get_prefs_int("RemoveDeletesPartial", -1))!=-1) {
      del= pref; /* Use saved preference */
    } else {
      del= MessageBox(info->list_wnd,
                      (del==1)? "Delete partially downloaded file?": "Delete partially downloaded files?",
                      "Confirm", MB_YESNOCANCEL|MB_ICONQUESTION);
      if(del==IDCANCEL) {
        return 0;
      }
      del= del==IDYES;
    }
  }

  /* First, make a list of all selected items */
  items= (int *)MALLOC(sizeof(int)*cnt);
  lvi.mask= LVIF_PARAM;
  lvi.iSubItem= 0; /* TODO: Is this REALLY needed? */
  i= 0;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    items[i++]= idx;

    /* Mark partial download for deleting if desired */
    if(del) {
      lvi.iItem= idx;
      SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
      iinfo= (item_info_t *)lvi.lParam;
      if(is_partial(iinfo)) {
        iinfo->flags|= FLAG_DELETE;
      }
    }
  }

  /* Then, delete the items in reverse */
  SendMessage(info->list_wnd, WM_SETREDRAW, FALSE, 0);
  for(i=cnt-1; i>=0; i--) {
    SendMessage(info->list_wnd, LVM_DELETEITEM, (int)items[i], 0);
  }
  SendMessage(info->list_wnd, WM_SETREDRAW, TRUE, 0);
  FREE(items);
  return 0;
}

LRESULT on_item_force(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int idx;
  LVITEM lvi;
  item_info_t *iinfo;

  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    iinfo->status= STAT_GOING; /* Simply start download */
  }
  if(iinfo) {
    PostMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Update list */
  }
  return 0;
}

LRESULT on_item_requeue(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int idx;
  LVITEM lvi;
  item_info_t *iinfo;

  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    iinfo->status= STAT_QUEUED; /* Simply set status to queued */
  }
  if(iinfo) {
    PostMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Update list */
  }
  return 0;
}

LRESULT on_item_run(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int idx;
  LVITEM lvi, lvi1;
  item_info_t *iinfo;
  char buf[MAX_PATH], *ptr;

  get_prefs_str("DownloadDir", buf, sizeof(buf), "");
  if(*buf) {
    strcat(buf, "\\");
  }
  ptr= buf+strlen(buf);
  lvi.mask= LVIF_PARAM;
  lvi1.iSubItem= get_column_index("Name");
  lvi1.pszText= ptr;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    lvi1.cchTextMax= sizeof(buf)-(ptr-buf);
    SendMessage(info->list_wnd, LVM_GETITEMTEXT, idx, (LPARAM)&lvi1);
    ShellExecute(NULL, NULL, buf, NULL, NULL, SW_SHOWNORMAL);
  }
  return 0;
}
