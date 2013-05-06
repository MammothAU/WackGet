
#ifndef WGDLL_MAIN_H
#define WGDLL_MAIN_H

/* strings */
#define THE_SCHEME         "wackget:"
#define THE_NEWLINE        "\r\n"
#define THE_DUMMY_CLASS    "WackGet Dummy Window"
#define THE_URL            "http://millweed.com/projects/wackget"

/* custom window messages */
#define WG_TRAY                     WM_APP+100
#define WG_SETIDLEMESSAGE           WG_TRAY+1
#define WG_PREFS_GENERALCHANGED     WG_SETIDLEMESSAGE+1
#define WG_PREFS_INTEGRATIONCHANGED WG_PREFS_GENERALCHANGED+1
#define WG_PREFS_DISPLAYCHANGED     WG_PREFS_INTEGRATIONCHANGED+1

#ifdef WGDLL_EXPORTS
#define WGDLL_API __declspec(dllexport)
#else
#define WGDLL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* util.c */
WGDLL_API const char *get_program_dir();
WGDLL_API char *find_whole_name(char *buff, int buff_size, const char *name);
WGDLL_API int get_prefs_int(const char *name, int dfl);
WGDLL_API void get_prefs_str(const char *name, char *out_buf, int sz, const char *dfl);
WGDLL_API void set_prefs_int(const char *name, int val);
WGDLL_API void set_prefs_str(const char *name, const char *val);
WGDLL_API int get_prof_int(const char *name, int dfl);
WGDLL_API int get_prof_binary(const char *name, char *out_buf, int *sz);
WGDLL_API void set_prof_int(const char *name, int val);
WGDLL_API void set_prof_binary(const char *name, char *buf, int sz);
WGDLL_API void util_cleanup();
WGDLL_API HWND find_wack_wnd(DWORD *out_pid);

#ifdef __cplusplus
}
#endif

#endif
