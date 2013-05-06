
#include "pch.h"
#include "main.h"

static const char *column_names[]= {
  "Name",
  "Status",
  "Progress",
  "Speed",
  0
};

int get_column_index(const char *name)
{
  int i;
  
  for(i=0; i<sizeof(column_names)/sizeof(column_names[i]); i++) {
    if(*(int *)name==*(int *)column_names[i]) {
      return i;
    }
  }
  return -1;
}

void create_list(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int style, ex_style, i, cnt;
  RECT rc;
  IDropTarget *dt;
  char buf[255];
  LVCOLUMN lvc;

  /* Create list window */
  GetClientRect(frame_wnd, &rc);
  style= WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_NOSORTHEADER|LVS_NOSORTHEADER;
  ex_style= WS_EX_CLIENTEDGE;
  if(!get_prefs_int("ShowColumnHeaders", 1)) {
    style|= LVS_NOCOLUMNHEADER;
  }
  info->list_wnd= CreateWindowEx(ex_style,
                                 WC_LISTVIEW,
                                 "",
                                 style,
                                 0, 0,
                                 rc.right-rc.left, rc.bottom-rc.top,
                                 frame_wnd,
                                 NULL,
                                 NULL,
                                 NULL);

  /* Load more prefs */
  ex_style= SendMessage(info->list_wnd, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0)|
    LVS_EX_LABELTIP|LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP;
  if(get_prefs_int("ShowGridLines", 1)) {
    ex_style|= LVS_EX_GRIDLINES;
  }
  SendMessage(info->list_wnd, LVM_SETEXTENDEDLISTVIEWSTYLE, ex_style, ex_style);

  /* Insert columns */
  for(i=0; column_names[i]; i++) {
    lvc.mask= LVCF_TEXT;
    lvc.pszText= (char *)column_names[i];
    SendMessage(info->list_wnd, LVM_INSERTCOLUMN, i, (LPARAM)&lvc);
  }

  /* Restore column order */
  cnt= sizeof(buf);
  if(get_prof_binary("LastColumnOrder", buf, &cnt)==0) {
    SendMessage(info->list_wnd, LVM_SETCOLUMNORDERARRAY, cnt/sizeof(int), (LPARAM)buf);
  }

  /* Restore column widths */
  cnt= SendMessage((HWND)SendMessage(info->list_wnd, LVM_GETHEADER, 0, 0), HDM_GETITEMCOUNT, 0, 0);
  for(i=0; i<cnt; i++) {
    sprintf(buf, "LastColumn%dWidth", i);
    SendMessage(info->list_wnd, LVM_SETCOLUMNWIDTH, i, get_prof_int(buf, 75));
  }

  /* Make tooltip stay on top (when WackGet was "AlwaysOnTop" tooltip was showing up BEHIND it) */
  SetWindowPos((HWND)SendMessage(info->list_wnd, LVM_GETTOOLTIPS, 0, 0), HWND_TOPMOST,
               0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);

  /* Enable drag & drop */
  create_droptarget_instance(&dt, info->list_wnd);
  RegisterDragDrop(info->list_wnd, dt);
  dt->lpVtbl->Release(dt);
}

void destroy_list(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  HWND hdr_wnd;
  int num_cols, i, *idxs;
  char buf[255];

  KillTimer(frame_wnd, TIMER_UPDATE);
  KillTimer(frame_wnd, TIMER_DELETEFILES);
  process_pending_file_deletes(frame_wnd); /* Try one more time before exiting */

  RevokeDragDrop(info->list_wnd);

  /* Save column widths */
  hdr_wnd= (HWND)SendMessage(info->list_wnd, LVM_GETHEADER, 0, 0);
  num_cols= SendMessage(hdr_wnd, HDM_GETITEMCOUNT, 0, 0);
  for(i=0; i<num_cols; i++) {
    sprintf(buf, "LastColumn%dWidth", i);
    set_prof_int(buf, SendMessage(info->list_wnd, LVM_GETCOLUMNWIDTH, i, 0));
  }

  /* Save order of columns */
  idxs= (int *)MALLOC(sizeof(int)*num_cols);
  if(SendMessage(hdr_wnd, HDM_GETORDERARRAY, num_cols, (LPARAM)idxs)) {
    set_prof_binary("LastColumnOrder", (unsigned char *)idxs, sizeof(int)*num_cols);
  }
  FREE(idxs);
}

int add_download_without_update(HWND list_wnd, const char *url, add_info_t *info)
{
  item_info_t *iinfo;
  LVITEM lvi;

  iinfo= MALLOC(sizeof(item_info_t));
  memset(iinfo, 0, sizeof(item_info_t));

  iinfo->phdl= INVALID_HANDLE_VALUE;
  iinfo->src_url= STRDUP(url);
  if(info) {
    iinfo->status= info->status;
    iinfo->flags= info->flags;
  }

  /* Add new item to list control */
  lvi.mask= LVIF_TEXT|LVIF_PARAM;
  lvi.iItem= SendMessage(list_wnd, LVM_GETITEMCOUNT, 0, 0);
  lvi.iSubItem= 0;
  lvi.pszText= iinfo->src_url;
  lvi.lParam= (DWORD)iinfo;
  SendMessage(list_wnd, LVM_INSERTITEM, 0, (LPARAM)&lvi);
  return 0;
}

void on_deleteitem(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  NMLISTVIEW *nmlv= (NMLISTVIEW *)lParam;

  if(nmlv->lParam) {
    item_info_t *iinfo= (item_info_t *)nmlv->lParam;

    /* Make sure download is stopped */
    if(iinfo->status==STAT_GOING) {
      DWORD exit_code;

      if(GetExitCodeProcess(iinfo->phdl, &exit_code) &&
         exit_code==STILL_ACTIVE) {
        TerminateProcess(iinfo->phdl, 1);
      }
    }

    /* Delete file if necessary */
    if(iinfo->flags&FLAG_DELETE) {
      char buf[MAX_PATH], len;
      LVITEM lvi;

      /* Get local path */
      /* TODO: DownloadDir could have changed since start of download; save and use initial path */
      get_prefs_str("DownloadDir", buf, sizeof(buf)/sizeof(buf[0]), "");
      if(buf[0]) {
        strcat(buf, "\\");
      }
      len= strlen(buf);

      /* Append local filename */
      lvi.iSubItem= get_column_index("Name");
      lvi.pszText= buf+len;
      lvi.cchTextMax= sizeof(buf)-len;
      SendMessage(info->list_wnd, LVM_GETITEMTEXT, nmlv->iItem, (LPARAM)&lvi);

      if(!DeleteFile(buf) &&
         GetLastError()!=ERROR_FILE_NOT_FOUND) {
        node_t *cur;

        /* HACK: Queue the file for deletion */
        cur= MALLOC(sizeof(node_t));
        cur->data= STRDUP(buf);
        cur->next= info->to_del;
        info->to_del= cur;

        /* Kick off file delete timer */
        SetTimer(frame_wnd, TIMER_DELETEFILES, 250, NULL);
      }
    }

    /* Clean up item info */
    if(iinfo->phdl!=INVALID_HANDLE_VALUE) {
      CloseHandle(iinfo->phdl);
    }
    FREE(iinfo->src_url);
    FREE(iinfo);
  }
}

LRESULT on_copydata(HWND frame_wnd, WPARAM wParam, LPARAM lParam)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  COPYDATASTRUCT *cds= (COPYDATASTRUCT *)lParam;

  if(cds->dwData==GetCurrentProcessId() &&
     cds->cbData>sizeof(long)*2) { /* Sanity check */
    char *ptr;
    int idx;

    ptr= cds->lpData; /* Points to first long */

    /* If first long ==0, it's some message */
    if(*(long *)ptr==0) {
      ptr+= sizeof(long); /* Points to operation */
      if(*(long *)ptr==0) {        /* Op: Add item */
        int sz;

        ptr+= sizeof(long); /* Points to url */
        sz= cds->cbData-sizeof(long)*2;
        if(sz>0) {
          *(ptr+sz-1)= '\0'; /* Safety */
          if(is_supported_url(ptr)) {
            add_download_without_update(info->list_wnd, ptr, NULL);
            PostMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Update list */
          }
        }
        return TRUE;
      } else if(*(long *)ptr==1) { /* Op: Parse command line */
        int sz;

        ptr+= sizeof(long); /* Points to command line */
        sz= cds->cbData-sizeof(long)*2;
        if(sz>0) {
          *(ptr+sz-1)= '\0'; /* Safety */
          process_command_line(frame_wnd, info, ptr, 0);
        }
        return TRUE;
      }
      /* TODO: Add any additional message handlers */
    } else if((idx= find_item_with_pid(info->list_wnd, *(long *)ptr))!=-1) { /* If first long !=0, it's a wget pid */
      LVITEM lvi;
      item_info_t *iinfo;

      lvi.mask= LVIF_PARAM;
      lvi.iItem= idx;
      SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
      iinfo= (item_info_t *)lvi.lParam;

      ptr+= sizeof(long); /* Points to operation */
      if(*(long *)ptr==0) {        /* Op: Local name */
        char *tok;

        ptr+= sizeof(long); /* Points to local name */
        if((tok= strrchr(ptr, '/'))==NULL) {
          tok= strrchr(ptr, '\\');
        }
        lvi.iSubItem= 0;
        lvi.pszText= (tok)? tok+1: ptr;
        SendMessage(info->list_wnd, LVM_SETITEMTEXT, idx, (LPARAM)&lvi);
        return TRUE;
      } else if(*(long *)ptr==1) { /* Op: Total size and starting offset */
        ptr+= sizeof(long); /* Points to total size */
        iinfo->prog.total= *(__int64 *)ptr;
        ptr+= sizeof(__int64); /* Points to starting offset */
        iinfo->prog.start= iinfo->prog.cur= *(__int64 *)ptr;
        return TRUE;
      } else if(*(long *)ptr==2) { /* Op: Bytes read and time elapsed */
        ptr+= sizeof(long); /* Points to bytes read */
        iinfo->prog.cur+= *(__int64 *)ptr;
        ptr+= sizeof(__int64); /* Points to time elapsed */
        iinfo->prog.elapsed= *(__int64 *)ptr;
        return TRUE;
      } else if(*(long *)ptr==3) { /* Op: Report error */
        ptr+= sizeof(long); /* Points to error code */
        iinfo->prog.total= -*(__int64 *)ptr; /* HACK: using info->prog.total field for error */
        return TRUE;
      }
      /* TODO: Add any additional progress message handlers */
    }
  }
  return FALSE;
}

static void spawn_download(item_info_t *info, int resume, update_info_t *uinfo)
{
  static char wget_pn[MAX_PATH]= "";
  char buf[1024], *ptr;
  int len;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  BOOL ret;

  if(wget_pn[0]=='\0') {
    find_whole_name(wget_pn, sizeof(wget_pn), "WGET.EXE");
  }

  /* Set up wget command-line */
  ptr= buf;
  ptr+= sprintf(ptr, "--tries=0 --waitretry=1 --ca-certificate=cacert.pem --passive --progress=wack:%d",
                GetCurrentProcessId());
  if(uinfo->download_dir &&
     uinfo->download_dir[0]!='\0') {
    ptr+= sprintf(ptr, " --directory-prefix=\"%s\"", uinfo->download_dir);
  }
  strcpy(ptr, (resume)? " --continue": " --timestamping"); /* Always continue if unpaused */
  ptr+= strlen(ptr);
  if((len= get_scheme_and_host_len(info->src_url))>0) {
    strcpy(ptr, " --referer=");
    ptr+= strlen(ptr);
    memcpy(ptr, info->src_url, len);
    ptr+= len;
  }
  if(uinfo->proxy_server &&
     uinfo->proxy_server[0]) {
    ptr+= sprintf(ptr, " -ehttpproxy=%s -eftpproxy=%s",
                  uinfo->proxy_server, uinfo->proxy_server);
    if(uinfo->proxy_username &&
       uinfo->proxy_username[0]) {
      ptr+= sprintf(ptr, " --proxy-user=%s", uinfo->proxy_username);
    }
    if(uinfo->proxy_password &&
       uinfo->proxy_password[0]) {
      ptr+= sprintf(ptr, " --proxy-passwd=%s", uinfo->proxy_password);
    }
  }
  ptr+= sprintf(ptr, " %s", info->src_url);

  /* Attempt to spawn the download */
  memset(&si, 0, sizeof(si));
  si.cb= sizeof(si);
  /* TODO: Re-evaluate use of IDLE_PRIORITY_CLASS (was having problems under XP */
  if(ret= CreateProcess(wget_pn, buf, NULL, NULL, FALSE, DETACHED_PROCESS/* |IDLE_PRIORITY_CLASS */,
                        NULL, NULL, &si, &pi)) {
    CloseHandle(pi.hThread);
    info->phdl= pi.hProcess;
    info->pid= pi.dwProcessId;
    info->status= STAT_GOING;
    uinfo->num_threads++;
    uinfo->num_queued--;
  } else {
    info->status= STAT_FAILED;
    info->phdl= INVALID_HANDLE_VALUE;
  }
}

static void spawn_download_if_possible(item_info_t *info, update_info_t *uinfo)
{
  /* See if any items can be started */
  if(info->status==STAT_QUEUED &&
     (uinfo->max_threads==0 ||
      uinfo->max_threads-uinfo->num_threads>0)) {
    spawn_download(info, uinfo->auto_resume, uinfo); /* Start queued item */
  } else if(info->status==STAT_GOING &&
            info->phdl==INVALID_HANDLE_VALUE) {
    spawn_download(info, 1, uinfo); /* Item was unpaused or forced */
  } else {
    return; /* No download necessary */
  }

  /* See if the new download finished already */
  if(info->status==STAT_GOING) {
    DWORD exit_code;

    if(GetExitCodeProcess(info->phdl, &exit_code) &&
       exit_code!=STILL_ACTIVE) {
      info->status= (exit_code==0)? STAT_DONE: STAT_FAILED; /* Next list update will clean up handles */
      uinfo->num_threads--;
    }
  }
}

static void load_download_prefs(update_info_t *uinfo, char *ptr, int sz)
{
  get_prefs_str("DownloadDir", ptr, sz, "");
  fix_download_dir(ptr);
  uinfo->download_dir= ptr;
#define NEXT_SPOT() do { sz--; } while(*ptr++) /* Skip to next open spot in buffer */
  NEXT_SPOT();

  *(int *)&uinfo->max_threads= get_prefs_int("MaxThreads", 0);
  *(int *)&uinfo->auto_resume= get_prefs_int("AutoResume", 1);
  *(int *)&uinfo->clear_completed= get_prefs_int("ClearCompleted", 0);
  *(int *)&uinfo->log= get_prefs_int("Log", 0);
  *(int *)&uinfo->show_in_systray= get_prefs_int("ShowInSystray", 1);
  *(int *)&uinfo->tray_status= (uinfo->show_in_systray &&
                                get_prefs_int("TrayStatus", 1));

  /* Proxy prefs */
  if(get_prefs_int("Proxy", 0)) {
    get_prefs_str("Proxy_Server", ptr, sz, "");
    uinfo->proxy_server= ptr;
    NEXT_SPOT();

    get_prefs_str("Proxy_Username", ptr, sz, "");
    uinfo->proxy_username= ptr;
    NEXT_SPOT();

    get_prefs_str("Proxy_Password", ptr, sz, "");
    uinfo->proxy_password= ptr;
    NEXT_SPOT();
  }
}

LRESULT on_timer_update(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  char cfgbuf[1024], smbuf[128], smbuf1[128]; /* 128 should be plenty... */
  update_info_t uinfo;
  int idx;
  __int64 speed;
  LVITEM lvi, lvi1;
  item_info_t *iinfo;
  node_t *to_clear, *next;
  NOTIFYICONDATA nid;
  HICON desired_icon;

  memset(&uinfo, 0, sizeof(uinfo));
  load_download_prefs(&uinfo, cfgbuf, sizeof(cfgbuf));

  to_clear= NULL;
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_ALL, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;

    /* Check up on the download */
    check_item_status(iinfo, &to_clear, idx, &uinfo);

    /* Invalidate handles if item finished, etc. */
    clean_up_handles_if_necessary(iinfo);

    /* Count active downloads */
    switch(iinfo->status) {
    case STAT_GOING:
    case STAT_PAUSED:
      uinfo.num_threads++;
      break;
    case STAT_QUEUED:
      uinfo.num_queued++;
      break;
    }
  }

  /* Clear just-completed items if desired (in reverse order) */
  SendMessage(info->list_wnd, WM_SETREDRAW, FALSE, 0);
  for(; to_clear; to_clear= next) {
    next= to_clear->next;
    SendMessage(info->list_wnd, LVM_DELETEITEM, (int)to_clear->data, 0);
    FREE(to_clear);
  }
  SendMessage(info->list_wnd, WM_SETREDRAW, TRUE, 0);

  /* Update the items */
  uinfo.total_speed= 0;
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_ALL, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;

    /* Do the deed! */
    spawn_download_if_possible(iinfo, &uinfo);

    /* Update item status text */
    if(make_status_text(smbuf, iinfo)!=-1) {
      set_text_if_different(info->list_wnd, idx, get_column_index("Status"), smbuf,
                            &lvi1, smbuf1, sizeof(smbuf1));
    }

    /* Update item progress text */
    if(make_progress_text(smbuf, iinfo)!=-1) {
      set_text_if_different(info->list_wnd, idx, get_column_index("Progress"), smbuf,
                            &lvi1, smbuf1, sizeof(smbuf1));
    }

    if((speed= get_speed(iinfo))!=-1) {
      /* Update item speed text */
      if(make_speed_text(smbuf, iinfo->status, speed, get_time_left(iinfo, speed))!=-1) {
        set_text_if_different(info->list_wnd, idx, get_column_index("Speed"), smbuf,
                              &lvi1, smbuf1, sizeof(smbuf1));
      }

      /* Add item's speed to the total */
      if(iinfo->status==STAT_GOING) {
        uinfo.total_speed+= speed;
      }
    }
  }

  /* Used for tray and status bar text */
  memset(&nid, 0, sizeof(nid));
  nid.cbSize= sizeof(nid);
  make_tooltip_text(nid.szTip, &uinfo);

  /* Update tray text (if tray configured) and optionally icon */
  if(uinfo.show_in_systray) {
    nid.hWnd= frame_wnd;
    nid.uFlags= NIF_TIP;

    /* Decide which icon to use */
    desired_icon= (uinfo.tray_status &&
                   uinfo.num_threads>0)? info->busy_icon: info->small_icon;
    if(info->tray_icon!=desired_icon) {
      nid.uFlags|= NIF_ICON;
      info->tray_icon= nid.hIcon= desired_icon;
    }
    Shell_NotifyIcon(NIM_MODIFY, &nid); /* This will always correctly reset the idle icon */
  }

  /* Update status bar text */
  if(strcmp(info->idle_msg, nid.szTip)!=0) {
    strcpy(info->idle_msg, nid.szTip);
    PostMessage(frame_wnd, WG_SETIDLEMESSAGE, /* info->idle_msg */0, 0);
  }

  /* Always reset timer; once per second */
  SetTimer(frame_wnd, TIMER_UPDATE, 1000, NULL);
  return 0;
}

LRESULT on_timer_deletefiles(HWND frame_wnd)
{
  if(process_pending_file_deletes(frame_wnd)==0) { /* Returns 0 when no more */
    KillTimer(frame_wnd, TIMER_DELETEFILES);
  }
  return 0;
}
