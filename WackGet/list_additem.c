
#include "pch.h"
#include "main.h"
#include "resource.h"

typedef struct {
  char *buf;
  int bufsz;
} additem_info_t;

static INT_PTR on_initdialog(HWND dlg_wnd, WPARAM wParam, LPARAM lParam)
{
  center_window(dlg_wnd, GetParent(dlg_wnd));

  SetWindowLongPtr(dlg_wnd, GWLP_USERDATA, lParam);
  return TRUE;
}

static INT_PTR on_ok(HWND dlg_wnd, WPARAM wParam, LPARAM lParam)
{
  additem_info_t *info= (additem_info_t *)GetWindowLongPtr(dlg_wnd, GWLP_USERDATA);
  int sz;

  /* Get URL & prepend http:// if none specified */
  GetWindowText(GetDlgItem(dlg_wnd, IDC_URL), info->buf, info->bufsz);
  if((sz= strlen(info->buf))>0) {
    sz++; /* NULL-terminator */
    if(info->bufsz-sz>7 &&
       strstr(info->buf, ":/")==NULL) {
      memmove(info->buf+7, info->buf, sz);
      memcpy(info->buf, "http://", 7);
    }
  }
  SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
  return TRUE;
}

static INT_PTR CALLBACK additem_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_INITDIALOG:
    return on_initdialog(hwndDlg, wParam, lParam);
  case WM_COMMAND:
    switch(LOWORD(wParam)) {
    case IDOK:
      on_ok(hwndDlg, wParam, lParam);
    case IDCANCEL:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;
    }
  }
  return FALSE;
}

LRESULT on_file_new(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  additem_info_t *myinfo;
  char buf[1024];

  myinfo= MALLOC(sizeof(additem_info_t));
  myinfo->buf= buf;
  myinfo->bufsz= sizeof(buf);
  if(DialogBoxParam(info->inst, MAKEINTRESOURCE(IDD_ADDITEM), frame_wnd,
                    additem_dlgproc, (LPARAM)myinfo)==IDOK &&
     is_supported_url(myinfo->buf)) {
    add_download_without_update(info->list_wnd, myinfo->buf, NULL);
    PostMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Update list */
  }
  FREE(myinfo);
  return 0;
}
