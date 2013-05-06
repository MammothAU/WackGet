
#include "pch.h"
#include "internal.h"

static const char *reg_pfx= "Software\\Millweed\\WackGet\\";

const char *get_program_dir()
{
  static char prog_dir[MAX_PATH]= "";

  if(*prog_dir=='\0') {
    HKEY key;

    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, reg_pfx, 0, KEY_QUERY_VALUE, &key)==ERROR_SUCCESS) {
      DWORD buff_size;

      buff_size= sizeof(prog_dir);
      if(RegQueryValueEx(key, NULL, NULL, NULL, (BYTE *)prog_dir, &buff_size)!=ERROR_SUCCESS) {
        *prog_dir= '\0';
      }
      RegCloseKey(key);
    }
  }
  return (*prog_dir)? prog_dir: ".";
}

char *find_whole_name(char *buff, int buff_size, const char *name)
{
  char *ptr;
  DWORD ret;

  /* Try install directory */
  if((ret= SearchPath(get_program_dir(), name, NULL, buff_size, buff, &ptr))>0 &&
     ret<=(DWORD)buff_size) {
    return buff;
  }

  /* Try default paths */
  if((ret= SearchPath(NULL, name, NULL, buff_size, buff, &ptr))>0 &&
     ret<=(DWORD)buff_size) {
    return buff;
  }

  /* Give up */
  strcpy(buff, name);
  return buff;
}

HWND find_wack_wnd(DWORD *out_pid)
{
  HWND wnd;
  DWORD pid;
  char buf[32];

  for(wnd= NULL; wnd= FindWindowEx(NULL, wnd, THE_DUMMY_CLASS, NULL); ) {
    GetWindowThreadProcessId(wnd, &pid);
    if(GetWindowText(wnd, buf, sizeof(buf))>0 &&
       pid==(DWORD)atoi(buf)) { /* Make sure pids match (sanity check) */
      break;
    }
  }
  if(wnd &&
     out_pid) {
    *out_pid= pid;
  }
  return wnd;
}

typedef enum {
  KEY_PREFERENCES,
  KEY_PROFILE,
  UTIL_CLEANUP=-1,
} key_index_t;

static HKEY get_key(key_index_t idx, int dont_create)
{
  static struct {
    const char *name;
    HKEY key;
  } keys[]= {
    {"Preferences", NULL},
    {"Profile", NULL},
  };
  if(idx==UTIL_CLEANUP) {
    int i;

    for(i=0; i<sizeof(keys)/sizeof(keys[0]); i++) {
      if(keys[i].key) {
        RegCloseKey(keys[i].key);
      }
    }
    return NULL;
  }
  if(keys[idx].key==NULL) {
    char buf[128];

    strcpy(buf, reg_pfx);
    strcat(buf, keys[idx].name);
    if(dont_create) {
      RegOpenKeyEx(HKEY_CURRENT_USER, buf, 0, KEY_READ|KEY_WRITE, &keys[idx].key);
    } else {
      RegCreateKeyEx(HKEY_CURRENT_USER, buf, 0, NULL, 0, KEY_READ|KEY_WRITE,
                     NULL, &keys[idx].key, NULL);
    }
  }
  return keys[idx].key;
}

static int get_int(key_index_t idx, const char *name, int dfl)
{
  HKEY key;
  DWORD val, sz;

  key= get_key(idx, 0);
  sz= sizeof(val);
  if(RegQueryValueEx(key, name, NULL, NULL, (LPBYTE)&val, &sz)==ERROR_SUCCESS) {
    return val;
  }
  return dfl;
}

static void get_str(key_index_t idx, const char *name, char *out_buf, int sz, const char *dfl)
{
  HKEY key;

  key= get_key(idx, 0);
  if(RegQueryValueEx(key, name, NULL, NULL, out_buf, &sz)!=ERROR_SUCCESS) {
    strncpy(out_buf, dfl, sz);
  }
}

static int get_binary(key_index_t idx, const char *name, char *out_buf, int *sz)
{
  HKEY key;

  key= get_key(idx, 0);
  if(RegQueryValueEx(key, name, NULL, NULL, out_buf, sz)==ERROR_SUCCESS) {
    return 0;
  }
  return -1;
}

static void set_int(key_index_t idx, const char *name, int val)
{
  HKEY key;

  key= get_key(idx, 0);
  RegSetValueEx(key, name, 0, REG_DWORD, (LPBYTE)&val, sizeof(val));
}

static void set_str(key_index_t idx, const char *name, const char *val)
{
  HKEY key;

  key= get_key(idx, 0);
  RegSetValueEx(key, name, 0, REG_SZ, val, strlen(val)+1);
}

static void set_binary(key_index_t idx, const char *name, char *buf, int sz)
{
  HKEY key;

  key= get_key(idx, 0);
  RegSetValueEx(key, name, 0, REG_BINARY, buf, sz);
}

int get_prefs_int(const char *name, int dfl)
{
  return get_int(KEY_PREFERENCES, name, dfl);
}

void get_prefs_str(const char *name, char *out_buf, int sz, const char *dfl)
{
  get_str(KEY_PREFERENCES, name, out_buf, sz, dfl);
}

void set_prefs_int(const char *name, int val)
{
  set_int(KEY_PREFERENCES, name, val);
}

void set_prefs_str(const char *name, const char *val)
{
  set_str(KEY_PREFERENCES, name, val);
}

int get_prof_int(const char *name, int dfl)
{
  return get_int(KEY_PROFILE, name, dfl);
}

int get_prof_binary(const char *name, char *out_buf, int *sz)
{
  return get_binary(KEY_PROFILE, name, out_buf, sz);
}

void set_prof_int(const char *name, int val)
{
  set_int(KEY_PROFILE, name, val);
}

void set_prof_binary(const char *name, char *buf, int sz)
{
  set_binary(KEY_PROFILE, name, buf, sz);
}

void util_cleanup()
{
  get_key(UTIL_CLEANUP, 0);
}
