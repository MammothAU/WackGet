
#include "pch.h"
#include "internal.h"

int dll_ref_cnt= 0;
HINSTANCE the_hinst= NULL;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  if(fdwReason==DLL_PROCESS_ATTACH) {
    char buff[MAX_PATH];

    GetModuleFileName(NULL, buff, sizeof(buff));
    if(StrRStrI(buff, NULL, "explorer.exe")!=NULL)  /* Don't attach to Windows Explorer! */
      return FALSE;
    the_hinst= hinstDLL;
  }
  else if(fdwReason==DLL_PROCESS_DETACH) {
    the_hinst= NULL;
  }
  return TRUE;
}

STDAPI DllUnregisterServer();

static const GUID CLSID_WGBrowserHelperObject=
  {0x248b131e, 0x1ea, 0x4587, {0x8e, 0xfe, 0x1d, 0x91, 0x5e, 0x14, 0x3d, 0x5e}};
static const char CLSID_WGBrowserHelperObject_str[]=
  "{248B131E-01EA-4587-8EFE-1D915E143D5E}";

STDAPI DllRegisterServer()
{
  HKEY key;
  char buff[MAX_PATH];

  /* See if IE integration is configured */
  if(!get_prefs_int("IE", 0)) {
    DllUnregisterServer();  /* No IE integration configured! */
    return E_FAIL;
  }

  /* Register the BHO */
  sprintf(buff, "CLSID\\%s", CLSID_WGBrowserHelperObject_str);
  RegCreateKeyEx(HKEY_CLASSES_ROOT, buff, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
  sprintf(buff, "WackGet Browser Helper Object");
  RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)buff, (DWORD)strlen(buff));
  RegCloseKey(key);

  sprintf(buff, "CLSID\\%s\\InprocServer32", CLSID_WGBrowserHelperObject_str);
  RegCreateKeyEx(HKEY_CLASSES_ROOT, buff, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
  GetModuleFileName(the_hinst, buff, sizeof(buff));
  RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)buff, (DWORD)strlen(buff));
  sprintf(buff, "Apartment");
  RegSetValueEx(key, "ThreadingModel", 0, REG_SZ, (BYTE *)buff, (DWORD)strlen(buff));
  RegCloseKey(key);

  sprintf(buff, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\%s", CLSID_WGBrowserHelperObject_str);
  RegCreateKeyEx(HKEY_LOCAL_MACHINE, buff, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
  RegCloseKey(key);

  /* Add/remove the IE context menu item */
  if(get_prefs_int("IE_ContextMenu", 0)) {
    if(find_whole_name(buff, sizeof(buff), "wgbho.js")!=NULL && PathFileExists(buff)) {  /* Find javascript helper file */
#define IE_CONTEXT_MENU_TEXT  "WackGet it!"
      RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\MenuExt\\"IE_CONTEXT_MENU_TEXT, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &key, NULL);
      RegSetValueEx(key, NULL, 0, REG_SZ, (BYTE *)buff, (DWORD)strlen(buff));
      RegCloseKey(key);
    }
  }
  else {
    SHDeleteKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\MenuExt\\"IE_CONTEXT_MENU_TEXT);
  }

  return S_OK;
}

STDAPI DllUnregisterServer()  /* Unregisters everything */
{
  char buff[MAX_PATH];

  SHDeleteKey(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Explorer\\MenuExt\\"IE_CONTEXT_MENU_TEXT);

  sprintf(buff, "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Browser Helper Objects\\%s", CLSID_WGBrowserHelperObject_str);
  SHDeleteKey(HKEY_LOCAL_MACHINE, buff);

  sprintf(buff, "CLSID\\%s", CLSID_WGBrowserHelperObject_str);
  SHDeleteKey(HKEY_CLASSES_ROOT, buff);

  return S_OK;
}

STDAPI DllCanUnloadNow()
{
  return (dll_ref_cnt<1)? S_OK: S_FALSE;
}

static int create_cf_instance(void **out_ifc);

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
  *ppv= NULL;
  if(IsEqualCLSID(rclsid, &CLSID_WGBrowserHelperObject)) {
    IUnknown *unk;
    HRESULT hres;

    if(create_cf_instance(&unk)<0) {
      return E_UNEXPECTED;
    }
    hres= unk->lpVtbl->QueryInterface(unk, riid, ppv);
    unk->lpVtbl->Release(unk);
    return hres;
  }
  return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT STDMETHODCALLTYPE cf_QueryInterface(void *_this, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE cf_AddRef(void *_this);
static ULONG STDMETHODCALLTYPE cf_Release(void *_this);
static HRESULT STDMETHODCALLTYPE cf_CreateInstance(void *_this, IUnknown *pUnkOuter, REFIID riid, void ** ppvObject);
static HRESULT STDMETHODCALLTYPE cf_LockServer(void *_this, BOOL fLock);

typedef struct {
  IClassFactory icf;

  int ref_cnt;
} cf_t;

static IClassFactoryVtbl cf_methods= {
  cf_QueryInterface,
  cf_AddRef,
  cf_Release,
  cf_CreateInstance,
  cf_LockServer,
};

static int create_cf_instance(void **out_ifc)
{
  cf_t *cf;

  cf= malloc(sizeof(cf_t));
  memset(cf, 0, sizeof(*cf));
  cf->icf.lpVtbl= &cf_methods;
  cf->ref_cnt= 1;
  *out_ifc= cf;
  dll_ref_cnt++;
  return 0;
}

static HRESULT STDMETHODCALLTYPE cf_QueryInterface(void *_this, REFIID riid, void **ppvObject)
{
  cf_t *this= _this;

  if(IsEqualIID(riid, &IID_IUnknown)) {
    *ppvObject= &this->icf;
    this->icf.lpVtbl->AddRef(&this->icf);
    return S_OK;
  }
  if(IsEqualIID(riid, &IID_IClassFactory)) {
    *ppvObject= &this->icf;
    this->icf.lpVtbl->AddRef(&this->icf);
    return S_OK;
  }
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE cf_AddRef(void *_this)
{
  cf_t *this= _this;

  return ++this->ref_cnt;
}

static ULONG STDMETHODCALLTYPE cf_Release(void *_this)
{
  cf_t *this= _this;

  if(--this->ref_cnt>0) {
    return this->ref_cnt;
  }
  free(this);
  return 0;
}

static HRESULT STDMETHODCALLTYPE cf_CreateInstance(void *_this, IUnknown *pUnkOuter, REFIID riid, void ** ppvObject)
{
  IUnknown *unk;
  HRESULT hres;

  if(pUnkOuter) {
    return CLASS_E_NOAGGREGATION;
  }
#if DEBUG
  AllocConsole(); /* Open a console for debug messages */
  freopen("CONOUT$", "w", stdout);
#endif
  if(create_wgbho_instance(&unk)<0) {
    return E_UNEXPECTED;
  }
  hres= unk->lpVtbl->QueryInterface(unk, riid, ppvObject);
  unk->lpVtbl->Release(unk);
  return hres;
}

static HRESULT STDMETHODCALLTYPE cf_LockServer(void *_this, BOOL fLock)
{
  return E_NOTIMPL;
}
