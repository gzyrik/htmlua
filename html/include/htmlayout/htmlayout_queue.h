#ifndef __HTMLAYOUT_QUEUE_H__
#define __HTMLAYOUT_QUEUE_H__

/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * Asynchronous GUI Task Queue.
 * Use these primitives when you need to run code in GUI thread.
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * 
 * (C) 2003-2006, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/*!\file
\brief Asynchronous GUI Task Queue
*/
#include <windows.h>
#include <stdio.h>
#include <assert.h>
#include <map>
#if defined(__cplusplus) && !defined( PLAIN_API_ONLY )

namespace htmlayout 
{
  class mutex 
  { 
    CRITICAL_SECTION cs;
 public:
    void lock()     { EnterCriticalSection(&cs); } 
    void unlock()   { LeaveCriticalSection(&cs); } 
    mutex()         { InitializeCriticalSection(&cs); }   
    ~mutex()        { DeleteCriticalSection(&cs); }
  };
  
  class critical_section { 
    mutex* m;
    critical_section():m(0) {} // no such thing
  public:
    critical_section(mutex& guard) : m(&guard) { m->lock(); }
    ~critical_section() { m->unlock(); }
  };

  // derive your own tasks from this and implement your own exec()
  class gui_task 
  {
    friend class queue;
    gui_task* next;
  public:
    gui_task(): next(0) {}
    virtual ~gui_task() {}
    virtual void exec() = 0; // override it
    virtual void release() { delete this; }
  };
  
  // this one needs to be created as singleton - one instance per GUI thread(s)
  class queue
  { 
    gui_task* head;
    gui_task* tail;
    mutex     guard;
    const DWORD threadId;
    static queue* _instance(const DWORD dwThreadId, gui_task* task=0, queue* q=0)
    {
      static mutex _guard;
      static std::map<DWORD, queue*> _queues;
      critical_section cs(_guard);
      if (q){
        if (task)
          _queues[dwThreadId] = q;
        else
          _queues.erase(dwThreadId);
      }
      else {
        std::map<DWORD, queue*>::const_iterator iter = _queues.find(dwThreadId);
        if (iter != _queues.end()){
          q = iter->second;
          if (task) q->_push(task);
          return q;
        }
        else if (task)
            task->release();
      }
      return 0;
    }
    // Place this call after GetMessage()/PeekMessage() in main loop
    void _execute()
    {
      for(gui_task* next = _pop(); next; next = _pop())
      {
        next->exec(); // do it
        next->release();
      }
    }
    void _push( gui_task* new_task)
    {
      assert(new_task);
      {
        critical_section cs(guard);
        if( tail )
          tail->next = new_task;
        else
          head = new_task;
        tail = new_task;
      }
      PostThreadMessage(threadId, WM_NULL, 0,0);
    }
    bool _is_empty() const
    { 
      return head == 0; 
    }

    gui_task* _pop()
    {
      critical_section cs(guard);
      if( !head ) return 0;

      gui_task* t = head; 
      head = head->next;
      if( !head ) tail = 0;
      return t;
    }
 public:
    queue():head(0),tail(0),threadId(::GetCurrentThreadId())
    { _instance(threadId, (gui_task*)1, this);}

    ~queue()
    {
      _instance(threadId,0,this);
      gui_task* next;
      while(next = _pop())
          next->release();
    }

    static void execute(){queue* q=_instance(::GetCurrentThreadId()); if (q) q->_execute(); }
    static void push( gui_task* new_task, const DWORD dwThreadId ) { _instance(dwThreadId,new_task);}
    static void push( gui_task* new_task, HWND hwnd ) { push(new_task, ::GetWindowThreadProcessId(hwnd, NULL));}
    static void push( gui_task* new_task, HELEMENT he ) { 
      HWND hwnd = NULL;
      HTMLayoutGetElementHwnd(he, &hwnd, true);
      push(new_task, hwnd);
    }
  };
 
}

// for one GUI thread per application cases:


/* 
   queue::execute() call shall be made in message pump loop as:
   
// Main message loop:
  while (GetMessageW(&msg, NULL, 0, 0)) 
  {
    // execute asynchronous tasks in GUI thread.
    queue::execute(); // <-- here

    if (!TranslateAcceleratorW(msg.hwnd, hAccelTable, &msg)) 
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }   

 */

/* Example of asynchronous operation wrapper:

     struct append_and_update: public gui_task
     {
       dom::element   what;
       dom::element   where;
              
       append_and_update( const dom::element& to, const dom::element& el ): where(to), what(el) {}
       
       // will be executed in GUI thread
       virtual void exec() { where.append(what); where.update(true); }
     };

     when you need this oparation simply do: 
     
     void SomeThreadProc()
     {
       ...
       queue::push( new append_and_update( parent, child ) );
     }

     so next queue::execute() invocation will execute that append_and_update::exec().

 */



#endif // __cplusplus

#endif // __HTMLAYOUT_QUEUE_H__
