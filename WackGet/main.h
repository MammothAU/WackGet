
#ifndef MAIN_H
#define MAIN_H

#include "..\WGDLL\main.h"

/* Timers ids */
#define TIMER_UPDATE      0 /* Updates the list */
#define TIMER_DELETEFILES 1 /* HACK: Processes pending file deletes */

#define MAX(x, y) ((y) > (x)? (y): (x))
#define BOOLCMP(x, y) (((x) != 0) != ((y) != 0)) /* Returns 0 if boolean expressions are equal */

/* Utility list node */
typedef struct node_s node_t;
struct node_s {
  void *data;
  node_t *next;
};

/* Item status */
typedef enum {
  STAT_QUEUED,    /* Queued, waiting to start */
  STAT_GOING,     /* In progress */
  STAT_PAUSED,    /* Paused (stopped) */
  STAT_DONE,      /* Complete */
  STAT_FAILED,    /* Failure */
} status_t;

/* Item flags */
typedef enum {
  FLAG_DELETE= 0x1, /* File needs deleting */
} flags_t;

/* Item data */
typedef struct {
  HANDLE phdl;    /* Process handle */
  DWORD pid;      /* Process ID */
  char *src_url;
  status_t status;
  flags_t flags;
  struct {
    __int64 total;   /* Total size in bytes */
    __int64 start;   /* Initial byte offset */
    __int64 cur;     /* Current byte offset */
    __int64 elapsed; /* Milliseconds elapsed */
  } prog;
} item_info_t;

/* Global data */
typedef struct {
  HINSTANCE inst;
  HWND dummy_wnd;
  HWND list_wnd;
  HWND sb_wnd;
  HICON small_icon;
  HICON busy_icon;
  HMENU menu;
  HMENU tray_menu;
  HWND prefs_wnd;
  char idle_msg[128];
  node_t *to_del; /* HACK: Files to delete; TIMER_DELETEFILES processes these */
  HICON tray_icon; /* HACK: Used to indicate which icon is in the tray */
} frame_info_t;

typedef struct {
  const char *download_dir;
  const int max_threads;
  const int auto_resume;
  const int clear_completed;
  const int log;
  const int show_in_systray;
  const int tray_status;
  const char *proxy_server;
  const char *proxy_username;
  const char *proxy_password;

  int num_threads;
  int num_queued;
  __int64 total_speed;
} update_info_t;

/* Callbacks */
typedef LRESULT (*update_menu_t)(HWND frame_wnd, frame_info_t *info, HMENU menu);

/* main.c */
extern UINT wm_taskbarcreated;

/* frame.c */
LRESULT CALLBACK frame_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK dummy_wndproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void do_showhide(HWND frame_wnd);
void do_help();

/* frame_create.c */
extern UINT wm_taskbarcreated;
HWND create_frame(HINSTANCE inst);
LRESULT on_destroy(HWND frame_wnd);

/* prefs.c */
LRESULT on_prefs_generalchanged(HWND frame_wnd);
LRESULT on_prefs_integrationchanged(HWND frame_wnd);
void on_prefs_displaychanged_alwaysontop(HWND frame_wnd);
LRESULT on_prefs_displaychanged(HWND frame_wnd);

/* frame_tray.c */
int add_default_tray_icon(HWND frame_wnd);
int remove_tray_icon(HWND frame_wnd);
LRESULT on_tray_lbuttonup(HWND frame_wnd);
LRESULT on_tray_rbuttonup(HWND frame_wnd);
LRESULT on_file_preferences(HWND frame_wnd);

/* list.c */
int get_column_index(const char *name);
void create_list(HWND frame_wnd);
void destroy_list(HWND frame_wnd);
typedef struct {
  status_t status; /* Add download with this status */
  flags_t flags;   /* Add download with these flags */
} add_info_t;
int add_download_without_update(HWND list_wnd, const char *url, add_info_t *info); /* info can be NULL */
void on_deleteitem(HWND frame_wnd, WPARAM wParam, LPARAM lParam);
LRESULT on_copydata(HWND frame_wnd, WPARAM wParam, LPARAM lParam);
LRESULT on_timer_update(HWND frame_wnd);
LRESULT on_timer_deletefiles(HWND frame_wnd);

/* list_drag.c */
int can_move_selection(HWND list_wnd, int idx_diff);
void on_begindrag(HWND frame_wnd, WPARAM wParam, LPARAM lParam);
void on_lbuttonup(HWND frame_wnd, WPARAM wParam, LPARAM lParam);
void on_mousemove(HWND frame_wnd, WPARAM wParam, LPARAM lParam);
LRESULT on_item_moveup(HWND frame_wnd);
LRESULT on_item_movedown(HWND frame_wnd);

/* list_util.c */
void fix_download_dir(char *download_dir);
int make_status_text(char *out_buff, item_info_t *info);
int make_progress_text(char *out_buff, item_info_t *info);
__int64 get_speed(item_info_t *info);
__int64 get_time_left(item_info_t *info, __int64 speed);
int make_speed_text(char *out_buff, status_t status, __int64 speed, __int64 time_left);
int is_partial(item_info_t *info);
int any_partials(HWND list_wnd);
int process_pending_file_deletes(HWND frame_wnd); /* Returns 0 when all pending files are deleted */
int find_item_with_pid(HWND list_wnd, int pid);
int might_save_queue(HWND frame_wnd);
int make_tooltip_text(char *out_buff, update_info_t *uinfo);
int do_log(item_info_t *info); /* Pass NULL to close file */
int clean_up_handles_if_necessary(item_info_t *info);
int check_item_status(item_info_t *info, node_t **to_clear, int item, update_info_t *uinfo);
void set_text_if_different(HWND list_wnd, int item, int column, char *text,
                           LVITEM *lvi, char *buf, int bufsz);
void process_command_line(HWND frame_wnd, frame_info_t *info, char *cl, int always_append);

/* list_additem.c */
LRESULT on_file_new(HWND frame_wnd);

/* droptarget.c */
int create_droptarget_instance(void **out_ifc, HWND list_wnd);
int paste(HWND list_wnd, int just_query);

/* queue_save.c */
int save_current_queue(HWND frame_wnd);
LRESULT on_file_save(HWND frame_wnd);
int load_queues(HWND frame_wnd, frame_info_t *info, node_t *list, int always_append);
LRESULT on_file_open(HWND frame_wnd);

/* menu.c */
void on_contextmenu(HWND frame_wnd, WPARAM wParam, LPARAM lParam);
LRESULT update_file_menu(HWND frame_wnd, frame_info_t *info, HMENU menu);
LRESULT update_tray_menu(HWND frame_wnd, frame_info_t *info, HMENU menu);

/* item_menu.c */
LRESULT update_item_menu(HWND frame_wnd, frame_info_t *info, HMENU menu);
LRESULT on_item_pause(HWND frame_wnd);
LRESULT on_edit_clear(HWND frame_wnd);
LRESULT on_item_force(HWND frame_wnd);
LRESULT on_item_requeue(HWND frame_wnd);
LRESULT on_item_run(HWND frame_wnd);
LRESULT on_edit_copy(HWND frame_wnd);

/* queue_menu.c */
LRESULT update_queue_menu(HWND frame_wnd, frame_info_t *info, HMENU menu);
LRESULT on_queue_clearallcomplete(HWND frame_wnd);
LRESULT on_edit_paste(HWND frame_wnd);
LRESULT on_edit_selectall(HWND frame_wnd);

/* util.c */
int parse_command_line(char *cl, node_t **out_list);
void grow_buff_if_needed(char **ptr, int *cur_size, int needed_size);
int is_supported_url(const char *url);
int get_scheme_and_host_len(const char *url);
char *extract_next_supported_url(char *text, char **out_next);
const char **get_html_parse_substrs(int *num_strs);
void extract_and_add_urls(HWND list_wnd, char *text);
void extract_and_add_urls_from_html(HWND list_wnd, char *html);
int can_extract_urls_from_html(char *html);
int can_extract_queues_from_hdrop(HDROP drop);
int extract_and_add_queues_from_hdrop(HWND frame_wnd, HDROP drop);
void center_window(HWND wnd, HWND par_wnd);
int add_file_assoc();
void remove_file_assoc();
void register_hot_key(HWND frame_wnd);

/* memory.c */
extern HANDLE heap;
void memory_init();
void memory_cleanup();
#define MALLOC(x) HeapAlloc(heap, 0, x)
#define REALLOC(x, y) HeapReAlloc(heap, 0, x, y)
#define FREE(x) HeapFree(heap, 0, x)
#define STRDUP(x) strcpy(MALLOC(strlen(x)+1), x)
/* #define MALLOC(x) malloc(x) */
/* #define REALLOC(x, y) realloc(x, y) */
/* #define FREE(x) free(x) */
/* #define STRDUP(x) strdup(x) */

/* prefs_general.c */
INT_PTR CALLBACK general_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prefs_integr.c */
INT_PTR CALLBACK integration_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prefs_display.c */
INT_PTR CALLBACK display_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* prefs_proxy.c */
INT_PTR CALLBACK proxy_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

/* about.c */
INT_PTR CALLBACK about_dlgproc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#endif
