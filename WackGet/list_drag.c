
#include "pch.h"
#include "main.h"

int can_move_selection(HWND list_wnd, int idx_diff)
{
  int cnt, sel_item;

  if(SendMessage(list_wnd, LVM_GETSELECTEDCOUNT, 0, 0)<1) {
    return 0;
  }
  if(idx_diff==0) {
    return 1; /* Of course you can move it by zero! */
  }
  cnt= SendMessage(list_wnd, LVM_GETITEMCOUNT, 0, 0);
  sel_item= -1;
  while((sel_item= SendMessage(list_wnd, LVM_GETNEXTITEM, sel_item, 
                               MAKELPARAM(LVNI_SELECTED, 0)))!=-1) {
    if(idx_diff>0) {
      if(sel_item+idx_diff>=cnt) {
        return 0; /* Too far */
      }
    } else {
      if(sel_item+idx_diff<0) {
        return 0; /* Too near */
      }
    }
  }
  return 1; /* Within range! */
}

/* Move item in list, returns index of new item */
static int move_item(HWND list_wnd, int item, int new_idx,
                     int num_cols, char **strs, char *buf, int bufsz)
{
  int i;
  LVITEM lvi, lvi1;

  /* Copy column text first */
  lvi.pszText= buf;
  lvi.cchTextMax= bufsz;
  for(i=0; i<num_cols; i++) {
    lvi.iSubItem= i;
    if(SendMessage(list_wnd, LVM_GETITEMTEXT, item, (LPARAM)&lvi)>0) {
      strs[i]= STRDUP(lvi.pszText);
    } else {
      strs[i]= NULL;
    }
  }

  /* Copy item */
  memset(&lvi, 0, sizeof(lvi));
  lvi.mask= ~LVIF_TEXT;
  lvi.iItem= item;
  SendMessage(list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
  lvi.iItem= new_idx;
  lvi.state|= LVIS_FOCUSED|LVIS_SELECTED;
  lvi.stateMask|= LVIS_FOCUSED|LVIS_SELECTED;

  /* Switch the items as fast as possible */
  lvi1.mask= LVIF_PARAM;
  lvi1.iItem= item;
  lvi1.iSubItem= 0;
  lvi1.lParam= 0;
  SendMessage(list_wnd, LVM_SETITEM, 0, (LPARAM)&lvi1); /* Don't let on_deleteitem() free the data */
  SendMessage(list_wnd, LVM_DELETEITEM, item, 0);
  new_idx= SendMessage(list_wnd, LVM_INSERTITEM, 0, (LPARAM)&lvi);

  /* Set column text */
  for(i=0; i<num_cols; i++) {
    if(strs[i]) {
      lvi.iSubItem= i;
      lvi.pszText= strs[i];
      SendMessage(list_wnd, LVM_SETITEMTEXT, new_idx, (LPARAM)&lvi);
      FREE(strs[i]);
    }
  }
  return new_idx;
}

static int move_selection(HWND list_wnd, int idx_diff, int hot_item)
{
  int num_sel_items, *sel_items, idx, i, num_cols;
  HWND hdr_wnd;
  char buf[1024] /* TODO: Big enough? */, **strs;

  /* Get selected items */
  num_sel_items= SendMessage(list_wnd, LVM_GETSELECTEDCOUNT, 0, 0);
  if(idx_diff==0 || num_sel_items<1) {
    return (hot_item==-1)? 0: hot_item; /* Nothing to do! */
  }
  sel_items= (int *)MALLOC(sizeof(int)*num_sel_items);
  idx= -1;
  for(i=0; (idx= SendMessage(list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1; i++) {
    sel_items[i]= idx;
  }

  /* Some setup */
  hdr_wnd= (HWND)SendMessage(list_wnd, LVM_GETHEADER, 0, 0);
  num_cols= SendMessage(hdr_wnd, HDM_GETITEMCOUNT, 0, 0);
  strs= (char **)MALLOC(sizeof(char *)*num_cols);

  SendMessage(list_wnd, WM_SETREDRAW, FALSE, 0);
  if(idx_diff>0) { /* Moving down */
    for(i= num_sel_items-1; i>=0; i--) {
      idx= move_item(list_wnd, sel_items[i], sel_items[i]+idx_diff,
                     num_cols, strs, buf, sizeof(buf));
      if(hot_item!=-1 && hot_item==sel_items[i]) {
        hot_item= idx;
      }
      sel_items[i]= idx;  /* Save new index */
    }
    if(hot_item==-1) {
      SendMessage(list_wnd, LVM_ENSUREVISIBLE, sel_items[0], TRUE); /* Ensure visiblity of tail item */
    }
  } else if(idx_diff<0) { /* Moving up */
    for(i=0; i<num_sel_items; i++) {
      idx= move_item(list_wnd, sel_items[i], sel_items[i]+idx_diff,
                     num_cols, strs, buf, sizeof(buf)); /* idx_diff is negative */
      if(hot_item!=-1 && hot_item==sel_items[i]) {
        hot_item= idx;
      }
      sel_items[i]= idx;  /* Save new index */
    }
    if(hot_item==-1) {
      SendMessage(list_wnd, LVM_ENSUREVISIBLE, sel_items[num_sel_items-1], TRUE); /* Ensure visibility of tail item */
    }
  }
  if(hot_item!=-1) {
    SendMessage(list_wnd, LVM_ENSUREVISIBLE, hot_item, TRUE);
  }
  SendMessage(list_wnd, WM_SETREDRAW, TRUE, 0);

  FREE(strs);
  FREE(sel_items);

  if(hot_item!=-1) {
    return hot_item;
  }
  return 0;
}

/* TODO: Make this not static maybe? */
static int drag_item= -1;

void on_begindrag(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  NMLISTVIEW *nmlv= (NMLISTVIEW *)lParam;
  LVITEM lvi;

  drag_item= nmlv->iItem;
  SetFocus(info->list_wnd);
  SetCapture(frame_wnd);
  lvi.stateMask= LVIS_SELECTED|LVIS_FOCUSED;
  lvi.state= LVIS_SELECTED|LVIS_FOCUSED;
  SendMessage(info->list_wnd, LVM_SETITEMSTATE, drag_item, (LPARAM)&lvi);
}

void on_lbuttonup(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  if(drag_item!=-1) {
    drag_item= -1;
    ReleaseCapture();
  }
}

void on_mousemove(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  if(drag_item!=-1) {
    frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
    LVHITTESTINFO lvhti;
    int drop_item;

    lvhti.pt.x= 2;
    lvhti.pt.y= HIWORD(lParam);
    lvhti.flags= 0;
    drop_item= SendMessage(info->list_wnd, LVM_HITTEST, 0, (LPARAM)&lvhti);
    if(drop_item!=-1 &&
       drop_item!=drag_item &&
       can_move_selection(info->list_wnd, drop_item-drag_item)) {
      drag_item= move_selection(info->list_wnd, drop_item-drag_item, drag_item);
    }
  }
}

LRESULT on_item_moveup(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  move_selection(info->list_wnd, -1, -1);
  return 0;
}

LRESULT on_item_movedown(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);

  move_selection(info->list_wnd, 1, -1);
  return 0;
}
