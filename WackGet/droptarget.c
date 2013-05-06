
#include "pch.h"
#include "main.h"

static HRESULT STDMETHODCALLTYPE dt_QueryInterface(void *_this, REFIID riid, void **ppvObject);
static ULONG STDMETHODCALLTYPE dt_AddRef(void *_this);
static ULONG STDMETHODCALLTYPE dt_Release(void *_this);
static HRESULT STDMETHODCALLTYPE dt_DragEnter(void *_this, IDataObject *pDataObject, DWORD grfKeyState,
                                              POINTL pt, DWORD *pdwEffect);
static HRESULT STDMETHODCALLTYPE dt_DragOver(void *_this, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
static HRESULT STDMETHODCALLTYPE dt_DragLeave(void *_this);
static HRESULT STDMETHODCALLTYPE dt_Drop(void *_this, IDataObject *pDataObject, DWORD grfKeyState, POINTL pt,
                                         DWORD *pdwEffect);

typedef struct {
  IDropTarget idt;

  int ref_cnt;
  HWND list_wnd;
  int can_drop;
} dt_t;

static IDropTargetVtbl dt_methods= {
  dt_QueryInterface,
  dt_AddRef,
  dt_Release,
  dt_DragEnter,
  dt_DragOver,
  dt_DragLeave,
  dt_Drop,
};

static CLIPFORMAT cf_html= 0;

int create_droptarget_instance(void **out_ifc, HWND list_wnd)
{
  dt_t *dt;

  if(!cf_html) {
    cf_html= RegisterClipboardFormat("HTML Format");
  }

  dt= MALLOC(sizeof(dt_t));
  memset(dt, 0, sizeof(*dt));
  dt->idt.lpVtbl= &dt_methods;
  dt->ref_cnt= 1;
  dt->list_wnd= list_wnd;
  *out_ifc= dt;
  return 0;
}

static HRESULT STDMETHODCALLTYPE dt_QueryInterface(void *_this, REFIID riid, void **ppvObject)
{
  dt_t *this= _this;

  if(IsEqualIID(riid, &IID_IUnknown)) {
    *ppvObject= &this->idt;
    this->idt.lpVtbl->AddRef(&this->idt);
    return S_OK;
  }
  if(IsEqualIID(riid, &IID_IDropTarget)) {
    *ppvObject= &this->idt;
    this->idt.lpVtbl->AddRef(&this->idt);
    return S_OK;
  }
  return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dt_AddRef(void *_this)
{
  dt_t *this= _this;

  return ++this->ref_cnt;
}

static ULONG STDMETHODCALLTYPE dt_Release(void *_this)
{
  dt_t *this= _this;

  if(--this->ref_cnt>0) {
    return this->ref_cnt;
  }
  FREE(this);
  return 0;
}

#define ALLOWED_DROPEFFECTS (DROPEFFECT_COPY|DROPEFFECT_LINK)

/* TODO: Add any additional desired dropeffects */
static void choose_dropeffect(DWORD *pdwEffect)
{
  if(*pdwEffect&DROPEFFECT_LINK) {
    *pdwEffect= DROPEFFECT_LINK; /* Prefer linking to copying */
  } else {
    *pdwEffect= DROPEFFECT_COPY;
  }
}

static int can_get_format(IDataObject *dob, CLIPFORMAT cf, FORMATETC *fmt)
{
  fmt->cfFormat= cf;
  return dob->lpVtbl->QueryGetData(dob, fmt)==S_OK;
}

static int do_html_drop(HWND list_wnd, char *html, int just_query)
{
  if(just_query) {
    if(can_extract_urls_from_html(html)) {
      return 0;
    }
  } else {
    extract_and_add_urls_from_html(list_wnd, html);
    return 0;
  }
  return -1;
}

static int do_text_drop(HWND list_wnd, char *text, int just_query)
{
  if(just_query) {
    if(extract_next_supported_url(text, NULL)) {
      return 0;
    }
  } else {
    extract_and_add_urls(list_wnd, text);
    return 0;
  }
  return -1;
}

static int do_hdrop_drop(HWND list_wnd, HDROP drop, int just_query)
{
  if(just_query) {
    if(can_extract_queues_from_hdrop(drop)) {
      return 0;
    }
  } else {
    extract_and_add_queues_from_hdrop(GetParent(list_wnd), drop);
    return 0;
  }
  return -1;
}

static HRESULT do_drop(dt_t *this, IDataObject *pDataObject, DWORD *pdwEffect, int just_query)
{
  this->can_drop= 0;
  if(*pdwEffect&ALLOWED_DROPEFFECTS) {
    FORMATETC fmt;

    memset(&fmt, 0, sizeof(fmt));
    fmt.dwAspect= DVASPECT_CONTENT;
    fmt.lindex= -1;
    fmt.tymed= TYMED_HGLOBAL;
    if(can_get_format(pDataObject, CF_HDROP, &fmt) || /* Try HDROP (files) first */
       can_get_format(pDataObject, cf_html, &fmt) || /* Then try HTML */
       can_get_format(pDataObject, CF_TEXT, &fmt)) { /* Finally, try text */
      STGMEDIUM stg;

      if(pDataObject->lpVtbl->GetData(pDataObject, &fmt, &stg)==S_OK) {
        void *data;

        data= GlobalLock(stg.hGlobal);
        switch(fmt.cfFormat) {
        case CF_HDROP:
          if(do_hdrop_drop(this->list_wnd, data, just_query)>=0) {
            this->can_drop= 1;
          }
          break;
        case CF_TEXT:
          if(do_text_drop(this->list_wnd, data, just_query)>=0) {
            this->can_drop= 1;
          }
          break;
        default:
          if(fmt.cfFormat==cf_html) {
            if(do_html_drop(this->list_wnd, data, just_query)>=0) {
              this->can_drop= 1;
            }
          }
        }
        GlobalUnlock(stg.hGlobal);
      }
      ReleaseStgMedium(&stg);
    }
  }
  if(this->can_drop) {
    choose_dropeffect(pdwEffect);
  } else {
    *pdwEffect= DROPEFFECT_NONE;
  }
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE dt_DragEnter(void *_this, IDataObject *pDataObject, DWORD grfKeyState,
                                              POINTL pt, DWORD *pdwEffect)
{
  dt_t *this= _this;
  HRESULT ret;

  ret= do_drop(this, pDataObject, pdwEffect, 1);
  return ret;
}

static HRESULT STDMETHODCALLTYPE dt_DragOver(void *_this, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
  dt_t *this= _this;

  if(this->can_drop) {
    choose_dropeffect(pdwEffect);
  } else {
    *pdwEffect= DROPEFFECT_NONE;
  }
  return S_OK;
}

static HRESULT STDMETHODCALLTYPE dt_DragLeave(void *_this)
{
  dt_t *this= _this;

  return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dt_Drop(void *_this, IDataObject *pDataObject, DWORD grfKeyState, POINTL pt,
                                         DWORD *pdwEffect)
{
  dt_t *this= _this;

  return do_drop(this, pDataObject, pdwEffect, 0);
}

int paste(HWND list_wnd, int just_query)
{
  int ret;

  ret= 0;
  if(OpenClipboard(NULL)) {
    HANDLE hdl;
    void *data;

    if(hdl= GetClipboardData(cf_html)) {
      data= GlobalLock(hdl);
      if(do_html_drop(list_wnd, data, just_query)>=0) {
        ret= 1;
      }
    } else if(hdl= GetClipboardData(CF_TEXT)) {
      data= GlobalLock(hdl);
      if(do_text_drop(list_wnd, data, just_query)>=0) {
        ret= 1;
      }
    }
    if(hdl) {
      GlobalUnlock(hdl);
    }
    CloseClipboard();
  }
  return ret;
}
