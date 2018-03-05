#pragma once
#include <comutil.h>
struct OleBox {
    virtual ~OleBox(){}
    struct Stub {
        virtual ~Stub(){}

        ///`�ֹ���Ⱦ', ����������
        virtual void Invalidate(HRGN hRGN, const RECT* pRect, bool fErase) = 0;

        //�����ⲿ����
        virtual bool Callback(const wchar_t func[], const _variant_t* argv,  _variant_t& retva)=0;
    };

    ///���ٺ���
    virtual void Destroy() = 0;

    ///`�ֹ���Ⱦ', forceTotalǿ��ȫ���ػ�
    virtual bool Draw(HDC hdc, bool forceTotal=false)=0;

    ///`�ֹ���Ⱦ', ֪ͨ��Ӧ�����¼�.
    virtual bool OnMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* result) = 0;

    ///����������Ⱦ����
    virtual void Resize(const RECT& rc) = 0;

    ///�����ڲ�����
    virtual bool Invoke(const wchar_t func[], const _variant_t* argv,  _variant_t& retval) = 0;

    template <typename T>
    bool Invoke(const wchar_t func[], const T& argval) {
        _variant_t ret;
        _variant_t arg(argval);
        if (!Invoke(func, &arg, ret)) return false;
        return ret;
    }
    template <typename T>
    bool Invoke(const wchar_t func[], const T& argval, _bstr_t& retval) {
        _variant_t ret;
        _variant_t arg(argval);
        if (!Invoke(func, &arg, ret)) return false;
        retval = ret;
        return true;
    }
};

/** ����Flashʵ��
 * @param[in] stub ��Ӧ�¼��ص�������
 * @param[in] url  ӰƬ��Ĭ�ϵ�ַǰ׺,����L"file://dir/"
 * @param[in] hWnd ��Ⱦ����,��ѡֵ
 *
 * @note
 *  ��hWnd ��Ч,���Զ���Ⱦ���ô�����.
 *  ��֮,�����`�ֹ���Ⱦ��,����
 *  - OleBox::Draw(...) ���а�����Ⱦ
 *  - OleBox::OnMessage(...) ��Ӧ�����¼�
 *
 * @remarks
 *  OleBox::Invoke()֧�����º���:
 *  - ���ƺ���:
 *      bool play(),bool stop(), bool forward(), bool back()
 *      bool loop(bool), bool menu(bool), 
 *
 *  -���û򷵻ض���ģʽ
 *      0 �� ��ǰλ��,1 L ��ǰλ�ÿ���,2 R ��ǰλ�ÿ���, 3 LR ��ǰλ�þ���,
 *      4 T ��ǰλ�ÿ���,5 LT ����,6 TR ����,7 LTR �Ϸ�����,8 B ��ǰλ�ÿ���
 *      9 LB ����, 10 RB ����, 11 LRB �·�����, 12 TB ��ǰλ�ô�ֱ����
 *      13 LTB ����ֱ����, 14 TRB ���Ҵ�ֱ����,15 LTRB ����λ�������������ò����غ���
 *      string align(string or int)
 *      
 *  -���û򷵻�ӰƬ��
 *      string movie(string)
 *      
 *  -���û򷵻���ʾ����: 0 Low, 1 High, 2 AutoLow, 3 AutoHigh 
 *      string quality(string or int)
 *      
 *  -���û򷵻�����ģʽ: 0 ShowAll ��ʾȫ��, 1 NoBorder �ޱ߿�ģʽ, 2 ExactFit ���쵽��������, 3 NoScale ԭʼ��С
 *      string scale(string or int)
 *      
 *  - ����Ϊflash�ڽű�����
 */
OleBox* OleBox_ShockwaveFlash(OleBox::Stub& stub, const wchar_t* url, HWND hWnd);

/** ����IE WebBrowser ʵ��
 * @param[in] stub ��Ӧ�¼��ص�������
 * @param[in] url  ӰƬ��Ĭ�ϵ�ַǰ׺,����L"file://dir/"
 * @param[in] hWnd ��Ⱦ����,��ѡֵ
 *
 * @note
 *  ��hWnd ��Ч,���Զ���Ⱦ���ô�����.
 *  ��֮,�����`�ֹ���Ⱦ��,����
 *  - OleBox::Draw(...) ���а�����Ⱦ
 *  - OleBox::OnMessage(...) ��Ӧ�����¼�
 *
 * @remarks
 *  OleBox::Invoke() ֧�����º���:
 *  - ���ƺ���:
 *      bool navigate(string url), bool back(),bool forward(), bool refresh(int level)
 *  OleBox::Stub::Callback() �������º���:
 *      BeforeNavigate2(url), NewWindow3(url,base),
 *      DocumentComplete(string url), TitleChange(string)
 *
 */
OleBox* OleBox_WebBrowser(OleBox::Stub& stub, const wchar_t* url, HWND hWnd);
