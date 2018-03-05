#pragma once
#include <Ole2.h>
struct OleBox {
    virtual ~OleBox(){}
    struct Stub {
        virtual ~Stub(){}

        ///`手工渲染', 反馈脏区域
        virtual void Invalidate(HRGN hRGN, const RECT* pRect, bool fErase) = 0;

        //调用外部函数
        virtual bool Callback(const wchar_t func[], const VARIANT* argv,  VARIANT& retva)=0;
    };

    ///销毁函数
    virtual void Destroy() = 0;

    ///`手工渲染', forceTotal强制全部重绘
    virtual bool Draw(HDC hdc, bool forceTotal=false)=0;

    ///`手工渲染', 通知相应窗口事件.
    virtual bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result) = 0;

    ///重新设置渲染区域
    virtual void Resize(const RECT& rc) = 0;

    ///调用内部函数
    virtual bool Invoke(const wchar_t func[], const VARIANT* argv,  VARIANT& retval) = 0;
   
    bool Invoke(const wchar_t func[], bool* retval = 0) {
        VARIANT ret={0};
        if (!Invoke(func, 0, ret)) return false;
        bool r = (ret.vt == VT_BOOL ? ret.boolVal != 0 : false);
        if (!retval) return r;
        *retval = r;
        return true;
    }
    bool Invoke(const wchar_t func[], int argval, bool* retval = 0) {
        VARIANT ret={0};
        VARIANT arg;
        arg.vt = VT_INT;
        arg.iVal = argval;
        if (!Invoke(func, &arg, ret)) return false;
        bool r = (ret.vt == VT_BOOL ? ret.boolVal != 0 : false);
        if (!retval) return r;
        *retval = r;
        return true;
    }
    bool Invoke(const wchar_t func[], bool argval, bool* retval = 0) {
        VARIANT ret={0};
        VARIANT arg;
        arg.vt = VT_BOOL;
        arg.boolVal = argval;
        if (!Invoke(func, &arg, ret)) return false;
        bool r = (ret.vt == VT_BOOL ? ret.boolVal != 0 : false);
        if (!retval) return r;
        *retval = r;
        return true;
    }
    bool Invoke(const wchar_t func[], const wchar_t* argval, bool* retval = 0) {
        VARIANT ret={0};
        VARIANT arg;
        arg.vt = VT_BSTR;
        arg.bstrVal = (wchar_t*)argval;
        if (!Invoke(func, &arg, ret)) return false;
        bool r = (ret.vt == VT_BOOL ? ret.boolVal != 0 : false);
        if (!retval) return r;
        *retval = r;
        return true;
    }
    bool Invoke(const wchar_t func[], const wchar_t* argval, BSTR& retval) {
        VARIANT ret={0};
        VARIANT arg;
        arg.vt = VT_BSTR;
        arg.bstrVal = (wchar_t*)argval;
        if (!Invoke(func, &arg, ret)) return false;
        retval = (ret.vt == VT_BSTR ? ret.bstrVal : 0);
        return true;
    }
};

/** 创建Flash实例
 * @param[in] stub 相应事件回调处理器
 * @param[in] url  影片的默认地址前缀,类似L"file://dir/"
 * @param[in] hWnd 渲染窗口,可选值
 *
 * @note
 *  若hWnd 有效,将自动渲染到该窗口上.
 *  反之,则进行`手工渲染’,调用
 *  - OleBox::Draw(...) 进行按需渲染
 *  - OleBox::OnMessage(...) 响应窗口事件
 *
 * @remarks
 *  OleBox::Invoke()支持如下函数:
 *  - 控制函数:
 *      bool play(),bool stop(), bool forward(), bool back()
 *      bool loop(bool), bool menu(bool), 
 *
 *  -设置或返回对齐模式
 *      0 空 当前位置,1 L 当前位置靠左,2 R 当前位置靠右, 3 LR 当前位置居中,
 *      4 T 当前位置靠上,5 LT 左上,6 TR 右上,7 LTR 上方居中,8 B 当前位置靠下
 *      9 LB 左下, 10 RB 右下, 11 LRB 下方居中, 12 TB 当前位置垂直居中
 *      13 LTB 靠左垂直居中, 14 TRB 靠右垂直居中,15 LTRB 中央位置内置属性设置并返回函数
 *      string align(string or int)
 *      
 *  -设置或返回影片名
 *      string movie(string)
 *      
 *  -设置或返回显示质量: 0 Low, 1 High, 2 AutoLow, 3 AutoHigh 
 *      string quality(string or int)
 *      
 *  -设置或返回伸缩模式: 0 ShowAll 显示全部, 1 NoBorder 无边框模式, 2 ExactFit 拉伸到整个画面, 3 NoScale 原始大小
 *      string scale(string or int)
 *      
 *  - 其他为flash内脚本函数
 */
OleBox* OleBox_ShockwaveFlash(OleBox::Stub& stub, const wchar_t* url, HWND hWnd);

/** 创建IE WebBrowser 实例
 * @param[in] stub 相应事件回调处理器
 * @param[in] url  影片的默认地址前缀,类似L"file://dir/"
 * @param[in] hWnd 渲染窗口,可选值
 *
 * @note
 *  若hWnd 有效,将自动渲染到该窗口上.
 *  反之,则进行`手工渲染’,调用
 *  - OleBox::Draw(...) 进行按需渲染
 *  - OleBox::OnMessage(...) 响应窗口事件
 *
 * @remarks
 *  OleBox::Invoke() 支持如下函数:
 *  - 控制函数:
 *      bool navigate(string url), bool back(),bool forward(), bool refresh(int level)
 *  OleBox::Stub::Callback() 反馈如下函数:
 *      BeforeNavigate2(url), NewWindow3(url,base),
 *      DocumentComplete(string url), TitleChange(string)
 *
 */
OleBox* OleBox_WebBrowser(OleBox::Stub& stub, const wchar_t* url, HWND hWnd);
