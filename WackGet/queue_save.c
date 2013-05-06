
#include "pch.h"
#include "main.h"

static int get_save_pathname(HWND hwnd, char *buff, int buff_size)
{
  OPENFILENAME ofn;

  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize= sizeof(ofn);
  ofn.hwndOwner= hwnd;
  ofn.lpstrFilter= "WackGet Queues (*.wgq)\0*.wgq\0\0";
  strcpy(buff, "*.wgq");
  ofn.lpstrFile= buff;
  ofn.nMaxFile= buff_size;
  ofn.lpstrTitle= "Save Current Queue";
  ofn.Flags= OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
  ofn.lpstrDefExt= "wgq";
  return (GetSaveFileName(&ofn))? 0: -1;
}

int save_current_queue(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  char pn[MAX_PATH];
  HANDLE fh;
  DWORD num_written;
  int val, idx;
  LVITEM lvi;
  item_info_t *iinfo;

  if(get_save_pathname(frame_wnd, pn, sizeof(pn))==-1) {
    return -1;
  }
  if((fh= CreateFile(pn, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                     NULL))==INVALID_HANDLE_VALUE) {
    MessageBox(frame_wnd, "Failed to create %s", "Error", MB_OK|MB_ICONSTOP);
    return -1;
  }

  /* Write version */
#define QUEUE_VERSION 12
  val= QUEUE_VERSION;
  WriteFile(fh, &val, sizeof(val), &num_written, NULL); /* Version (.wgq format might change) */

  /* Write item records */
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_ALL, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    val= strlen(iinfo->src_url);
    WriteFile(fh, &val, sizeof(val), &num_written, NULL); /* Length of source url */
    WriteFile(fh, iinfo->src_url, val, &num_written, NULL); /* Source url */
    WriteFile(fh, &iinfo->status, sizeof(iinfo->status), &num_written, NULL); /* Item status */
    WriteFile(fh, &iinfo->flags, sizeof(iinfo->flags), &num_written, NULL); /* Item flags */
  }
  CloseHandle(fh);
  return 0;
}

LRESULT on_file_save(HWND frame_wnd)
{
  save_current_queue(frame_wnd);
  return 0;
}

static int get_load_filenames(HWND hwnd, char *buf, int buf_size, node_t **out_list)
{
  OPENFILENAME ofn;
  node_t *cur;

  memset(&ofn, 0, sizeof(ofn));
  ofn.lStructSize= sizeof(ofn);
  ofn.hwndOwner= hwnd;
  ofn.lpstrFilter= "WackGet Queues (*.wgq)\0*.wgq\0\0";
  *buf= '\0';
  ofn.lpstrFile= buf;
  ofn.nMaxFile= buf_size-1;
  ofn.lpstrTitle= "Load Queue";
  ofn.Flags= OFN_ALLOWMULTISELECT|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
  if(!GetOpenFileName(&ofn)) {
    return -1;
  }
  *out_list= cur= NULL;
  for(buf+= ofn.nFileOffset; *buf; buf+= strlen(buf)+1) {
    if(cur) {
      cur->next= MALLOC(sizeof(node_t));
      cur= cur->next;
    } else {
      *out_list= cur= MALLOC(sizeof(node_t));
    }
    cur->data= buf;
    cur->next= NULL;
  }
  return (*out_list)? 0: -1;
}

int load_queues(HWND frame_wnd, frame_info_t *info, node_t *list, int always_append)
{
  char errbuf[MAX_PATH], *buf;
  int bufsz, val;
  HANDLE fh;
  DWORD num_read;
  add_info_t ainfo;
  int rv;

  rv= 0;
  buf= NULL;
  for(; list; list= list->next) {
    /* See if file can be read */
    if((fh= CreateFile(list->data, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL, NULL))==INVALID_HANDLE_VALUE ||
       !ReadFile(fh, &val, sizeof(val), &num_read, NULL) ||
       num_read!=sizeof(val) ||
       val>QUEUE_VERSION) { /* Check version */
      if(fh!=INVALID_HANDLE_VALUE) {
        CloseHandle(fh);
      }
      sprintf(errbuf, "Failed to load %s\n\nPress OK to continue or Cancel to abort.", list->data);
      if(MessageBox(frame_wnd, errbuf, "Error", MB_OKCANCEL|MB_ICONSTOP)==IDCANCEL) {
        rv= -1;
        goto done;
      }
      continue;
    }

    /* Check whether should append or not */
    if(!always_append) {
      if(SendMessage(info->list_wnd, LVM_GETITEMCOUNT, 0, 0)<1) {
        always_append= 1; /* If current queue is empty, append everything */
      } else {
        if((val= MessageBox(frame_wnd, "Append to current queue?", "Confirm",
                            MB_YESNOCANCEL|MB_ICONQUESTION))==IDCANCEL) {
          CloseHandle(fh);
          rv= -1;
          goto done;
        } else if(val!=IDNO) {
          always_append= 1;
        }
      }
    }

    /* Clear the current queue if not appending */
    SendMessage(info->list_wnd, WM_SETREDRAW, FALSE, 0); /* This operation can be big */
    if(!always_append) {
      SendMessage(info->list_wnd, LVM_DELETEALLITEMS, 0, 0);
      always_append= 1; /* Append everything now */
    }

    /* Read the queue */
    while(ReadFile(fh, &val, sizeof(val), &num_read, NULL) &&
          num_read==sizeof(val)) {
      if(buf==NULL) {
        bufsz= MAX(val+1, MAX_PATH);
        buf= MALLOC(bufsz);
      }
      grow_buff_if_needed(&buf, &bufsz, val+1); /* Need room for url + NULL */
      if(!ReadFile(fh, buf, val, &num_read, NULL) ||
         (int)num_read!=val) {
        break;
      }
      buf[val]= '\0'; /* NULL-terminate */
      memset(&ainfo, 0, sizeof(ainfo));
      if(!ReadFile(fh, &ainfo.status, sizeof(ainfo.status), &num_read, NULL) ||
         num_read!=sizeof(ainfo.status)) {
        break;
      }
      if(!ReadFile(fh, &ainfo.flags, sizeof(ainfo.flags), &num_read, NULL) ||
         num_read!=sizeof(ainfo.flags)) {
        break;
      }
      add_download_without_update(info->list_wnd, buf, &ainfo);
    }
    CloseHandle(fh);
    SendMessage(info->list_wnd, WM_SETREDRAW, TRUE, 0);

    PostMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Update list */
  }
 done:
  if(buf) {
    FREE(buf);
  }
  return rv;
}

LRESULT on_file_open(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  char buf[MAX_PATH]; /* TODO: Big enough? */
  node_t *list, *cur;

  if(get_load_filenames(frame_wnd, buf, sizeof(buf), &list)==-1) {
    return 0;
  }
  load_queues(frame_wnd, info, list, 0);
  while(list) {
    cur= list;
    list= list->next;
    FREE(cur);
  }
  return 0;
}
