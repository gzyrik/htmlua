struct LuaBehavior : public htmlayout::event_handler
{
    LuaBehavior():event_handler(0){}
    virtual event_handler* attach (HELEMENT /*he*/ ) { return this; } 

    static void HandleDetached(const LuaCallGruad& ctx, HELEMENT he);
    virtual void detached  (HELEMENT he) { if (he) HandleDetached(LuaCallGruad(), he);}

    static void HandleAttached(const LuaCallGruad& ctx, HELEMENT he);
    virtual void attached  (HELEMENT he) { if (he) HandleAttached(LuaCallGruad(), he);}
        
    static BOOL HandleMouse (const LuaCallGruad& ctx, HELEMENT he, MOUSE_PARAMS& params);
    virtual BOOL handle_mouse  (HELEMENT he, MOUSE_PARAMS& params) { return HandleMouse(LuaCallGruad(), he, params); }

    static BOOL HandleKey (const LuaCallGruad& ctx, HELEMENT he, KEY_PARAMS& params);
    virtual BOOL handle_key (HELEMENT he, KEY_PARAMS& params) { return HandleKey(LuaCallGruad(), he, params); }

    static BOOL HandleFocus  (const LuaCallGruad& ctx, HELEMENT he, FOCUS_PARAMS& params);
    virtual BOOL handle_focus  (HELEMENT he, FOCUS_PARAMS& params) { return HandleFocus(LuaCallGruad(), he, params); }

    BOOL HandleTimer  (const LuaCallGruad& ctx, HELEMENT he,TIMER_PARAMS& params); 
    virtual BOOL handle_timer  (HELEMENT he,TIMER_PARAMS& params) { return HandleTimer(LuaCallGruad(), he, params); }

    static void HandleSize (const LuaCallGruad& ctx, HELEMENT he);
    virtual void handle_size (HELEMENT he) { HandleSize(LuaCallGruad(), he); }

    static BOOL HandleScroll (const LuaCallGruad& ctx, HELEMENT he, SCROLL_PARAMS& params);
    virtual BOOL handle_scroll  (HELEMENT he, SCROLL_PARAMS& params ) { return HandleScroll(LuaCallGruad(), he, params); }

    static BOOL HandleDraw (const LuaCallGruad& ctx, HELEMENT he, DRAW_PARAMS& params);
    virtual BOOL handle_draw (HELEMENT he, DRAW_PARAMS& params) { return HandleDraw(LuaCallGruad(), he, params); }

    // notification event: data requested by HTMLayoutRequestData just delivered
    static BOOL HandleDataArrived (const LuaCallGruad& ctx, HELEMENT he, DATA_ARRIVED_PARAMS& params);
    virtual BOOL handle_data_arrived (HELEMENT he, DATA_ARRIVED_PARAMS& params) { return HandleDataArrived(LuaCallGruad(), he, params); }

    // gesture events
    virtual BOOL handle_gesture (HELEMENT he, GESTURE_PARAMS& params) 
    {
        return FALSE;
    }
    // system D&D events
    static BOOL HandleExchange (const LuaCallGruad& ctx, HELEMENT he, EXCHANGE_PARAMS& params);
    virtual BOOL handle_exchange (HELEMENT he, EXCHANGE_PARAMS& params) { return HandleExchange(LuaCallGruad(), he, params); }

    static BOOL HandleMethodCall (const LuaCallGruad& ctx, HELEMENT he, METHOD_PARAMS& params);
    virtual BOOL handle_method_call (HELEMENT he, METHOD_PARAMS& params) { return HandleMethodCall(LuaCallGruad(), he, params);}

    static BOOL HandleScriptCall (const LuaCallGruad& ctx, HELEMENT he, XCALL_PARAMS& params );
    virtual BOOL handle_script_call (HELEMENT he, XCALL_PARAMS& params) { return HandleScriptCall(LuaCallGruad(), he, params); }

    static BOOL HandleBehaviorEvent (const LuaCallGruad& ctx, HELEMENT he, BEHAVIOR_EVENT_PARAMS& params);
    // notification events from builtin behaviors - synthesized events: BUTTON_CLICK, VALUE_CHANGED
    // see enum BEHAVIOR_EVENTS
    virtual BOOL handle_event (HELEMENT he, BEHAVIOR_EVENT_PARAMS& params) { return HandleBehaviorEvent(LuaCallGruad(), he, params); }
};
