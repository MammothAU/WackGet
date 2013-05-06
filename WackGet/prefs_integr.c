
#include "pch.h"
#include "main.h"
#include "resource.h"

static void update_ie_controls(HWND dlg_wnd)
{
  int i;

  i= SendMessage(GetDlgItem(dlg_wnd, IDC_IE), BM_GETCHECK, 0, 0);
  EnableWindow(GetDlgItem(dlg_wnd, IDC_IE_CONTEXT_MENU), i==BST_CHECKED);
  EnableWindow(GetDlgItem(dlg_wnd, IDC_IE_KEYBOARD_CLICK), i==BST_CHECKED);
  EnableWindow(GetDlgItem(dlg_wnd, IDC_CLICK_PLUS_LABEL), i==BST_CHECKED);
  EnableWindow(GetDlgItem(dlg_wnd, IDC_CLICK_PLUS_FIELD), i==BST_CHECKED);
}

static INT_PTR on_integration_initdialog(HWND dlg_wnd)
{
  HKEY key;

  /* IE integration */
  if(get_prefs_int("IE", 0)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_IE), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("IE_ContextMenu", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_IE_CONTEXT_MENU), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("IE_KeyboardClick", 1)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_IE_KEYBOARD_CLICK), BM_SETCHECK, BST_CHECKED, 0);
  }
  update_ie_controls(dlg_wnd);

  /* File association */
  if(RegOpenKeyEx(HKEY_CLASSES_ROOT, ".wgq", 0, KEY_READ, &key)==ERROR_SUCCESS) {
    RegCloseKey(key);
    SendMessage(GetDlgItem(dlg_wnd, IDC_ASSOCIATE), BM_SETCHECK, BST_CHECKED, 0);
  }
  if(get_prefs_int("HotKey", 0)) {
    SendMessage(GetDlgItem(dlg_wnd, IDC_HOTKEY), BM_SETCHECK, BST_CHECKED, 0);
  }

  SetWindowLongPtr(dlg_wnd, GWLP_USERDATA, 1); /* Initialization complete */
  return TRUE;
}

static INT_PTR on_integration_clicked(HWND dlg_wnd, WPARAM wParam)
{
  if(GetWindowLongPtr(dlg_wnd, GWLP_USERDATA)) {
    switch(LOWORD(wParam)) {
    case IDC_IE:
      update_ie_controls(dlg_wnd);
    case IDC_IE_CONTEXT_MENU:
    case IDC_IE_KEYBOARD_CLICK:
    case IDC_ASSOCIATE:
    case IDC_HOTKEY:
      SendMessage(GetParent(dlg_wnd), PSM_CHANGED, (WPARAM)dlg_wnd, 0);
      SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, 0);
      return TRUE;
    }
  }
  return FALSE;
}

static INT_PTR on_integration_apply(HWND dlg_wnd)
{
  int i, ie;

  if(!IsWindowEnabled(GetDlgItem(GetParent(dlg_wnd), ID_APPLY_NOW))) {
    return FALSE;
  }

  /* IE integration */
  ie= SendMessage(GetDlgItem(dlg_wnd, IDC_IE), BM_GETCHECK, 0, 0);
  set_prefs_int("IE", ie==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_IE_CONTEXT_MENU), BM_GETCHECK, 0, 0);
  set_prefs_int("IE_ContextMenu", i==BST_CHECKED);
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_IE_KEYBOARD_CLICK), BM_GETCHECK, 0, 0);
  set_prefs_int("IE_KeyboardClick", i==BST_CHECKED);
  if(ie==BST_CHECKED) { /* Apply the settings changes */
    if(DllRegisterServer()!=S_OK) {
      MessageBox(dlg_wnd, "Failed to register WackGet.dll", "Error", MB_OK|MB_ICONSTOP);
    }
  } else {
    if(DllUnregisterServer()!=S_OK) {
      MessageBox(dlg_wnd, "Failed to unregister WackGet.dll", "Error", MB_OK|MB_ICONSTOP);
    }
  }

  /* File association */
  if(SendMessage(GetDlgItem(dlg_wnd, IDC_ASSOCIATE), BM_GETCHECK, 0, 0)==BST_CHECKED) {
    if(add_file_assoc()<0) {
      remove_file_assoc();
      MessageBox(dlg_wnd, "Failed to add .wgq file association", "Error", MB_OK|MB_ICONSTOP);
    }
  } else {
    remove_file_assoc();
  }
  i= SendMessage(GetDlgItem(dlg_wnd, IDC_HOTKEY), BM_GETCHECK, 0, 0);
  set_prefs_int("HotKey", i==BST_CHECKED);

  PostMessage(GetParent(GetParent(dlg_wnd)), WG_PREFS_INTEGRATIONCHANGED, 0, 0);

  SetWindowLongPtr(dlg_wnd, DWLP_MSGRESULT, PSNRET_NOERROR);
  return TRUE;
}

INT_PTR CALLBACK integration_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_INITDIALOG:
    return on_integration_initdialog(hwndDlg);
  case WM_COMMAND:
    switch(HIWORD(wParam)) {
    case BN_CLICKED:
      return on_integration_clicked(hwndDlg, wParam);
    }
    break;
  case WM_NOTIFY:
    switch(((NMHDR *)lParam)->code) {
    case PSN_APPLY:
      return on_integration_apply(hwndDlg);
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
