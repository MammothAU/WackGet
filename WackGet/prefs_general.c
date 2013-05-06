
#include "pch.h"
#include "main.h"
#include "resource.h"

static INT_PTR on_general_initdialog(HWND dlg_wnd)
{
  char buf[MAX_PATH];
  int i;

  /* Locations */
  get_prefs_str("DownloadDir", buf, sizeof(buf), "");
  SetWindowText(GetDlgItem(dlg_wnd, IDC_DOWNLOADDIR), buf);

  /* Downloads */
  sprintf(buf, "%d", get_prefs_int("MaxThreads", 0));
  SetWindowText(GetDlgItem(dlg_wnd, IDC_MAXTHREADS), buf);
  if(get_prefs_int("ClearCompleted", 0)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_CLEARCOMPLETED), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("AutoResume", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_RESUME), BM_SETCHECK, BST_CHECKED, 0);
  } else {
    SendMessage(GetDlgItem(dlg_wnd, IDC_OVERWRITE), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("ClearFailedToo", 0)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_CLEARFAILEDTOO), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("Log", 0)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_LOG), BM_SETCHECK, BST_CHECKED, 0);
  }
  i= get_prefs_int("RemoveDeletesPartial", -1);
  if(i>0) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_RM_DELETE), BM_SETCHECK, BST_CHECKED, 0);
  } else if(i==0) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_RM_LEAVE), BM_SETCHECK, BST_CHECKED, 0);
  } else {
    SendMessage(GetDlgItem(dlg_wnd, IDC_RM_PROMPT), BM_SETCHECK, BST_CHECKED, 0);
  }

  SetWindowLongPtr(dlg_wnd, GWLP_USERDATA, 1); /* Initialization complete */
  return TRUE;
}

static INT_PTR on_general_apply(HWND dlg_wnd)
{
  char buf[MAX_PATH];
  int i;

  if(!IsWindowEnabled(GetDlgItem(GetParent(dlg_wnd), ID_APPLY_NOW))) {
    return FALSE;
  }

  /* Locations */
  GetWindowText(GetDlgItem(dlg_wnd, IDC_DOWNLOADDIR), buf, sizeof(buf));
  set_prefs_str("DownloadDir", buf);

  /* Downloads */
  GetWindowText(GetDlgItem(dlg_wnd, IDC_MAXTHREADS), buf, sizeof(buf));
  set_prefs_int("MaxThreads", atoi(buf));
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_CLEARCOMPLETED), BM_GETCHECK, 0, 0);
  set_prefs_int("ClearCompleted", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_RESUME), BM_GETCHECK, 0, 0);
  set_prefs_int("AutoResume", i==BST_CHECKED); /* Don't need to store IDC_OVERWRITE; it's just AutoResume==0 */
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_CLEARFAILEDTOO), BM_GETCHECK, 0, 0);
  set_prefs_int("ClearFailedToo", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_LOG), BM_GETCHECK, 0, 0);
  set_prefs_int("Log", i==BST_CHECKED);
  i= -1;
  if(SendMessage(GetDlgItem(dlg_wnd, IDC_RM_DELETE), BM_GETCHECK, 0, 0)==BST_CHECKED) {
    i= 1;
  } else if(SendMessage(GetDlgItem(dlg_wnd, IDC_RM_LEAVE), BM_GETCHECK, 0, 0)==BST_CHECKED) {
    i= 0;
  }
  set_prefs_int("RemoveDeletesPartial", i);

  PostMessage(GetParent(GetParent(dlg_wnd)), WG_PREFS_GENERALCHANGED, 0, 0);

  SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, PSNRET_NOERROR);
  return TRUE;
}

static INT_PTR on_general_deltapos(HWND dlg_wnd, LPARAM lParam)
{
  NMUPDOWN *nmud= (void *)lParam;

  nmud->iDelta= -nmud->iDelta; /* Can't believe it; default is actually UP = decrease value */
  SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
  return TRUE;
}

static int CALLBACK browse_cb(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  if(uMsg==BFFM_SELCHANGED) {
    if(SHGetPathFromIDList((LPITEMIDLIST)lParam, (char *)lpData)) {
      SendMessage(hwnd, BFFM_ENABLEOK, 0, 1); /* Only allow things with paths! */
    } else {
      SendMessage(hwnd, BFFM_ENABLEOK, 0, 0);
    }
  }
  return 0;
}

static INT_PTR on_general_browse(HWND dlg_wnd)
{
  BROWSEINFO bi;
  char buf[MAX_PATH];
  LPITEMIDLIST pidl;

  memset(&bi, 0, sizeof(bi));
  bi.hwndOwner= dlg_wnd;
  bi.lpszTitle= "Select Download Directory...";
/*   bi.ulFlags= BIF_USENEWUI;  /\* TODO: Ask dupree bout this *\/ */
  bi.ulFlags= BIF_EDITBOX|BIF_RETURNONLYFSDIRS;
  bi.lpfn= browse_cb;
  bi.lParam= (LPARAM)buf;
  if(pidl= SHBrowseForFolder(&bi)) {
    LPMALLOC pmalloc;

    if(SHGetPathFromIDList(pidl, buf)) {
      SetWindowText(GetDlgItem(dlg_wnd, IDC_DOWNLOADDIR), buf);
    }
    SHGetMalloc(&pmalloc);
    pmalloc->lpVtbl->Free(pmalloc, pidl);
    pmalloc->lpVtbl->Release(pmalloc);
  }
  SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
  return TRUE;
}

static INT_PTR on_general_clicked(HWND dlg_wnd, WPARAM wParam)
{
  if(GetWindowLongPtr(dlg_wnd, GWLP_USERDATA)) {
    switch(LOWORD(wParam)) {
    case IDC_BROWSE:
      return on_general_browse(dlg_wnd);
    case IDC_RESUME:
    case IDC_OVERWRITE:
    case IDC_RM_DELETE:
    case IDC_RM_LEAVE:
    case IDC_RM_PROMPT:
    case IDC_CLEARCOMPLETED:
    case IDC_CLEARFAILEDTOO:
    case IDC_LOG:
      SendMessage(GetParent(dlg_wnd), PSM_CHANGED, (WPARAM)dlg_wnd, 0);
      SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
      return TRUE;
    }
  }
  return FALSE;
}

static INT_PTR on_general_change(HWND dlg_wnd, WPARAM wParam)
{
  if(GetWindowLongPtr(dlg_wnd, GWLP_USERDATA)) {
    switch(LOWORD(wParam)) {
    case IDC_DOWNLOADDIR:
    case IDC_MAXTHREADS:
      SendMessage(GetParent(dlg_wnd), PSM_CHANGED, (WPARAM)dlg_wnd, 0);
      SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
      return TRUE;
    }
  }
  return FALSE;
}

INT_PTR CALLBACK general_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_INITDIALOG:
    return on_general_initdialog(hwndDlg);
  case WM_COMMAND:
    switch(HIWORD(wParam)) {
    case BN_CLICKED:
      return on_general_clicked(hwndDlg, wParam);
    case EN_CHANGE:
      return on_general_change(hwndDlg, wParam);
    }
    break;
  case WM_NOTIFY:
    switch(((NMHDR *)lParam)->code) {
    case UDN_DELTAPOS:
      return on_general_deltapos(hwndDlg, lParam);
    case PSN_APPLY:
      return on_general_apply(hwndDlg);
    case PSN_HELP:
      do_help();
      return TRUE;
    }
    break;
  case WM_HELP:
    do_help();
    break;
  }
  return FALSE;
}
