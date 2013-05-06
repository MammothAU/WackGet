
#include "pch.h"
#include "main.h"

int parse_command_line(char *cl, node_t **out_list)
{
  char *ptr, *tok, delim;
  node_t *cur;

  delim= '\0';
  ptr= cl;
  tok= NULL;
  *out_list= cur= NULL;
  while(*ptr) {
    if(delim=='\0') {
      while(*ptr==' ' ||
            *ptr=='\t') {
        ptr++; /* Ignore whitespace between parameters */
      }
      if(*ptr=='\"') {
        delim= *ptr++; /* Next parameter is in quotes, search for close-quote */
      } else {
        tok= strpbrk(ptr, " \t"); /* Next parameter is not in quotes, just search for next whitespace */
      }
    }
    if(delim) {
      tok= strchr(ptr, delim);
      delim= '\0';
    }
    if(tok ||
       delim=='\0') {
      if(cur) {
        cur->next= MALLOC(sizeof(node_t));
        cur= cur->next;
      } else {
        *out_list= cur= MALLOC(sizeof(node_t));
      }
      cur->data= ptr;
      cur->next= NULL;
      if(tok) {
        *tok= '\0';
        ptr= tok+1;
      } else if(delim=='\0') {
        break;
      }
    } else {
      break;
    }
  }
  return (*out_list)? 0: -1;
}

void grow_buff_if_needed(char **ptr, int *cur_size, int needed_size)
{
  if(*ptr==NULL ||
     *cur_size<needed_size) {
    *cur_size= MAX_PATH*((needed_size/MAX_PATH)+1);
    *ptr= REALLOC(*ptr, *cur_size);
  }
}

static const struct {
  const char *scheme;
  int len;
} schemes[]= {
  {"http", 4},
  {"ftp", 3},
  {"https", 5},
};

int is_supported_url(const char *url)
{
  int i;
  char *tok;

  if((tok= strstr(url, "://"))==NULL) {
    return 0;
  }
  for(i=0; i<sizeof(schemes)/sizeof(schemes[0]); i++) {
    if(strnicmp(schemes[i].scheme, url, tok-url)==0) {
      return 1;
    }
  }
  return 0;
}

int get_scheme_and_host_len(const char *url)
{
  register char *tok;

  if(tok= strstr(url, "://")) {
    for(tok+=3; isalnum(*tok) || *tok=='-' || *tok=='.'; tok++);
  }
  return (tok)? tok-url: -1;
}

static void make_url(char *url, char **out_next)
{
  char *ptr;

#define INVALID_URL_CHARS "\"<> \t'" THE_NEWLINE /* TODO: Figure out if these are all */
  if(ptr= strpbrk(url, INVALID_URL_CHARS)) {
    *ptr= '\0';
  }
  if(out_next) {
    *out_next= (ptr)? ptr+1: NULL; /* Return pointer to end of parse if desired (for subsequent calls) */
  }
}

char *extract_next_supported_url(char *text, char **out_next)
{
  char *ptr;

  while(text && (ptr= strstr(text, "://"))) {
    int i;

    for(i=0; i<sizeof(schemes)/sizeof(schemes[0]); i++) {
      if(ptr-text>=schemes[i].len &&
         strnicmp(ptr-schemes[i].len, schemes[i].scheme, schemes[i].len)==0) {
        text= ptr-schemes[i].len;
        break; /* Found a supported scheme... */
      }
    }
    if(i>=sizeof(schemes)/sizeof(schemes[0])) {
      text= ptr+3; /* Skip forward */
      continue;
    }
    make_url(text, out_next); /* Found supported url */
    return text;
  }
  return NULL;
}

void extract_and_add_urls(HWND list_wnd, char *text)
{
  char *ptr, *next;

  SendMessage(list_wnd, WM_SETREDRAW, FALSE, 0); /* This operation can be big */
  for(; ptr= extract_next_supported_url(text, &next); text= next) {
    add_download_without_update(list_wnd, ptr, NULL);
  }
  SendMessage(list_wnd, WM_SETREDRAW, TRUE, 0);
  PostMessage(GetParent(list_wnd), WM_TIMER, TIMER_UPDATE, 0); /* Update list */
}

static char *strstrs(char *text, char **strs, int *cnt)
{
  register char *ptr, *ptr1, **pptr;

  for(ptr= text; *ptr; ptr++) {
    for(pptr= strs; pptr-strs<*cnt; pptr++) {
      if(*ptr==**pptr) {
        ptr1= *pptr;
        text= ptr;
        while(*(++ptr1) &&
              *(++ptr)==*ptr1);
        if(!*ptr1) {
          *cnt= pptr-strs;
          return text;
        }
        ptr= text;
      }
    }
  }
  return NULL;
}

const char **get_html_parse_substrs(int *num_strs)
{
#define SCHEME_OFFSET 2
  static const char *strs[sizeof(schemes)/sizeof(schemes[0])+SCHEME_OFFSET]= {
    "href=",
    "src=",
    /* Modified schemes start here */
  };
  int i;

  /* HACK: Free the alloc'ed modified schemes */
  if(num_strs==NULL) {
    for(i=0; i<sizeof(schemes)/sizeof(schemes[0]); i++) {
      if(strs[i+SCHEME_OFFSET]) {
        FREE((char *)strs[i+SCHEME_OFFSET]);
        (char *)strs[i+SCHEME_OFFSET]= NULL;
      }
    }
    return NULL;
  }

  /* Set up rest of strings to look for if necessary */
  if(strs[SCHEME_OFFSET]==NULL) {
    for(i=0; i<sizeof(schemes)/sizeof(schemes[0]); i++) {
      strs[i+SCHEME_OFFSET]= MALLOC(strlen(schemes[i].scheme)+2); /* HACK: Add stupid ':' to end of scheme */
      sprintf((char *)strs[i+SCHEME_OFFSET], "%s:", schemes[i].scheme);
    }
  }
  *num_strs= sizeof(schemes)/sizeof(schemes[0])+SCHEME_OFFSET;
  return strs;
}

void extract_and_add_urls_from_html(HWND list_wnd, char *html)
{
  char buf[1024], *src, *srcrootend, *srcend, *ptr, *next;
  const char **strs;
  int num_strs, i;

  /* Find source url */
  src= srcrootend= srcend= NULL;
  if(ptr= strstr(html, "SourceURL:")) {
    ptr+= 10;
    make_url(ptr, &next);
    src= ptr;
    if(next) {
      html= next;
      srcend= next-1;
    }
    if((ptr= strstr(src, "//")) &&
       (ptr= strchr(ptr+2, '/'))) {
      srcrootend= ptr+1;
    }
  }

  /* Find beginning of fragment */
  if(ptr= strstr(html, "<!--StartFragment")) {
    html= ptr+17;
  }

  /* Find all links */
  SendMessage(list_wnd, WM_SETREDRAW, FALSE, 0); /* This operation can be big */
  strs= get_html_parse_substrs(&num_strs);
  ptr= html;
  while(ptr) {
    i= num_strs;
    if((ptr= strstrs(ptr, (char **)strs, &i))==NULL) {
      break;
    }
    if(i<SCHEME_OFFSET) {
      for(ptr+= strlen(strs[i]); strchr(INVALID_URL_CHARS, *ptr); ptr++); /* Skip invalid chars */
      if(*ptr=='/') { /* Is it a url relative to the host? */
        if(srcrootend) {
          make_url(ptr+1, &next);
          memcpy(buf, src, srcrootend-src);
          strcpy(buf+(srcrootend-src), ptr+1); /* Append to source url (without current path) */
          ptr= buf;
        } else {
          ptr= NULL;
        }
      } else {
        make_url(ptr, &next);
        if(strstr(ptr, "://")==NULL) { /* Is it a url relative to the current path? */
          if(srcend) {
            memcpy(buf, src, srcend-src);
            strcpy(buf+(srcend-src), ptr); /* Append to source url + current path */
            ptr= buf;
          } else {
            ptr= NULL;
          }
        }
      }
    } else {
      make_url(ptr, &next); /* It's just a url */
    }
    if(ptr) {
      add_download_without_update(list_wnd, ptr, NULL);
    }
    ptr= next;
  }
  SendMessage(list_wnd, WM_SETREDRAW, TRUE, 0);
  PostMessage(GetParent(list_wnd), WM_TIMER, TIMER_UPDATE, 0); /* Update list */
}

int can_extract_urls_from_html(char *html)
{
  char *ptr;
  const char **strs;
  int i;

  if(ptr= strstr(html, "<!--StartFragment")) {
    html= ptr+17;
  }
  strs= get_html_parse_substrs(&i);
  return strstrs(html, (char **)strs, &i)!=NULL;
}

int can_extract_queues_from_hdrop(HDROP drop)
{
  int cnt;

  if((cnt= DragQueryFile(drop, 0xffffffff, NULL, 0))>0) {
    int i;
    char buf[MAX_PATH], *ptr;

    /* Look for .wgq files */
    for(i=0; i<cnt; i++) {
      DragQueryFile(drop, i, buf, sizeof(buf));
      if((ptr= strrchr(buf, '.')) &&
         stricmp(ptr+1, "wgq")==0) {
        return 1;
      }
    }
  }
  return 0;
}

int extract_and_add_queues_from_hdrop(HWND frame_wnd, HDROP drop)
{
  frame_info_t *info= (void *)GetWindowLongPtr(frame_wnd, GWLP_USERDATA);
  int cnt, i;
  char buf[MAX_PATH], *ptr;
  node_t *list, *cur;

  if((cnt= DragQueryFile(drop, 0xffffffff, NULL, 0))<1) {
    return -1;
  }
  list= cur= NULL;
  for(i=0; i<cnt; i++) {
    DragQueryFile(drop, i, buf, sizeof(buf));
    if((ptr= strrchr(buf, '.')) &&
       stricmp(ptr+1, "wgq")==0) {
      if(cur) {
        cur->next= MALLOC(sizeof(node_t));
        cur= cur->next;
      } else {
        list= cur= MALLOC(sizeof(node_t));
      }
      cur->data= STRDUP(buf);
      cur->next= NULL;
    }
  }
  if(list==NULL) {
    return -1;
  }
  load_queues(frame_wnd, info, list, 0);
  while(list) {
    cur= list;
    list= list->next;
    FREE(cur->data);
    FREE(cur);
  }
  return 0;
}

void center_window(HWND wnd, HWND par_wnd)
{
  RECT par_rc, rc;

  GetWindowRect(par_wnd, &par_rc);
  GetWindowRect(wnd, &rc);
  SetWindowPos(wnd,
               NULL,
               par_rc.left+((par_rc.right-par_rc.left)-(rc.right-rc.left))/2,
               par_rc.top+((par_rc.bottom-par_rc.top)-(rc.bottom-rc.top))/2,
               0, 0,
               SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_NOZORDER);
}

int add_file_assoc()
{
  HKEY key;
  char buf[MAX_PATH], *ptr;

  /* See if it already exists */
  if(RegOpenKeyEx(HKEY_CLASSES_ROOT, ".wgq", 0, KEY_READ, &key)==ERROR_SUCCESS) {
    RegCloseKey(key);
#define WGQ_NAME "WackGet Queue"
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT, WGQ_NAME, 0, KEY_READ, &key)==ERROR_SUCCESS) {
      RegCloseKey(key);
      return 0;
    }
  }

  if(SHSetValue(HKEY_CLASSES_ROOT, ".wgq", NULL, REG_SZ,
                WGQ_NAME, sizeof(WGQ_NAME)-1)!=ERROR_SUCCESS) {
    return -1;
  }
  GetModuleFileName(NULL, buf, sizeof(buf));
  ptr= buf+strlen(buf);
  strcpy(ptr, ",0");
  if(SHSetValue(HKEY_CLASSES_ROOT, WGQ_NAME"\\DefaultIcon", NULL, REG_SZ,
                buf, strlen(buf))!=ERROR_SUCCESS) {
    return -1;
  }
  strcpy(ptr, " \"%1\"");
  if(SHSetValue(HKEY_CLASSES_ROOT, WGQ_NAME"\\shell\\open\\command", NULL, REG_SZ,
                buf, strlen(buf))!=ERROR_SUCCESS) {
    return -1;
  }
  return 0;
}

void remove_file_assoc()
{
  RegDeleteKey(HKEY_CLASSES_ROOT, ".wgq");
  SHDeleteKey(HKEY_CLASSES_ROOT, WGQ_NAME);
}

void register_hot_key(HWND frame_wnd)
{
  if(!RegisterHotKey(frame_wnd, 0, MOD_WIN, 'W')) { /* TODO: Allow changing hotkey */
    MessageBox(frame_wnd, "Failed to register hot key", "Error", MB_OK|MB_ICONSTOP);
  }
}
