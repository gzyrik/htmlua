#include "olebox.h"
#include <sstream>
#include <MsHtmHst.h>
#include <ExDisp.h>
#include <MsHTML.h>
#include <comutil.h>
#include <ExDispid.h>
#include <cstdio>

#define HRTEST_SE(fn, err) if FAILED(fn) { wprintf(err); goto RETURN; }
#define NULLTEST_SE(fn, err) if (!(fn)) {wprintf(err); goto RETURN; }
#define NULLTEST(fn) if(!(fn)) goto RETURN;
#define HRTEST_E HRTEST_SE
#define NULLTEST_E NULLTEST_SE
#define RELEASE_AND_CLEAR(p) if (p) { (p) -> Release () ; (p) = NULL ; }

namespace {
class WebBrowser:
    public IDispatch,
    public IOleClientSite,
    public IOleInPlaceSite,
    public IOleInPlaceFrame,
    public IDocHostUIHandler
{
    //继承的类应该实现这个方法,告诉WebBrowser,到底用哪一个HWND放置WebBrowser
    virtual HWND GetHWND(){return NULL;};
    // DWebBrowserEvents2, 走私货,以后再讲
    virtual void DocumentComplete( IDispatch *pDisp,const wchar_t* url) = 0;
    virtual void BeforeNavigate2( IDispatch *pDisp, const wchar_t* url,LONG Flags,const wchar_t* TargetFrameName,VARIANT *&PostData,const wchar_t* Headers,VARIANT_BOOL *&Cancel) = 0;
    virtual void NewWindow3(IDispatch **&pDisp,VARIANT_BOOL *&Cancel,LONG dwFlags,const wchar_t* bstrUrlContext, const wchar_t* bstrUrl) = 0;
    virtual void TitleChange( const wchar_t* bstrTitle) = 0;
    // 内部工具函数
private:
    inline IOleObject* _GetOleObject(){return _pOleObj;};
    inline IOleInPlaceObject* _GetInPlaceObject(){return _pInPlaceObj;};
protected:
    long   _refNum;
private:
    RECT  _rcWebWnd;
    bool  _bInPlaced;
    bool  _bExternalPlace;
    bool  _bCalledCanInPlace;
    bool  _bWebWndInited;
private:
    IOleObject*                 _pOleObj; 
    IOleInPlaceObject*          _pInPlaceObj;
    IStorage*                   _pStorage;
    IWebBrowser2*               _pWB2;
    IHTMLDocument2*             _pHtmlDoc2;
    IHTMLDocument3*             _pHtmlDoc3;
    IHTMLWindow2*               _pHtmlWnd2;
    IHTMLEventObj*              _pHtmlEvent;
	IConnectionPoint* _pConnectionPoint;
	DWORD _dwCookie;
protected:
    // 构造和析构  
    WebBrowser(void):
        _refNum(0),
        _bInPlaced(false),
        _bExternalPlace(false),
        _bCalledCanInPlace(false),
        _bWebWndInited(false),
        _pOleObj(NULL), 
        _pInPlaceObj(NULL), 
        _pStorage(NULL), 
        _pWB2(NULL), 
        _pHtmlDoc2(NULL), 
        _pHtmlDoc3(NULL), 
        _pHtmlWnd2(NULL), 
        _pHtmlEvent(NULL),
		_pConnectionPoint(NULL),
		_dwCookie(0)
    {
        ::memset( (PVOID)&_rcWebWnd,0,sizeof(_rcWebWnd));
        HRTEST_SE( StgCreateDocfile(0,STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT | STGM_CREATE,0,&_pStorage),L"StgCreateDocfile错误");
        HRTEST_SE( OleCreate(CLSID_WebBrowser,IID_IOleObject,OLERENDER_DRAW,0,this,_pStorage,(void**)&_pOleObj),L"Ole创建失败");
        HRTEST_SE( _pOleObj->QueryInterface(IID_IOleInPlaceObject,(LPVOID*)&_pInPlaceObj),L"OleInPlaceObject创建失败");
RETURN:
        return;
    }

    // IUnknown methods|
    STDMETHODIMP QueryInterface(REFIID iid,void**ppvObject)
    {
        *ppvObject = 0;
        if ( iid == IID_IOleClientSite )
            *ppvObject = (IOleClientSite*)this;
        else if ( iid == IID_IUnknown )
            *ppvObject = this;
        else if ( iid == IID_IDispatch )
            *ppvObject = (IDispatch*)this;
        else if ( _bExternalPlace == false)
        {
            if ( iid == IID_IOleInPlaceSite )
                *ppvObject = (IOleInPlaceSite*)this;
            else if ( iid == IID_IOleInPlaceFrame )
                *ppvObject = (IOleInPlaceFrame*)this;
            else if ( iid == IID_IOleInPlaceUIWindow )
                *ppvObject = (IOleInPlaceUIWindow*)this;
        }
        //这里是一点走私货, 留在以后讲,如果有机会,你可以发现,原来如此简单.
        if ( iid == DIID_DWebBrowserEvents2 )
            *ppvObject = (DWebBrowserEvents2 *)this;
        else if ( iid == IID_IDocHostUIHandler)
            *ppvObject = (IDocHostUIHandler*)this;
        if ( *ppvObject )
        {
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
	ULONG STDMETHODCALLTYPE AddRef() { return ::InterlockedIncrement( &_refNum ); }
	ULONG STDMETHODCALLTYPE Release()
	{  
		ULONG ret = InterlockedDecrement(&_refNum);

		if(ret == 0)
			delete this;

		return ret;
	}

    // IDispatch Methods
    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
        unsigned int * pctinfo) 
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetTypeInfo(
        unsigned int iTInfo,
        LCID lcid,
        ITypeInfo FAR* FAR* ppTInfo) 
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid, 
        OLECHAR FAR* FAR* rgszNames, 
        unsigned int cNames, 
        LCID lcid, 
        DISPID FAR* rgDispId )
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE Invoke(
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        unsigned int* puArgErr)
    {
        //走私货,留在以后讲,是关于DWebBrowserEvents2让人激动的实现,而且简单.
        // DWebBrowserEvents2
        switch(dispIdMember)
        {
        case DISPID_DOCUMENTCOMPLETE:
            DocumentComplete(pDispParams->rgvarg[1].pdispVal,_bstr_t(pDispParams->rgvarg[0]));
            return S_OK;
        case DISPID_BEFORENAVIGATE2:
            BeforeNavigate2( pDispParams->rgvarg[6].pdispVal,
                _bstr_t(pDispParams->rgvarg[5]),
                pDispParams->rgvarg[4].lVal,
                _bstr_t(pDispParams->rgvarg[3]),
                pDispParams->rgvarg[2].pvarVal,
                _bstr_t(pDispParams->rgvarg[1]),
                pDispParams->rgvarg[0].pboolVal);
            return S_OK;
        case DISPID_NEWWINDOW3:
            NewWindow3(pDispParams->rgvarg[4].ppdispVal,
                pDispParams->rgvarg[3].pboolVal,
                pDispParams->rgvarg[2].lVal,
                _bstr_t(pDispParams->rgvarg[1]),
                _bstr_t(pDispParams->rgvarg[0]));
            return S_OK;
        case DISPID_TITLECHANGE:
            TitleChange(_bstr_t(pDispParams->rgvarg[0]));
            return S_OK;
        }
        return E_NOTIMPL;
    }

    // IOleClientSite methods
    STDMETHODIMP SaveObject()
    {
        return S_OK;
    }
    STDMETHODIMP GetMoniker(DWORD dwA,DWORD dwW,IMoniker**pm)
    {
        *pm = 0;
        return E_NOTIMPL;
    }
    STDMETHODIMP GetContainer(IOleContainer**pc)
    {
        *pc = 0;
        return E_FAIL;
    }
    STDMETHODIMP ShowObject()
    {
        return S_OK;
    }
    STDMETHODIMP OnShowWindow(BOOL f)
    {
        return S_OK;
    }
    STDMETHODIMP RequestNewObjectLayout()
    {
        return S_OK;
    }

    // IOleInPlaceSite methods
    STDMETHODIMP GetWindow(HWND *p)
    {
        *p = GetHWND();
        return S_OK;
    }
    STDMETHODIMP ContextSensitiveHelp(BOOL)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP CanInPlaceActivate()//If this function return S_FALSE, AX cannot activate in place!
    {
        if ( _bInPlaced )//Does WebBrowser Control already in placed?
        {
            _bCalledCanInPlace = true;
            return S_OK;
        }
        return S_FALSE;
    }
    STDMETHODIMP OnInPlaceActivate()
    {
        return S_OK;
    }
    STDMETHODIMP OnUIActivate()
    {
        return S_OK;
    }
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame** ppFrame,IOleInPlaceUIWindow **ppDoc,LPRECT r1,LPRECT r2,LPOLEINPLACEFRAMEINFO o)
    {

        *ppFrame = (IOleInPlaceFrame*)this;
        AddRef();
        *ppDoc = NULL;
        *r1 = _rcWebWnd;
        *r2 = _rcWebWnd;
        o->cb = sizeof(OLEINPLACEFRAMEINFO);
        o->fMDIApp = false;
        o->hwndFrame = GetParent( GetHWND() );
        o->haccel = 0;
        o->cAccelEntries = 0;

        return S_OK;
    }
    STDMETHODIMP Scroll(SIZE s)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP OnUIDeactivate(int)
    {
        return S_OK;
    }
    STDMETHODIMP OnInPlaceDeactivate()
    {
        return S_OK;
    }
    STDMETHODIMP DiscardUndoState()
    {
        return S_OK;
    }
    STDMETHODIMP DeactivateAndUndo()
    {
        return S_OK;
    }
    STDMETHODIMP OnPosRectChange(LPCRECT)
    {
        return S_OK;
    }

    // IOleInPlaceFrame methods
    STDMETHODIMP GetBorder(LPRECT l)
    {
        *l = _rcWebWnd;
        return S_OK;
    }
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS b)
    {
        return S_OK;
    }
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS b)
    {
        return S_OK;
    }
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject*pV,LPCOLESTR s)
    {
        return S_OK;
    }
    STDMETHODIMP SetStatusText(LPCOLESTR t)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP EnableModeless(BOOL f)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP TranslateAccelerator(LPMSG,WORD)
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE RemoveMenus(HMENU h)
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE InsertMenus(HMENU h,LPOLEMENUGROUPWIDTHS x)
    {
        return E_NOTIMPL;
    }
    HRESULT STDMETHODCALLTYPE SetMenu(HMENU h,HOLEMENU hO,HWND hw)
    {
        return E_NOTIMPL;
    }
    // IDocHostUIHandler, 传说中的IDocHostUIHanler,同样留在以后讲
    HRESULT  STDMETHODCALLTYPE ShowContextMenu( 
        DWORD dwID,
        POINT *ppt,
        IUnknown *pcmdtReserved,
        IDispatch *pdispReserved){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE GetHostInfo( 
        DOCHOSTUIINFO *pInfo){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE ShowUI( 
        DWORD dwID,
        IOleInPlaceActiveObject *pActiveObject,
        IOleCommandTarget *pCommandTarget,
        IOleInPlaceFrame *pFrame,
        IOleInPlaceUIWindow *pDoc){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE HideUI( void){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE UpdateUI( void){return E_NOTIMPL;}
    //HRESULT  EnableModeless( 
    //  BOOL fEnable){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE OnDocWindowActivate( 
        BOOL fActivate){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE OnFrameWindowActivate( 
        BOOL fActivate){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE ResizeBorder( 
        LPCRECT prcBorder,
        IOleInPlaceUIWindow *pUIWindow,
        BOOL fRameWindow){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE TranslateAccelerator( LPMSG lpMsg, const GUID *pguidCmdGroup, DWORD nCmdID){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE GetOptionKeyPath( LPOLESTR *pchKey, DWORD dw){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE GetDropTarget( IDropTarget *pDropTarget, IDropTarget **ppDropTarget)
    {
        return E_NOTIMPL;//使用默认拖拽
        //return S_OK;//自定义拖拽
    }
    HRESULT  STDMETHODCALLTYPE GetExternal( IDispatch **ppDispatch)
    {
        return E_NOTIMPL;
    }
    HRESULT  STDMETHODCALLTYPE TranslateUrl( 
        DWORD dwTranslate,
        OLECHAR *pchURLIn,
        OLECHAR **ppchURLOut){return E_NOTIMPL;}
    HRESULT  STDMETHODCALLTYPE FilterDataObject( 
        IDataObject *pDO,
        IDataObject **ppDORet){return E_NOTIMPL;}

    /* Other Methods */
    IWebBrowser2* GetWebBrowser2()
    {
        if( _pWB2 != NULL )
            return _pWB2;
        NULLTEST_SE( _pOleObj,L"Ole对象为空");
        HRTEST_SE( _pOleObj->QueryInterface(IID_IWebBrowser2,(void**)&_pWB2),L"QueryInterface IID_IWebBrowser2 失败");
        return _pWB2;
RETURN:
        return NULL;
    }
    IHTMLDocument2*    GetHTMLDocument2()
    {
        if( _pHtmlDoc2 != NULL )
            return _pHtmlDoc2;
        IWebBrowser2* pWB2 = NULL;
        NULLTEST(pWB2 = GetWebBrowser2());//GetWebBrowser2已经将错误原因交给LastError.
        IDispatch* pDp =  NULL;
        HRTEST_SE(pWB2->get_Document(&pDp),L"DWebBrowser2::get_Document 错误");
        HRTEST_SE(pDp->QueryInterface(IID_IHTMLDocument2,(void**)&_pHtmlDoc2),L"QueryInterface IID_IHTMLDocument2 失败");
        return _pHtmlDoc2;
RETURN:
        return NULL;
    }
    IHTMLDocument3*    GetHTMLDocument3()
    {
        if( _pHtmlDoc3 != NULL )
            return _pHtmlDoc3;
        IWebBrowser2* pWB2 = NULL;
        NULLTEST(pWB2 = GetWebBrowser2());//GetWebBrowser2已经将错误原因交给LastError.
        IDispatch* pDp =  NULL;
        HRTEST_SE(pWB2->get_Document(&pDp),L"DWebBrowser2::get_Document 错误");
        HRTEST_SE(pDp->QueryInterface(IID_IHTMLDocument3,(void**)&_pHtmlDoc3),L"QueryInterface IID_IHTMLDocument3 失败");
        return _pHtmlDoc3;
RETURN:
        return NULL;
    }
    IHTMLWindow2* GetHTMLWindow2()
    {
        if( _pHtmlWnd2 != NULL)
            return _pHtmlWnd2;
        IHTMLDocument2*  pHD2 = GetHTMLDocument2();
        NULLTEST( pHD2 );
        HRTEST_SE( pHD2->get_parentWindow(&_pHtmlWnd2),L"IHTMLWindow2::get_parentWindow 错误" );
        return _pHtmlWnd2;
RETURN:
        return NULL;
    }
    IHTMLEventObj*   GetHTMLEventObject()
    {
        if( _pHtmlEvent != NULL )
            return _pHtmlEvent;
        IHTMLWindow2* pHW2;
        NULLTEST( pHW2 = GetHTMLWindow2() );
        HRTEST_SE( pHW2->get_event(&_pHtmlEvent),L"IHTMLWindow2::get_event 错误");
        return _pHtmlEvent;
RETURN:
        return NULL;
    }
    BOOL  SetWebRect(LPRECT lprc)
    {
        BOOL bRet = FALSE;
        _rcWebWnd = *lprc;
        if (_bInPlaced)//已经打开WebBrowser,通过 IOleInPlaceObject::SetObjectRects 调整大小
        {
            SIZEL size;
            size.cx = lprc->right - lprc->left;
            size.cy = lprc->bottom - lprc->top;
            IOleObject* pOleObj;
            NULLTEST( pOleObj= _GetOleObject());
            HRTEST_E( pOleObj->SetExtent(  1,&size ),L"SetExtent 错误");
            IOleInPlaceObject* pInPlace;
            NULLTEST( pInPlace = _GetInPlaceObject());
            HRTEST_E( pInPlace->SetObjectRects(lprc,lprc),L"SetObjectRects 错误");
        }
        bRet = TRUE;
RETURN:
        return bRet;
    }
    BOOL   OpenWebBrowser()
    {
        BOOL bRet = FALSE;
        NULLTEST_E( _GetOleObject(),L"ActiveX对象为空" );//对于本身的实现函数,其自身承担错误录入工作

        if( _bInPlaced == false )// Activate In Place
        {
            _bInPlaced = true;//_bInPlaced must be set as true, before INPLACEACTIVATE, otherwise, once DoVerb, it would return error;
            _bExternalPlace = 0;//lParam;

            HRTEST_E( _GetOleObject()->DoVerb(OLEIVERB_INPLACEACTIVATE,0,this,0, GetHWND()  ,&_rcWebWnd),L"关于INPLACE的DoVerb错误");
            _bInPlaced = true;


            //* 挂接DWebBrwoser2Event
            IConnectionPointContainer* pCPC = NULL;
            HRTEST_E( GetWebBrowser2()->QueryInterface(IID_IConnectionPointContainer,(void**)&pCPC),L"枚举IConnectionPointContainer接口失败");
            HRTEST_E( pCPC->FindConnectionPoint( DIID_DWebBrowserEvents2,&_pConnectionPoint),L"FindConnectionPoint失败");
            HRTEST_E( _pConnectionPoint->Advise( (IUnknown*)(void*)this,&_dwCookie),L"IConnectionPoint::Advise失败");
			pCPC->Release();
        }
        bRet = TRUE;
RETURN:
        return bRet;
    }

	void CloseWebBrowser()
	{
		HRESULT result;
		if(_pConnectionPoint)
		{
			if(_dwCookie)
			{
				result = _pConnectionPoint->Unadvise(_dwCookie);
				_dwCookie = 0;
			}

			_pConnectionPoint->Release();
			_pConnectionPoint = 0;
		}
		_pOleObj->Close(OLECLOSE_NOSAVE);
		RELEASE_AND_CLEAR(_pHtmlEvent);
		RELEASE_AND_CLEAR(_pHtmlWnd2);
		RELEASE_AND_CLEAR(_pHtmlDoc3);
		RELEASE_AND_CLEAR(_pHtmlDoc2);
		RELEASE_AND_CLEAR(_pWB2);
		RELEASE_AND_CLEAR(_pStorage);
		RELEASE_AND_CLEAR(_pInPlaceObj);
		RELEASE_AND_CLEAR(_pOleObj);
	}
    HRESULT Navigate(const wchar_t* url)
    {
        _variant_t var(url);
        return GetWebBrowser2()->Navigate2(&var,0,0,0,0);
    }
	void Destroy()
	{

	}
};
struct __WebBoxImpl__ : public WebBrowser, public OleBox
{
    OleBox::Stub& m_lisnter;
    HWND m_hWnd;
    std::wstring m_cwd;
    __WebBoxImpl__(HRESULT& hr, OleBox::Stub& lisnter, HWND hWnd, const wchar_t* cwd)
        : m_lisnter(lisnter), m_hWnd(hWnd), m_cwd(cwd)
    {
        hr = OpenWebBrowser() ? S_OK : E_FAIL;
    }
    virtual HWND GetHWND(){ return m_hWnd;};
    virtual void BeforeNavigate2( IDispatch *pDisp, const wchar_t* bstrUrl,LONG Flags,const wchar_t* TargetFrameName,VARIANT *&PostData,const wchar_t* Headers,VARIANT_BOOL *&Cancel)
    {
        _variant_t argval(bstrUrl), retval;
        if (m_lisnter.Callback(L"BeforeNavigate2", &argval, retval)){
            if (retval.vt == VT_BOOL) *Cancel = retval.boolVal;
        }
    }
    virtual void NewWindow3(IDispatch **&pDisp,VARIANT_BOOL *&Cancel,LONG dwFlags,const wchar_t* bstrUrlContext, const wchar_t* bstrUrl)
    {
        _variant_t argval(bstrUrl), retval;
        if (m_lisnter.Callback(L"NewWindow3", &argval, retval)){
            if (retval.vt == VT_BOOL) *Cancel = retval.boolVal;
        }
    }
    virtual void TitleChange( const wchar_t* bstrTitle)
    {
        _variant_t argval(bstrTitle), retval;
        m_lisnter.Callback(L"TitleChange", &argval, retval);
    }
    virtual void DocumentComplete( IDispatch *pDisp,const wchar_t* url)
    {
        _variant_t argval(url), retval;
        m_lisnter.Callback(L"DocumentComplete", &argval, retval);
    }
    virtual void Destroy(){ CloseWebBrowser(); }
    ///`手工渲染', forceTotal强制全部重绘
    virtual bool Draw(HDC hdc, bool forceTotal){ return true; }
    ///重新设置渲染区域
    virtual void Resize(const RECT& rc)
    {
        SetWebRect((LPRECT)&rc);
    }
    ///`手工渲染‘时,通知相应窗口事件.
    virtual bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result)
    {
        return false;
    }
    virtual bool Invoke(const wchar_t func[], const _variant_t* argv,  _variant_t& retval) 
    { 
        HRESULT hr;
        const std::wstring cmd(func);
        if (cmd == L"navigate"){
            std::wstring path;
            if (argv) {
                if (argv->vt != VT_BSTR) return false;
                path = argv->bstrVal;
                if (!path.empty()){
                    if (path.find(L"www.") == 0)
                        path = L"http://" + path;
                    else if (path.find(L"://") == path.npos)
                        path = m_cwd + path;
                }
            }
            if (!path.empty())
                hr = Navigate(path.c_str());
            else
                hr = GetWebBrowser2()->GoHome();
            retval = SUCCEEDED(hr);
            return true;
        }
        else if (cmd == L"back") {
            hr = GetWebBrowser2()->GoBack();
            retval = SUCCEEDED(hr);
            return true;
        }
        else if (cmd == L"forward") {
            hr = GetWebBrowser2()->GoForward();
            retval = SUCCEEDED(hr);
            return true;
        }
        else if (cmd == L"refresh") {
            if (argv)
                hr = GetWebBrowser2()->Refresh2((VARIANT *)argv);
            else
                hr = GetWebBrowser2()->Refresh();
            retval = SUCCEEDED(hr);
            return true;
        }
        return false; 
    }

};
}
OleBox* OleBox_WebBrowser(OleBox::Stub& stub, const wchar_t* url, HWND hWnd)
{
    HRESULT hr;
    __WebBoxImpl__* box = new __WebBoxImpl__(hr, stub, hWnd, url);
    if (SUCCEEDED(hr)) return box;

    box->Destroy();
    return 0;   
}
