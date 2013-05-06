
#include "pch.h"
#include "main.h"

/* HACK: wget doesn't like when DownloadDir ends in '\\' */
void fix_download_dir(char *download_dir)
{
  register char *ptr;

  ptr= download_dir+strlen(download_dir)-1;
  while(ptr>=download_dir &&
        (*ptr=='\\' ||
         *ptr=='/')) {
    *ptr--= '\0';
  }
}

int make_status_text(char *out_buff, item_info_t *info)
{
  static const char *status_text[]= {
    "Queued",
    "Downloading",
    "Paused",
    "Complete",
    "Failed",
  };
  if(info->status<0 ||
     info->status>sizeof(status_text)/sizeof(status_text[0])) {
    return -1;
  }
  strcpy(out_buff, status_text[info->status]);
  switch(info->status) {
  case STAT_FAILED:
    if(info->prog.total<0) {
      sprintf(out_buff+strlen(out_buff), " (%d)", -info->prog.total);
    }
    break;
  }
  return 0;
}

/* Returns pointer to terminating NULL; doesn't work with negative numbers */
static char *commafy(__int64 val, char *out_buff)
{
  if(val<1000) {
    sprintf(out_buff, "%ld", val);
    return out_buff+strlen(out_buff);
  }
  sprintf(out_buff= commafy(val/1000, out_buff), ",%03ld", val%1000);
  return out_buff+4;
}

int make_progress_text(char *out_buff, item_info_t *info)
{
  if(info->prog.total<1) {
    return -1;
  }
  *out_buff= '\0';
  if(info->prog.cur<info->prog.total) {
    strcpy(commafy(info->prog.cur/1024, out_buff), "K/");
  }
  sprintf(commafy(info->prog.total/1024, out_buff+strlen(out_buff)), "K; %.0f%%",
          (double)info->prog.cur/(double)info->prog.total*100.0);
  return 0;
}

__int64 get_speed(item_info_t *info)
{
  if(info->prog.elapsed<1000) { /* Don't try to calculate speed until at least a second has passed */
    return -1;
  }
  return (info->prog.cur-info->prog.start)/1024/(info->prog.elapsed/1000);
}

__int64 get_time_left(item_info_t *info, __int64 speed)
{
  if(speed<1) { /* Don't divide by zero! */
    return -1;
  }
  return (info->prog.total-info->prog.cur)/1024/speed;
}

static int make_time_left_text(char *out_buff, __int64 time_left)
{
  if(time_left<0) {
    strcpy(out_buff, "?");
  } else {
    sprintf(out_buff, "%d:%02d", time_left/60, time_left%60); /* Not commafy()ing possible thousand+ minutes */
  }
  return 0;
}

int make_speed_text(char *out_buff, status_t status, __int64 speed, __int64 time_left)
{
  if(status!=STAT_GOING) {
    *out_buff= '\0'; /* Blank if not downloading */
  } else {
    strcpy(commafy(speed, out_buff), "K/sec; ");
    make_time_left_text(out_buff+strlen(out_buff), time_left);
  }
  return 0;
}

int is_partial(item_info_t *info)
{
  return info->status!=STAT_DONE &&
    (info->prog.start>0 || info->prog.cur>0);
}

int any_partials(HWND list_wnd)
{
  int idx, ret;
  LVITEM lvi;
  item_info_t *info;

  ret= 0;
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_SELECTED, 0)))!=-1 &&
        ret<2) { /* HACK: Short-circuit at 2 partial files available */
    lvi.iItem= idx;
    SendMessage(list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    info= (item_info_t *)lvi.lParam;
    if(is_partial(info)) {
      ret++;
    }
  }
  return ret;
}

int process_pending_file_deletes(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  node_t *cur, *next;

  cur= info->to_del;
  info->to_del= NULL;
  for(; cur; cur= next) {
    next= cur->next;
    if(!DeleteFile((char *)cur->data) &&
       GetLastError()!=ERROR_FILE_NOT_FOUND) {
      cur->next= info->to_del;
      info->to_del= cur; /* Re-add failed deletes to list */
    } else {
      FREE(cur->data);
      FREE(cur);
    }
  }
  return (info->to_del)? 1: 0;
}

int find_item_with_pid(HWND list_wnd, int pid)
{
  int idx;
  LVITEM lvi;
  item_info_t *info;

  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_ALL, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    info= (item_info_t *)lvi.lParam;
    if(info->phdl!=INVALID_HANDLE_VALUE &&
       info->pid==(DWORD)pid) {
      break;
    }
  }
  return idx;
}

int might_save_queue(HWND frame_wnd)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int idx;
  LVITEM lvi;
  item_info_t *iinfo;

  SendMessage(frame_wnd, WM_TIMER, TIMER_UPDATE, 0); /* Make sure list is updated */
  lvi.mask= LVIF_PARAM;
  idx= -1;
  while((idx= SendMessage(info->list_wnd, LVM_GETNEXTITEM, idx, MAKELPARAM(LVNI_ALL, 0)))!=-1) {
    lvi.iItem= idx;
    SendMessage(info->list_wnd, LVM_GETITEM, 0, (LPARAM)&lvi);
    iinfo= (item_info_t *)lvi.lParam;
    switch(iinfo->status) {
    case STAT_QUEUED:
    case STAT_GOING:
    case STAT_PAUSED:
      return 1;
    }
  }
  return 0;
}

int make_tooltip_text(char *out_buff, update_info_t *uinfo)
{
  strcpy(commafy(uinfo->num_threads, out_buff), " downloading; ");
  strcpy(commafy(uinfo->num_queued, out_buff+strlen(out_buff)), " queued; ");
  strcpy(commafy(uinfo->total_speed, out_buff+strlen(out_buff)), "K/sec total");
  return 0;
}

int do_log(item_info_t *info)
{
  static HANDLE fh= NULL; /* TODO: Make not static? */

  if(info==NULL) {
    if(fh) {
      CloseHandle(fh);
      fh= NULL;
    }
    return 0;
  }
  if(fh==NULL) {
    char fn[MAX_PATH];

    sprintf(fn, "%s\\%s", get_program_dir(), "WackGet.log");
    if((fh= CreateFile(fn, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL))!=INVALID_HANDLE_VALUE) {
      SetFilePointer(fh, 0, NULL, FILE_END);
    } else {
      fh= NULL;
    }
  }
  if(fh) {
    DWORD num_written;

    WriteFile(fh, info->src_url, strlen(info->src_url), &num_written, NULL);
    WriteFile(fh, THE_NEWLINE, sizeof(THE_NEWLINE)-1, &num_written, NULL);
  }
  return 0;
}

int clean_up_handles_if_necessary(item_info_t *info)
{
  if(info->phdl!=INVALID_HANDLE_VALUE &&
     (info->status==STAT_DONE ||
      info->status==STAT_FAILED ||
      info->status==STAT_PAUSED)) {
    CloseHandle(info->phdl);
    info->phdl= INVALID_HANDLE_VALUE;
  }
  return 0;
}

int check_item_status(item_info_t *info, node_t **to_clear, int item, update_info_t *uinfo)
{
  if(info->phdl!=INVALID_HANDLE_VALUE &&
     (info->status==STAT_GOING || info->status==STAT_PAUSED)) {
    DWORD exit_code;

    if(GetExitCodeProcess(info->phdl, &exit_code)) {
      if(info->status==STAT_GOING &&
         exit_code!=STILL_ACTIVE) { /* Did download finish? */
        info->status= (exit_code==0)? STAT_DONE: STAT_FAILED;
        if(info->status==STAT_DONE) {
          if(uinfo->clear_completed) {
            node_t *cur;

            cur= MALLOC(sizeof(node_t));
            cur->data= (void *)item;
            cur->next= *to_clear;
            *to_clear= cur; /* Insert at beginning, becuz wanna remove in reverse order */
          }
          if(uinfo->log) {
            do_log(info);
          }
        }
      } else if(info->status==STAT_PAUSED &&
                exit_code==STILL_ACTIVE) { /* Need to pause? */
        TerminateProcess(info->phdl, 1);
        info->status= STAT_PAUSED;
      }
    }
  }
  return 0;
}

void set_text_if_different(HWND list_wnd, int item, int column, char *text,
                           LVITEM *lvi, char *buf, int bufsz)
{
  lvi->iSubItem= column;
  lvi->pszText= buf;
  lvi->cchTextMax= bufsz;
  SendMessage(list_wnd, LVM_GETITEMTEXT, item, (LPARAM)lvi);
  if(strcmp(text, buf)!=0) {
    lvi->pszText= text;
    SendMessage(list_wnd, LVM_SETITEMTEXT, item, (LPARAM)lvi);
  }
}

void process_command_line(HWND frame_wnd, frame_info_t *info, char *cl, int always_append)
{
  node_t *list, *cur, *urls, *urls_cur, *queues, *queues_cur;
  char *ptr;
  int ret;

  if(parse_command_line(cl, &list)<0) {
    return;
  }
  cur= list;
  list= list->next; /* Skip first arg */
  FREE(cur);
  if(list==NULL) {
    return;
  }

  /* Make list of urls to add and queues to load */
  urls= urls_cur= queues= queues_cur= NULL;
  while(list) {
    cur= list;
    list= list->next;
    if(is_supported_url(cur->data)) {
      if(urls_cur) {
        urls_cur->next= cur;
        urls_cur= cur;
      } else {
        urls= urls_cur= cur;
      }
      cur->next= NULL;
    } else if((ptr= strrchr(cur->data, '.')) &&
              stricmp(ptr+1, "wgq")==0) {
      if(queues_cur) {
        queues_cur->next= cur;
        queues_cur= cur;
      } else {
        queues= queues_cur= cur;
      }
      cur->next= NULL;
    } else {
      FREE(cur);
    }
  }

  /* Add urls and queues to the list */
  ret= load_queues(frame_wnd, info, queues, always_append);
  while(queues) {
    cur= queues;
    queues= queues->next;
    FREE(cur);
  }
  while(urls) {
    cur= urls;
    urls= urls->next;
    if(ret>=0) {
      add_download_without_update(info->list_wnd, cur->data, NULL);
    }
    FREE(cur);
  }
}
