
#include "pch.h"
#include "internal.h"

static HRESULT STDMETHODCALLTYPE ows_QueryInterface(void *_this, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE ows_AddRef(void *_this);
static ULONG STDMETHODCALLTYPE ows_Release(void *_this);
static HRESULT STDMETHODCALLTYPE ows_SetSite(void *_this, IUnknown *pUnkSite);
static HRESULT STDMETHODCALLTYPE ows_GetSite(void *_this, REFIID riid, void **ppvSite);

static HRESULT STDMETHODCALLTYPE disp_QueryInterface(void *_this, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE disp_AddRef(void *_this);
static ULONG STDMETHODCALLTYPE disp_Release(void *_this);
static HRESULT STDMETHODCALLTYPE disp_GetTypeInfoCount(void *_this, unsigned int FAR *pctinfo);
static HRESULT STDMETHODCALLTYPE disp_GetTypeInfo(void *_this, unsigned int iTInfo, LCID lcid, ITypeInfo FAR *FAR *ppTInfo);
static HRESULT STDMETHODCALLTYPE disp_GetIDsOfNames(void *_this, REFIID riid, OLECHAR FAR *FAR *rgszNames, 
                                                    unsigned int cNames, LCID lcid, DISPID FAR *rgDispId);
static HRESULT STDMETHODCALLTYPE disp_Invoke(void *_this, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                                             DISPPARAMS FAR *pDispParams, VARIANT FAR *pVarResult, EXCEPINFO FAR *pExcepInfo,
                                             unsigned int FAR *puArgErr);

typedef struct {
  IObjectWithSite iows;
  IDispatch idisp;

  int ref_cnt;
  IWebBrowser *site;
  IConnectionPoint *cp;
  DWORD cookie;
} wgbho_t;

static IObjectWithSiteVtbl ows_methods= {
  ows_QueryInterface,
  ows_AddRef,
  ows_Release,
  ows_SetSite,
  ows_GetSite,
};

static IDispatchVtbl disp_methods= {
  disp_QueryInterface,
  disp_AddRef,
  disp_Release,
  disp_GetTypeInfoCount,
  disp_GetTypeInfo,
  disp_GetIDsOfNames,
  disp_Invoke,
};

int create_wgbho_instance(void **out_ifc)
{
  wgbho_t *wgbho;

  wgbho= malloc(sizeof(wgbho_t));
  memset(wgbho, 0, sizeof(*wgbho));
  wgbho->iows.lpVtbl= &ows_methods;
  wgbho->idisp.lpVtbl= &disp_methods;
  wgbho->ref_cnt= 1;
  *out_ifc= wgbho;
  dll_ref_cnt++;
  return 0;
}

static HRESULT STDMETHODCALLTYPE ows_QueryInterface(void *_this, REFIID riid, void **ppvObject)
{
  wgbho_t *this= _this;

  if(IsEqualIID(riid, &IID_IUnknown)) {
    *ppvObject= &this->iows;
    this->iows.lpVtbl->AddRef(&this->iows);
    return S_OK;
  }
  if(IsEqualIID(riid, &IID_IObjectWithSite)) {
    *ppvObject= &this->iows;
    this->iows.lpVtbl->AddRef(&this->iows);
    return S_OK;
  }
  if(IsEqualIID(riid, &IID_IDispatch)) {
    *ppvObject= &this->idisp;
    this->idisp.lpVtbl->AddRef(&this->idisp);
    return S_OK;
  }
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE ows_AddRef(void *_this)
{
  wgbho_t *this= _this;

  return ++this->ref_cnt;
}

static ULONG STDMETHODCALLTYPE ows_Release(void *_this)
{
  wgbho_t *this= _this;

  if(--this->ref_cnt>0) {
    return this->ref_cnt;
  }
  this->iows.lpVtbl->SetSite(&this->iows, NULL);
  free(this);
  return 0;
}

static HRESULT STDMETHODCALLTYPE ows_SetSite(void *_this, IUnknown *pUnkSite)
{
  wgbho_t *this= _this;

  if(this->cp) {
    this->cp->lpVtbl->Unadvise(this->cp, this->cookie);
    this->cp->lpVtbl->Release(this->cp);
    this->cp= NULL;
  }
  if(this->site) {
    this->site->lpVtbl->Release(this->site);
    this->site= NULL;
  }
  if(pUnkSite==NULL ||
     pUnkSite->lpVtbl->QueryInterface(pUnkSite, &IID_IWebBrowser, (void **)&this->site)!=S_OK) {
    this->site= NULL;
  } else {
    IConnectionPointContainer *cpc;

    if(this->site->lpVtbl->QueryInterface(this->site, &IID_IConnectionPointContainer, (void **)&cpc)==S_OK) {
      if(cpc->lpVtbl->FindConnectionPoint(cpc, &DIID_DWebBrowserEvents2, &this->cp)==S_OK ||  /* Try IE 4.0+ events first */
        cpc->lpVtbl->FindConnectionPoint(cpc, &DIID_DWebBrowserEvents, &this->cp)==S_OK) {    /* Then try IE <4.0 events */
        this->cp->lpVtbl->Advise(this->cp, (IUnknown *)&this->idisp, &this->cookie);
      }
      cpc->lpVtbl->Release(cpc);
    }
  }
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE ows_GetSite(void *_this, REFIID riid, void **ppvSite)
{
  wgbho_t *this= _this;

  if(this->site==NULL) {
    return E_FAIL;
  }
  return this->site->lpVtbl->QueryInterface(this->site, riid, ppvSite);
}

static HRESULT STDMETHODCALLTYPE disp_QueryInterface(void *_this, REFIID riid, void **ppvObject)
{
  return ows_QueryInterface((void **)_this-1, riid, ppvObject);
}

static ULONG STDMETHODCALLTYPE disp_AddRef(void *_this)
{
  return ows_AddRef((void **)_this-1);
}

static ULONG STDMETHODCALLTYPE disp_Release(void *_this)
{
  return ows_Release((void **)_this-1);
}

static HRESULT STDMETHODCALLTYPE disp_GetTypeInfoCount(void *_this, unsigned int FAR *pctinfo)
{
  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE disp_GetTypeInfo(void *_this, unsigned int iTInfo, LCID lcid, ITypeInfo FAR *FAR *ppTInfo)
{
  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE disp_GetIDsOfNames(void *_this, REFIID riid, OLECHAR FAR *FAR *rgszNames, 
                                                    unsigned int cNames, LCID lcid, DISPID FAR *rgDispId)
{
  return E_NOTIMPL;
}

static HWND get_wack_hwnd(DWORD *out_pid)
{
  static HWND wack_hwnd= NULL;
  static DWORD wack_pid= -1;

  if(wack_hwnd &&
     !IsWindow(wack_hwnd)) {
    wack_hwnd= NULL; /* If the previous wack hwnd is gone, find a new one */
  }
  if(wack_hwnd==NULL) {
    wack_hwnd= find_wack_wnd(&wack_pid);
  }
  if(wack_hwnd &&
     out_pid) {
    *out_pid= wack_pid;
  }
  return wack_hwnd;
}

static int send_wack_url(const char *url)
{
  DWORD pid;
  HWND hwnd;
  COPYDATASTRUCT cds;
  LRESULT res;

  if(url==NULL || *url=='\0') {
    return -1;
  }
  if((hwnd= get_wack_hwnd(&pid))==NULL) {
    return -1;
  }

#if DEBUG
  printf("send_wack_url(%s)\n", url);
#endif

  cds.dwData= pid;
  cds.cbData= 2*sizeof(long)+strlen(url)+1;
  cds.lpData= malloc(cds.cbData);
  *(long *)cds.lpData= 0;
  *((long *)cds.lpData+1)= 0;
  strcpy((char *)((long *)cds.lpData+2), url);
  res= SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&cds);
  free(cds.lpData);
  return (res)? 0: -1;
}

#if DEBUG
static struct {
  int id;
  const char *name;
} map[]= {
  {DISPID_BEFORENAVIGATE, "DISPID_BEFORENAVIGATE"},
  {DISPID_BEFORENAVIGATE2, "DISPID_BEFORENAVIGATE2"},
  {DISPID_NAVIGATEERROR, "DISPID_NAVIGATEERROR"},
  {DISPID_FILEDOWNLOAD, "DISPID_FILEDOWNLOAD"},
  {DISPID_DOWNLOADBEGIN, "DISPID_DOWNLOADBEGIN"},
  {DISPID_DOWNLOADCOMPLETE, "DISPID_DOWNLOADCOMPLETE"},
  {DISPID_PROPERTYCHANGE, "DISPID_PROPERTYCHANGE"},
};

static const char *map_id_to_name(int id)
{
  int i;

  for(i=0; i<sizeof(map)/sizeof(map[0]); i++) {
    if(id==map[i].id) {
      return map[i].name;
    }
  }
  return NULL;
}
#endif

static char *bstr_to_str(BSTR bstr)
{
  char *str;
  int sz;

  sz= WideCharToMultiByte(CP_ACP, 0, bstr, -1, NULL, 0, NULL, NULL);
  str= malloc(sz);
  WideCharToMultiByte(CP_ACP, 0, bstr, -1, str, sz, NULL, NULL);
  return str;
}

static HRESULT STDMETHODCALLTYPE disp_Invoke(void *_this, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                                             DISPPARAMS FAR *pDispParams, VARIANT FAR *pVarResult, EXCEPINFO FAR *pExcepInfo,
                                             unsigned int FAR *puArgErr)
{
  wgbho_t *this= (void *)((void **)_this-1);

  if(dispIdMember==DISPID_QUIT ||
     dispIdMember==DISPID_ONQUIT) {
    this->iows.lpVtbl->SetSite(&this->iows, NULL);  /* Unadvises and cleans up */
    return S_OK;
  }

  if(!get_prefs_int("IE", 0)) {  /* Extra check (BHO shouldn't even be registered in this case) */
    return S_OK;
  }

#if DEBUG
  {
    const char *name;
    if((name= map_id_to_name(dispIdMember))!=NULL) {
      printf("%s\n", name);
    } else if(dispIdMember!=102 &&
              dispIdMember!=105 &&  /* Filter messages I don't care about */
              dispIdMember!=108) {
      printf("%d\n", dispIdMember);
    }
  }
#endif

  if(dispIdMember==DISPID_BEFORENAVIGATE ||
     dispIdMember==DISPID_BEFORENAVIGATE2) {
    char *nav_url;

    if(pDispParams->rgvarg[5].vt!=(VT_VARIANT|VT_BYREF) ||
       pDispParams->rgvarg[5].pvarVal->vt!=VT_BSTR) {
      MessageBox(NULL, "ERROR! NOT THE RIGHT TYPE!", "", 0);
      abort();
    }
    nav_url= bstr_to_str(pDispParams->rgvarg[5].pvarVal->bstrVal);
#if DEBUG
    printf("\t%s\n", nav_url);
#endif

    if(GetKeyState(VK_CONTROL)&0x8000 &&                                /* 1: Key-click combo */
       get_prefs_int("IE_KeyboardClick", 0)) {
      if(send_wack_url(nav_url)==0) {
        *pDispParams->rgvarg[0].pboolVal= VARIANT_TRUE;  /* If handled, tell browser to cancel */
      }
    } else if(StrNCmpI(nav_url, THE_SCHEME, sizeof(THE_SCHEME)-1)==0) { /* 2: Context menu */
      send_wack_url(nav_url+sizeof(THE_SCHEME)-1);
      *pDispParams->rgvarg[0].pboolVal= VARIANT_TRUE;  /* Always tell browser to cancel */
    }
    free(nav_url);
    return S_OK;
  }
  return S_OK;
}
