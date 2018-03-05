#include "olebox.h"
#include <sstream>
#import "flash.ocx" named_guids
#pragma warning (default : 4192)
namespace {
struct ____FlashBoxImpl____ :
    public OleBox,
    public IOleClientSite,
    public IOleInPlaceSiteWindowless,
    public IOleInPlaceFrame,
    public ShockwaveFlashObjects::_IShockwaveFlashEvents
{
public:
    long m_refCount;
    IOleObject* m_oleObject;
    LPCONNECTIONPOINT m_connectionPoint;	
    DWORD m_cookie;
    IOleInPlaceObject* m_inPlace;
    ShockwaveFlashObjects::IShockwaveFlash* m_swf;
    IOleInPlaceObjectWindowless* m_windowless;
    IViewObject* m_viewObject;
    HWND m_hWnd;
    Stub& m_lisnter;
    RECT m_dirty, m_location;
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, LPVOID* ppv)
    {
        *ppv = 0;
        if(riid == IID_IUnknown)
        {
            *ppv = (IUnknown*)(IOleClientSite*)this;
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IOleWindow)
        {
            *ppv = (IOleWindow*)(IOleInPlaceSiteWindowless*)this;
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IOleInPlaceSite)
        {
            *ppv = (IOleInPlaceSite*)this;
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IOleInPlaceSiteEx)
        {
            *ppv = (IOleInPlaceSiteEx*)this;
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IOleInPlaceSiteWindowless)
        {
            *ppv = (IOleInPlaceSiteWindowless*)this;
            AddRef();
            return S_OK;
        }
        else if (riid == IID_IOleClientSite)
        {
            *ppv = (IOleClientSite*)this;
            AddRef();
            return S_OK;
        }
        else if (riid == __uuidof(ShockwaveFlashObjects::_IShockwaveFlashEvents))
        {
            *ppv = (ShockwaveFlashObjects::_IShockwaveFlashEvents*) this;
            AddRef();
            return S_OK;
        }
        else if (m_hWnd && riid == __uuidof(IOleInPlaceFrame))
        {
            *ppv = (IOleInPlaceFrame*) this;
            AddRef();
            return S_OK;
        }
        else 
            return E_NOTIMPL;
    }

    ULONG STDMETHODCALLTYPE AddRef() { return ::InterlockedIncrement( &m_refCount ); }
    ULONG STDMETHODCALLTYPE Release()
    {  
        ULONG ret = InterlockedDecrement(&m_refCount);

        if(ret == 0)
            delete this;

        return ret;
    }
    HRESULT STDMETHODCALLTYPE GetWindow(HWND __RPC_FAR* theWnndow)
    {
        if (!m_hWnd) return E_FAIL;
        *theWnndow = m_hWnd;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetWindowContext(IOleInPlaceFrame __RPC_FAR *__RPC_FAR *ppFrame,
        IOleInPlaceUIWindow __RPC_FAR *__RPC_FAR *ppDoc, 
        LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
    {
        *ppFrame = 0;
        QueryInterface(__uuidof(IOleInPlaceFrame), (void**)ppFrame);		

        *lprcPosRect = m_location;
        *lprcClipRect = m_location;
        *ppDoc = 0;

        lpFrameInfo->hwndFrame = m_hWnd ? ::GetParent(m_hWnd) : 0;
        lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
        lpFrameInfo->fMDIApp = FALSE;
        lpFrameInfo->haccel = 0;
        lpFrameInfo->cAccelEntries = 0;

        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetBorder(LPRECT l)
    {
        *l = m_location;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE InvalidateRect(LPCRECT pRect, BOOL fErase)
    {
        if (m_hWnd)
            ::InvalidateRect(m_hWnd, pRect, fErase);
        else if (m_location.right - m_location.left > 0
            && m_location.bottom - m_location.top > 0){
            m_lisnter.Invalidate(0, pRect, fErase != 0);
            if (pRect)
                ::UnionRect(&m_dirty, &m_dirty, pRect);
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE InvalidateRgn(HRGN hRGN, BOOL fErase)
    {
        if (m_hWnd)
            ::InvalidateRgn(m_hWnd, hRGN, fErase);
        else if (m_location.right - m_location.left > 0
            && m_location.bottom - m_location.top > 0)
            m_lisnter.Invalidate(hRGN, 0, fErase != 0);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE SaveObject(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE Scroll(SIZE scrollExtant) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnUIDeactivate(BOOL fUndoable) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnInPlaceDeactivate(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE DiscardUndoState(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE DeactivateAndUndo(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnPosRectChange(LPCRECT lprcPosRect) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnInPlaceActivateEx(BOOL __RPC_FAR *pfNoRedraw, DWORD dwFlags) {return S_OK; }
    HRESULT STDMETHODCALLTYPE OnInPlaceDeactivateEx(BOOL fNoRedraw) { return S_OK; }
    HRESULT STDMETHODCALLTYPE RequestUIActivate(void) { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE CanWindowlessActivate(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetCapture(void) { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE SetCapture(BOOL fCapture) { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE GetFocus(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetFocus(BOOL fFocus) { return S_OK; }
    HRESULT STDMETHODCALLTYPE GetDC(LPCRECT pRect, DWORD grfFlags, HDC __RPC_FAR *phDC) { return E_INVALIDARG; }
    HRESULT STDMETHODCALLTYPE ReleaseDC(HDC hDC) { return E_INVALIDARG; }
    HRESULT STDMETHODCALLTYPE ScrollRect(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip) { return S_OK; }
    HRESULT STDMETHODCALLTYPE AdjustRect(LPRECT prc) { if(!prc) return E_INVALIDARG; return S_OK; }
    HRESULT STDMETHODCALLTYPE OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT __RPC_FAR *plResult) { return S_FALSE; }
    HRESULT STDMETHODCALLTYPE GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk) { *ppmk = 0; return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetContainer(IOleContainer ** theContainerP) { return E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE ShowObject(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE OnShowWindow(BOOL) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE RequestNewObjectLayout(void) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode) { return S_OK; }
    HRESULT STDMETHODCALLTYPE CanInPlaceActivate(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnInPlaceActivate(void) { return S_OK; }
    HRESULT STDMETHODCALLTYPE OnUIActivate(void) {return S_OK; }
    HRESULT STDMETHODCALLTYPE RequestBorderSpace(LPCBORDERWIDTHS b) { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetBorderSpace(LPCBORDERWIDTHS b) { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetActiveObject(IOleInPlaceActiveObject*pV,LPCOLESTR s) { return S_OK; }
    HRESULT STDMETHODCALLTYPE SetStatusText(LPCOLESTR t) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE EnableModeless(BOOL f) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG,WORD) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU h) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE InsertMenus(HMENU h,LPOLEMENUGROUPWIDTHS x) { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetMenu(HMENU h,HOLEMENU hO,HWND hw) { return E_NOTIMPL; }
public:

    virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT* pctinfo) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid,DISPID* rgDispId) { return E_NOTIMPL; }

    virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, ::DISPPARAMS __RPC_FAR *pDispParams, VARIANT __RPC_FAR *pVarResult,
        ::EXCEPINFO __RPC_FAR *pExcepInfo, UINT __RPC_FAR *puArgErr)
    {
        switch(dispIdMember)
        {
        case 0x7a6://OnProgress
            break;
        case 0x96://FSCommand
            if((pDispParams->cArgs == 2) && (pDispParams->rgvarg[0].vt == VT_BSTR) && (pDispParams->rgvarg[1].vt == VT_BSTR))
                FSCommand(pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].bstrVal);
            break;
        case 0xC5://FlashCall
            if((pDispParams->cArgs == 1) && (pDispParams->rgvarg[0].vt == VT_BSTR))
                FlashCall(pDispParams->rgvarg[0].bstrVal);
            break;
        case DISPID_READYSTATECHANGE:					
            break;
        default:			
            return DISP_E_MEMBERNOTFOUND;
        } 

        return NOERROR;
    }
    HRESULT OnReadyStateChange (long newState) {	return S_OK; }
    HRESULT OnProgress(long percentDone ) { return S_OK;}
    HRESULT FSCommand (_bstr_t command, _bstr_t args){ //TODO
        return S_OK;
    }	
    HRESULT FlashCall (_bstr_t request) {//TODO
        return S_OK;
    }
public:
    std::wstring m_cwd;
    ____FlashBoxImpl____(HRESULT& hr, IOleObject* object, Stub& lisnter, HWND hWnd, const wchar_t* cwd):
        m_refCount(0),m_oleObject(object),m_swf(0),m_hWnd(hWnd),m_lisnter(lisnter),m_cwd(cwd)
    {
        hr = NOERROR;
        memset(&m_location, 0, sizeof(m_location));
        hr = m_oleObject->SetClientSite(this);
        if SUCCEEDED(hr)
            hr = m_oleObject->QueryInterface(__uuidof(ShockwaveFlashObjects::IShockwaveFlash), (LPVOID*)&m_swf);
        if (!m_hWnd && m_swf) m_swf->PutWMode("transparent");
        if SUCCEEDED(hr)
            hr = m_oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, 0, this, 0, m_hWnd, m_hWnd ? &m_location : 0);
        if SUCCEEDED(hr)
            hr = m_oleObject->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (LPVOID*)&m_windowless);
        if SUCCEEDED(hr)
            hr = m_oleObject->QueryInterface(__uuidof(IOleInPlaceObject), (LPVOID*)&m_inPlace);	
        if (m_swf && SUCCEEDED(hr)) {

            LPCONNECTIONPOINTCONTAINER cPointContainer = 0;
            if ((m_swf->QueryInterface(IID_IConnectionPointContainer, (void**)&cPointContainer) == S_OK) &&
                (cPointContainer->FindConnectionPoint(__uuidof(ShockwaveFlashObjects::_IShockwaveFlashEvents), &m_connectionPoint) == S_OK))			
            {
                IDispatch* dispatch = this;
                hr = m_connectionPoint->Advise((LPUNKNOWN)dispatch, &m_cookie);
            }
            if(cPointContainer)
                cPointContainer->Release();

            hr = m_swf->QueryInterface(IID_IViewObject, (void**)&m_viewObject); 
        }
    }
    void Destroy()
    {
        HRESULT result;
        if(m_connectionPoint)
        {
            if(m_cookie)
            {
                result = m_connectionPoint->Unadvise(m_cookie);
                m_cookie = 0;
            }

            m_connectionPoint->Release();
            m_connectionPoint = 0;
        }
        m_oleObject->Close(OLECLOSE_NOSAVE);
        m_windowless->Release();
        m_inPlace->Release();
        m_viewObject->Release();
        m_swf->Release();
        m_oleObject->Release();
    }
    virtual void Resize(const RECT& rect)
    {
        m_location = m_dirty = rect;
        m_inPlace->SetObjectRects(&rect, &rect);
    }
    virtual bool Draw(HDC hdc, bool total)
    {
        RECT rt = total ? m_location : m_dirty;
        if (rt.right > rt.left && rt.bottom > rt.top){
            ::IntersectClipRect(hdc, rt.left, rt.top, rt.right, rt.bottom);
            ::OleDraw(m_viewObject, DVASPECT_TRANSPARENT, hdc, &m_location);
            memset(&m_dirty, 0, sizeof(m_dirty));
            return true;
        }
        return false;
    }
    virtual bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result)
    {
        return SUCCEEDED(m_windowless->OnWindowMessage(msg, wParam, lParam, result));
    }
    virtual bool Invoke(const wchar_t func[], const VARIANT* argv,  VARIANT& ret)
    {
        _variant_t retval(ret, false);
        const std::wstring cmd(func);
        if (cmd == L"play")
            retval = SUCCEEDED(m_swf->Play());
        else if (cmd == L"stop")
            retval = SUCCEEDED(m_swf->Stop());
        else if (cmd == L"forward")
            retval = SUCCEEDED(m_swf->Forward());
        else if (cmd == L"back")
            retval = SUCCEEDED(m_swf->Back());
        else if (cmd == L"align") {
            if (argv) {
                if (argv->vt == VT_INT)
                    m_swf->AlignMode = argv->iVal;
                else if (argv->vt == VT_BSTR)
                    m_swf->SAlign = argv->bstrVal;
                else
                    return false;
            }
            retval = (wchar_t*)m_swf->SAlign;
        }
        else if (cmd == L"loop") {
            if (argv) {
                if (argv->vt != VT_BOOL) return false;
                m_swf->Loop = argv->boolVal;
            }
            retval = m_swf->Loop != 0;
        }
        else if (cmd == L"menu") {
            if (argv) {
                if (argv->vt != VT_BOOL) return false;
                m_swf->Menu = argv->boolVal;
            }
            retval = m_swf->Menu != 0;
        }
        else if (cmd == L"movie") {
            if (argv) {
                if (argv->vt != VT_BSTR) return false;
                std::wstring file(argv->bstrVal);
                if (file.find(L"://") == file.npos) file = m_cwd + file;
                m_swf->Movie = file.c_str();
            }
            retval = (wchar_t*)m_swf->Movie;
        }
        else if (cmd == L"quality") {
            if (argv) {
                if (argv->vt == VT_INT)
                    m_swf->Quality = argv->iVal;
                else if (argv->vt == VT_BSTR)
                    m_swf->Quality2 = argv->bstrVal;
                else
                    return false;
            }
            retval = (wchar_t*)m_swf->Quality2;
        }
        else if (cmd == L"scale") {
            if (argv) {
                if (argv->vt == VT_INT)
                    m_swf->ScaleMode = argv->iVal;
                else if (argv->vt == VT_BSTR)
                    m_swf->Scale = argv->bstrVal;
                else
                    return false;
            }
            retval = (wchar_t*)m_swf->Scale;
        }
        else {
            std::wostringstream oss; {
                oss<<L"<invoke name=\""<<func<<L"\" returntype=\"xml\"><arguments>";
                if (argv) {
                    switch(argv->vt) {
                    case VT_BOOL:
                        oss<<(argv->boolVal ? L"<true/>":L"<false/>"); break;
                    case VT_R4:
                        oss<<L"<number>"<<argv->fltVal<<L"</number>"; break;
                    case VT_BSTR:
                        oss<<L"<string>"<<argv->bstrVal<<L"</string>";
                    }
                }
                oss<<"</arguments></invoke>";
            }
            BSTR result = 0;
            HRESULT hr = m_swf->raw_CallFunction(_bstr_t(oss.str().c_str()), &result);
            if (FAILED(hr)) {
                SysFreeString(result);
                return false;
            }
            const wchar_t* str = result;
            if (wcscmp(str, L"<true/>") == 0)
                retval = true;
            else if (wcscmp(str, L"<false/>") == 0)
                retval = false;
            else if (wcsncmp(str, L"<string>", 8) == 0){
                const wchar_t* end = wcsstr(str, L"</string>");
                retval = _bstr_t(std::wstring(str+8, end - str - 8).c_str());
            }
            else if (wcsncmp(str, L"<number>", 8) == 0){
                const wchar_t* end = wcsstr(str, L"</number>");
                retval = _wtof(str+8);
            }
            SysFreeString(result);
        }
        return true;
    }
};
}

OleBox* OleBox_ShockwaveFlash(OleBox::Stub& lisnter, const wchar_t* url, HWND hWnd){
    HRESULT hr;
    static IClassFactory *factory = 0;
    if (!factory) {
        hr = CoGetClassObject(ShockwaveFlashObjects::CLSID_ShockwaveFlash,
            CLSCTX_INPROC_SERVER, 0, IID_IClassFactory, (LPVOID *)&factory);
        if (FAILED(hr))
            return 0;
    }
    IOleObject* oleObject=0;
    hr = factory->CreateInstance(0, IID_IOleObject, (void**)&oleObject);
    if (FAILED(hr)) return 0;

    ____FlashBoxImpl____* flash = new ____FlashBoxImpl____(hr, oleObject, lisnter, hWnd, url);
    if (SUCCEEDED(hr)) return flash;

    flash->Destroy();
    return 0;   
}
