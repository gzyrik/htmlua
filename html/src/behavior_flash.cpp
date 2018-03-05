#include "htmlua.h"
#include "behaviors/behavior_aux.h"
namespace 
{
using namespace htmlayout;
/*

BEHAVIOR: flash
  
  ole flash player

TYPICAL USE CASE:
   < div style="behavior:flash;" movie="url.swf" />
*/
struct flash :
    public event_handler,
    public OleBox::Stub
{
    virtual void Invalidate(HRGN hRGN, LPCRECT pRect, bool fErase){ 
        if (hRGN)
            ::InvalidateRgn(hwnd, hRGN, fErase);
        else
            ::InvalidateRect(hwnd, pRect, fErase);
    }
    virtual bool Callback(const wchar_t func[], const _variant_t* argv, _variant_t& retval){ return false; }
    OleBox* swf;
    HWND hwnd;
    bool focus_got;
    flash(UINT subscribed): event_handler(subscribed), swf(0), hwnd(0),focus_got(false) {}

    virtual void attached (HELEMENT he )
    {
        dom::element el(he);
        hwnd = el.get_element_hwnd(false);
        std::wstring movie = el.attribute("-movie", L"");
        if (movie.empty()) movie = el.get_value().get(L"");
        swf = OleBox_ShockwaveFlash(*this, el.url(L""), subscribed_to & HANDLE_DRAW ? 0 : hwnd);
        if (!movie.empty()) swf->Invoke(L"movie", movie.c_str());
    }
    virtual void detached(HELEMENT he)
    {
        if (swf) swf->Destroy();
        delete this;
    }
    virtual void handle_size(HELEMENT he)
    {
        if (swf){
            dom::element el(he);
            swf->Resize(el.get_location());
        }
    }
    virtual BOOL handle_draw   (HELEMENT he, DRAW_PARAMS& params ) 
    {
        if (!swf || params.cmd != DRAW_CONTENT) return FALSE;
        return swf->Draw(params.hdc, true);
    }
    virtual BOOL handle_focus  (HELEMENT he, FOCUS_PARAMS& params )
    {
        if (!swf || !(params.cmd&SINKING)) return FALSE;
        focus_got = (params.cmd & FOCUS_GOT) != 0;
        HWND hwnd = 0;
        if (params.target){
            dom::element el(params.target);
            hwnd = el.get_element_hwnd(false);
        }
        LRESULT result;
        return swf->OnMessage(focus_got ? WM_SETFOCUS : WM_KILLFOCUS, (WPARAM)hwnd, 0, &result);
    }
    virtual BOOL handle_key    (HELEMENT he, KEY_PARAMS& params )
    {
        if (!swf || !focus_got || !(params.cmd&SINKING) || params.target != he) return FALSE;
        UINT msg = 0;
        LPARAM lParam = MAKELPARAM(1,::MapVirtualKey(params.key_code,MAPVK_VK_TO_VSC));
        if (params.cmd == (KEY_DOWN | SINKING))
            msg = WM_KEYDOWN;
        else if (params.cmd == (KEY_UP | SINKING)){
            lParam |= 3<<30;
            msg = WM_KEYUP;
        }
        else if (params.cmd == (KEY_CHAR | SINKING))
            msg = WM_CHAR;
        else
            return FALSE;
        LRESULT result;
        return swf->OnMessage(msg, params.key_code, lParam, &result);
/*
UINT      alt_state;    // KEYBOARD_STATES   
CONTROL_KEY_PRESSED = 0x1,
SHIFT_KEY_PRESSED = 0x2,
ALT_KEY_PRESSED = 0x4
*/
    }
    virtual BOOL handle_mouse  (HELEMENT he, MOUSE_PARAMS& params )
    {
        if (!swf || !(params.cmd&SINKING) || params.target != he) return FALSE;
        UINT msg = 0;
        if (params.cmd == (MOUSE_DOWN | SINKING))
            msg = params.button_state == MAIN_MOUSE_BUTTON ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        else if (params.cmd == (MOUSE_UP | SINKING))
            msg = params.button_state == MAIN_MOUSE_BUTTON ? WM_LBUTTONUP : WM_RBUTTONUP;
        else if (params.cmd == (MOUSE_MOVE | SINKING))
            msg = WM_MOUSEMOVE;
        else
            return FALSE;
        WPARAM wParam = 0;
        if (params.button_state & MAIN_MOUSE_BUTTON)
            wParam |= MK_LBUTTON;
        if (params.button_state & PROP_MOUSE_BUTTON)
            wParam |= MK_RBUTTON;
        if (params.button_state & MIDDLE_MOUSE_BUTTON)
            wParam |= MK_MBUTTON;
        LRESULT result;
        return swf->OnMessage(msg, wParam, MAKELPARAM(params.pos_document.x, params.pos_document.y), &result);
    }
    virtual BOOL handle_method_call (HELEMENT he, METHOD_PARAMS& params )
    {
        return FALSE;
    }
    virtual BOOL handle_script_call (HELEMENT he, XCALL_PARAMS& params) 
    {
        return script_call_olebox(swf, params);
    }
};

struct flash_factory: public behavior
{
    flash_factory(): behavior(0, "flash") {}
    // this behavior has unique instance for each element it is attached to
    virtual event_handler* attach (HELEMENT he ) 
    {
        UINT subsriptions = HANDLE_SIZE|HANDLE_METHOD_CALL;
        dom::element el(he);
        const wchar_t* WMode = el.attribute("-WMode", (const wchar_t*)0);
        if(WMode && wcscmp(WMode, L"transparent") == 0)
            subsriptions |= HANDLE_MOUSE|HANDLE_KEY|HANDLE_DRAW|HANDLE_FOCUS;
        return new flash(subsriptions);
    }
};

// instantiating and attaching it to the global list
flash_factory flash_factory_instance;

}
