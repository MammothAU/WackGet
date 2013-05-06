
#include "pch.h"
#include "main.h"
#include "resource.h"

typedef struct {
  HCURSOR link_cursor;
  HFONT link_font;
  WNDPROC link_wndproc;
  WNDPROC license_wndproc;
} about_info_t;

static LRESULT CALLBACK link_wndproc(HWND link_wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  about_info_t *info= (void *)GetWindowLongPtr(GetParent(link_wnd), GWLP_USERDATA);

  switch(uMsg) {
  case WM_SETCURSOR:
    SetCursor(info->link_cursor);
    return TRUE;
  }
  switch(GetDlgCtrlID(link_wnd)) {
  case IDC_LINK:
    return CallWindowProc(info->link_wndproc, link_wnd, uMsg, wParam, lParam);
  case IDC_LICENSE:
    return CallWindowProc(info->license_wndproc, link_wnd, uMsg, wParam, lParam);
  }
  return 0;
}

static INT_PTR on_initdialog(HWND dlg_wnd)
{
  about_info_t *info;

  center_window(dlg_wnd, GetParent(dlg_wnd));

  SetWindowText(GetDlgItem(dlg_wnd, IDC_VERSION), "WackGet version 1.2.4-Mammoth");

  info= MALLOC(sizeof(about_info_t));
  memset(info, 0, sizeof(*info));
  info->link_cursor= LoadCursor(NULL, IDC_HAND);

  /* Must subclass "link" statics to get notify events */
  info->link_wndproc= (WNDPROC)SetWindowLongPtr(GetDlgItem(dlg_wnd, IDC_LINK), GWLP_WNDPROC,
                                                (LONG_PTR)link_wndproc);
  info->license_wndproc= (WNDPROC)SetWindowLongPtr(GetDlgItem(dlg_wnd, IDC_LICENSE), GWLP_WNDPROC,
                                                   (LONG_PTR)link_wndproc);

  SetWindowLongPtr(dlg_wnd, GWLP_USERDATA, (LONG_PTR)info);
  return TRUE;
}

static INT_PTR on_ctlcolorstatic(HWND dlg_wnd, WPARAM wParam, LPARAM lParam)
{
  about_info_t *info= (void *)GetWindowLongPtr(dlg_wnd, GWLP_USERDATA);
  HDC hdc= (HDC)wParam;

  switch(GetDlgCtrlID((HWND)lParam)) {
  case IDC_LINK:
  case IDC_LICENSE:
    if(info->link_font==NULL) {
      HFONT font;
      LOGFONT lf;

      /* Create underline font */
      font= GetCurrentObject(hdc, OBJ_FONT);
      GetObject(font, sizeof(lf), &lf);
      lf.lfUnderline= TRUE;
      info->link_font= CreateFontIndirect(&lf);
    }

    /* Turn text blue */
    SetTextColor(hdc, 0x00ff0000);
    SetBkColor(hdc, GetSysColor(COLOR_BTNFACE)); /* TODO: Why can't I just get this from somewhere else?  And why do i have to set it? */
    SelectObject(hdc, info->link_font);

    return (BOOL)GetStockObject(NULL_BRUSH);
  }
  return FALSE;
}

static INT_PTR on_link()
{
  ShellExecute(NULL, NULL, THE_URL, NULL, NULL, SW_SHOWNORMAL);
  return TRUE;
}

static INT_PTR on_license()
{
  char buf[MAX_PATH];

  if(find_whole_name(buf, sizeof(buf), "LICENSE.txt") &&
     GetFileAttributes(buf)!=INVALID_FILE_ATTRIBUTES) {
    ShellExecute(NULL, NULL, buf, NULL, NULL, SW_SHOWNORMAL);
  }
  return TRUE;
}

static INT_PTR on_okcancel(HWND dlg_wnd, WPARAM wParam, LPARAM lParam)
{
  about_info_t *info= (void *)GetWindowLongPtr(dlg_wnd, GWLP_USERDATA);

  /* Restore default window proc before destroying */
  SetWindowLongPtr(GetDlgItem(dlg_wnd, IDC_LINK), GWLP_WNDPROC, (LONG_PTR)info->link_wndproc);
  SetWindowLongPtr(GetDlgItem(dlg_wnd, IDC_LICENSE), GWLP_WNDPROC, (LONG_PTR)info->license_wndproc);

  DestroyCursor(info->link_cursor);
  if(info->link_font) {
    DeleteObject(info->link_font);
  }
  FREE(info);

  EndDialog(dlg_wnd, LOWORD(wParam));
  return TRUE;
}

INT_PTR CALLBACK about_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_INITDIALOG:
    return on_initdialog(hwndDlg);
  case WM_CTLCOLORSTATIC:
    return on_ctlcolorstatic(hwndDlg, wParam, lParam);
  case WM_COMMAND:
    switch(HIWORD(wParam)) {
    case STN_CLICKED:
      switch(LOWORD(wParam)) {
      case IDC_LINK:
        return on_link(hwndDlg);
      case IDC_LICENSE:
        return on_license(hwndDlg);
      }
    }
    switch(LOWORD(wParam)) {
    case IDOK:
    case IDCANCEL:
      return on_okcancel(hwndDlg, wParam, lParam);
    }
  }
  return FALSE;
}
