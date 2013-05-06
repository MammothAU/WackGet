
#include "pch.h"
#include "main.h"
#include "resource.h"

UINT wm_taskbarcreated;

static int send_command_line(HWND dummy_wnd, DWORD pid)
{
  COPYDATASTRUCT cds;
  char *ptr;
  LRESULT ret;

  /* Send command line to running WackGet */
  cds.dwData= pid;
  ptr= GetCommandLine();
  cds.cbData= 2*sizeof(long)+strlen(ptr)+1;
  cds.lpData= malloc(cds.cbData);
  *(long *)cds.lpData= 0;
  *((long *)cds.lpData+1)= 1;
  strcpy((char *)((long *)cds.lpData+2), ptr);
  ret= SendMessage(dummy_wnd, WM_COPYDATA, 0, (LPARAM)&cds);
  free(cds.lpData);
  return (ret)? 0: -1;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  HWND wnd;
  DWORD pid;
  HACCEL accel;
  MSG msg;

  /* See if WackGet's already running */
  if(wnd= find_wack_wnd(&pid)) {
    int rv;

    rv= 1;
    if(lpCmdLine &&
       lpCmdLine[0]) {
      if(send_command_line(wnd, pid)>=0) {
        rv= 0;
      }
    }
    return rv;
  }

  memory_init();

  OleInitialize(NULL); /* Needed for drag & drop */

  wm_taskbarcreated= RegisterWindowMessage("TaskbarCreated"); /* Use to regenerate tray icon if Explorer gets restarted */

  wnd= create_frame(hInstance);

  accel= LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAIN));

  while(GetMessage(&msg, NULL, 0, 0)) {
    if(!TranslateAccelerator(wnd, accel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }

  DestroyAcceleratorTable(accel);

  OleUninitialize();

  get_html_parse_substrs(NULL);
  util_cleanup();
  memory_cleanup();

  return msg.wParam;
}
