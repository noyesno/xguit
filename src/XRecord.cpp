
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <X11/keysym.h>
#include <X11/StringDefs.h>
#include <X11/extensions/record.h>     // -lXtst

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>       // usleep()
#include <sys/time.h>
#include <string>
#include <map>
#include <deque>
#include <vector>
#include <algorithm>
#include <iostream>

#include "debug.hpp"
#include "XRRHelper.hpp"



static int (*OldErrorHandler)(Display *, XErrorEvent *) = NULL;
/* Function: IgnoreBadWindow
 * Description: User defined error handler callback for X event errors.
 */
static int IgnoreBadWindow(Display *display, XErrorEvent *error) {
        /* Ignore bad window errors here, handle elsewhere */
        if (error->error_code != BadWindow) {
                //assert(NULL != OldErrorHandler);
		if (NULL == OldErrorHandler)
		    return(1);

                return (*OldErrorHandler)(display, error);
        }
        /* Note: Return is ignored */
        printf("Error: BadWindow\n");
        return(0);
}



void EventCallback(XPointer p, XRecordInterceptData *idata);

/* KeyButMask

http://h30097.www3.hp.com/docs/base_doc/DOCUMENTATION/V51B_ACRO_SUP/XWINSYS.PDF

KEYMASK {Shift, Lock, Control, Mod1, Mod2, Mod3, Mod4, Mod5}
BUTMASK {Button1, Button2, Button3, Button4, Button5}
KEYBUTMASK = KEYMASK or BUTMASK

#x0001 Shift
#x0002 Lock
#x0004 Control
#x0008 Mod1
#x0010 Mod2
#x0020 Mod3
#x0040 Mod4
#x0080 Mod5
#x0100 Button1
#x0200 Button2
#x0400 Button3
#x0800 Button4
#x1000 Button5
#xe000 unused but must be zero


Keyboard Macros:
IsModifierKey(ks);
*/
struct UIEvent {
  BYTE type;
  BYTE detail;
  CARD16 serial B16;
  Time time B32;
  INT32 mask B32; // event mask

  union {
    struct {
      BYTE       button;
      KeyButMask state B16;
      //INT16 x B16, y B16;
      union {
        struct {
          short x, y;
        };
        XPoint loc;
      };
      INT16  n; // number of click or motion
      Time delay B32;
    } mouse;

    struct {
      KeyCode    key;
      KeyCode    kc;
      KeyButMask state B16;
      KeySym     ks B32;
    } key;

    struct {
      Window      window;
    } focus;
  };
};

struct WindowInfo {
    Window            window;
    XRectangle        rect;
    std::string       type;
    std::string       name;
    XWindowAttributes attrs;
};

std::string EventName[] = {
"--","--",
"KeyPress",
"KeyRelease",
"ButtonPress",
"ButtonRelease",
"MotionNotify",
"EnterNotify",
"LeaveNotify",
"FocusIn",
"FocusOut",
"KeymapNotify",
"Expose",
"GraphicsExpose",
"NoExpose",
"VisibilityNotify",
"CreateNotify",
"DestroyNotify",
"UnmapNotify",
"MapNotify",
"MapRequest",
"ReparentNotify",
"ConfigureNotify",
"ConfigureRequest",
"GravityNotify",
"ResizeRequest",
"CirculateNotify",
"CirculateRequest",
"PropertyNotify",
"SelectionClear",
"SelectionRequest",
"SelectionNotify",
"ColormapNotify",
"ClientMessage",
"MappingNotify",
"LASTEvent",
};







class XRecordControl {

protected:
  Display        *cdisp;    // Control Connection
  Display        *ddisp;    // User display to record, Data Connection
  Display        *display;  // Used for query

  bool _stop;

private:
  XRecordContext     xrctx;    // XRecordContext
  XRecordRange      *xrr;      // XRecordRange
  XRecordClientSpec  xrclient;

public:

  XRecordControl():cdisp(NULL),ddisp(NULL),xrr(NULL){
    connect();
  }

  Display* getDisplay(){ return cdisp; }

  void connect(){
    cdisp = XOpenDisplay(NULL);

    if (cdisp == NULL) {
      fprintf(stderr, "Unable to open display connection.\n");
      exit(1);
    }

    ddisp = XOpenDisplay(NULL);
    if (ddisp == NULL) {
      fprintf(stderr, "Unable to open other display connection.\n");
      exit(1);
    }

    //display = cdisp;

    XSynchronize(cdisp, True); // XSynchronize(ddisp, True);

    // Ensure extension available 
    int major = 0, minor = 0;
    if(XRecordQueryVersion(cdisp, &major, &minor)) {
      printf("# XRecord version %d.%d\n", major, minor);
    }else{
      fprintf(stderr, "The record extension is unavailable.\n");
      exit(1);
    }

    xrr = XRecordAllocRange();
    if (xrr == NULL) {
      fprintf(stderr, "Range allocation failed.\n");
      exit(1);
    }

    //xrr->device_events.first = KeyPress;
    //xrr->device_events.last  = MotionNotify;

    xrr->delivered_events.first = KeyPress;
    xrr->delivered_events.last  = MotionNotify;
    //xrr->delivered_events.last  = ConfigureNotify;

    //-- xrr->delivered_events.first = FocusIn;   // Cause Fatal for ICC
    //-- xrr->delivered_events.last  = FocusOut;

    //xrr->delivered_events.last  = ConfigureNotify;
    // xrr->delivered_events.last  = PropertyNotify;
    // xrr->delivered_events.last  = SelectionNotify;
    // INFO: use PropertyNotify when Copy
    // INFO: use SelectionRequest when Paste


    //-- xrr->core_requests.first = CreateNotify;
    //-- xrr->core_requests.last  = ConfigureNotify;

    //-- xrr->core_replies.first = CreateNotify;
    //-- xrr->core_replies.last  = ConfigureNotify;



//-- request-range            0-0
//-- reply-range                   0-0
//-- extension-request-major-range  0-0
//-- extension-request-minor-range  0-0
//-- extension-reply-major-range   0-0
//-- extension-reply-minor-range   0-0
//-- delivered-event-range         21-21
//-- device-event-range            4-6
//-- error-range                   6-6

    xrr->device_events.first = KeyPress;
    xrr->device_events.last  = KeyRelease;

    xrr->delivered_events.first = 21;
    xrr->delivered_events.last  = 21;
    xrr->delivered_events.first = ButtonPress;
    xrr->delivered_events.last  = ConfigureNotify;
    xrr->errors.first = 6;
    xrr->errors.last  = 6;


    int datum_flags = XRecordFromServerTime | XRecordFromClientTime  | XRecordFromClientSequence;
    //XRecordClientSpec xrclient = XRecordAllClients;
    xrclient = XRecordCurrentClients;
    xrclient = XRecordAllClients;

    printf("# device_events    range : %2d - %2d\n",  xrr->device_events.first, xrr->device_events.last);
    printf("# delivered_events range : %2d - %2d\n",  xrr->delivered_events.first, xrr->delivered_events.last);
    printf("# core_requests    range : %2d - %2d\n",  xrr->core_requests.first, xrr->core_requests.last);
    printf("# core_replies     range : %2d - %2d\n",  xrr->core_replies.first, xrr->core_replies.last);
    printf("# erros            range : %2d - %2d\n",  xrr->errors.first, xrr->errors.last);
    printf("# datum_flags = %d\n",datum_flags);

    //xrctx = XRecordCreateContext(cdisp, 0, &xrclient, 1, &xrr, 1);
    xrctx = XRecordCreateContext(cdisp, datum_flags, &xrclient, 1, &xrr, 1);
    if (!xrctx) {
      fprintf(stderr, "Unable to create context.\n");
      exit(1);
    }


    display = cdisp;
  }



  void start(XRecordInterceptProc callback, bool async=false){
    // Record...
    XFlush(cdisp); XFlush(ddisp);
    XSync(cdisp, True); XSync(ddisp, True);


    this->_stop = false;
    /* Use Data Connection */
    Status ret = async?XRecordEnableContextAsync(ddisp, xrctx, callback, (XPointer)this)
                      :XRecordEnableContext(ddisp, xrctx, callback, (XPointer)this);

    if (!ret){
      fprintf(stderr, "XRecordEnableContext Fail! async=%d\n", async);
      exit(1);  // TODO
    }
    printf("Info: use async\n");
    if(!async) return;

    // async if arrive here
    printf("Info: before run ...\n");
    run();
  }


  void stop(bool realstop=false){
    if(realstop){
      this->_stop = true;
      return;
    }

    debug()<<"Stop Recording ..."<<std::endl;
    if(!XRecordDisableContext(cdisp, xrctx)){ // FAIL
      printf("DEBUG: Stop Recording FAIL\n");
    }
  }

  void close(){
    XRecordFreeContext(cdisp, xrctx);
    XFree(xrr);
    XCloseDisplay(cdisp);
    XCloseDisplay(ddisp);
  }


  virtual void run(){
    printf("Info: run 111 ...\n");
    while (!_stop) {
      XRecordProcessReplies(ddisp);
        usleep(10000);  // 1ms
    }
  }

  // void pause(){ }


  void onIdle(){ }
};

class GUIRecord:public XRecordControl {
public:
  Display        *display;
  XRRHelper      xhelper;

  struct {
    Window   w;
    bool     hasShift;
    bool     hasCtrl;
    bool     hasAlt;
    KeyCode  key;

    int x;
    int y;
    int b;
    int d;         // delta distance
    unsigned char  buttons;   // mouse status

    int pressx;
    int pressy;
    int pressb;
    int clickn;
    int clickb;

    long     delay;
    timeval  now;
  } evt;

  union REvent {
    int type;
  };

  struct {
    Time time_press;
    Time time_release;
    Time time_click;
  } mouse;

public:
  GUIRecord():XRecordControl(),windows(){
    display = getDisplay();

    xhelper = XRRHelper(display); // TODO
    //--TORM--: init_keysym();
  }

  KeyCode hotkey_stop;
  KeyCode hotkey_pause;

  void init(){
    evt.buttons = 0;
    evt.pressb  = 0;
    evt.clickb  = 0;
    evt.clickn  = 0;
    evt.w = 0;

    evt.hasShift = false;
    evt.hasCtrl  = false;
    evt.hasAlt   = false;
    evt.delay    = 0;

   //xreplay_config_window();


   hotkey_stop  = XKeysymToKeycode (display, XStringToKeysym ("Escape"));
   hotkey_pause = XKeysymToKeycode (display, XStringToKeysym ("Pause"));
   XGrabKey (display, hotkey_stop, 0, XDefaultRootWindow(display), True, GrabModeAsync, GrabModeAsync);
   XGrabKey (display, hotkey_pause, 0, XDefaultRootWindow(display), True, GrabModeAsync, GrabModeAsync);


  focus.window = xhelper.focus();
  focus.rect   = xhelper.getWindowRectangle(focus.window);



  std::string wtype = xhelper.getWindowType(focus.window);
  std::string wname = xhelper.getWindowName(focus.window);
  std::string wsize = xhelper.getWindowGeometry(focus.window);

  WindowInfo winfo;
  winfo.type = wtype;
  winfo.name = wname;
  winfo.rect = focus.rect;
  windows[focus.window] = winfo;

  printf("x::window wait %s -type %s -name \"%s\" -geometry %s\n", "-focus", wtype.c_str(), wname.c_str(), wsize.c_str());
 }



  int  nEvent;
  Window lastFocus;


  struct {
    Window         window;
    XRectangle     rect;
    std::string    type;
    std::string    name;
  } focus;

  void run(){
    printf("DEBUG: run 222 ...\n");
    nEvent=0;
    lastFocus  = 0;
    while (!_stop) {
      XGrabPointer(display, xhelper.root, True, 0, GrabModeSync, GrabModeSync, None, None, CurrentTime);
      XRecordProcessReplies(ddisp);
      XUngrabPointer(display, CurrentTime);
      if(nEvent>0) {
        nEvent=0; 
        usleep(10000);  // 1ms
        continue;
      }

      //--if(nEvent==0){
      //--  recordWindow();
      //--}

      if(nEvent<=0 && nEvent>=-100) nEvent--;
      if(nEvent==-100){  //  1000ms 
        onIdle();
        //nEvent=0;
      }
      usleep(10000);     // 10ms
    }
  }

  void onIdle(){
    debug()<<"onIdle() ..."<<std::endl<<std::endl;


    //xhelper.getFocusWindow();
    Window focus = xhelper.focus();
    focus = xhelper.focus();

    if(focus == lastFocus || focus==None || focus==PointerRoot) return;
    lastFocus = focus;
    debug()<<"Window d "<<focus<<std::endl;

    std::string win_name = xhelper.getWindowName(focus);
    // std::string win_type = xhelper.getWindowPropery(focus, "_NET_WM_WINDOW_TYPE");
    std::string win_type = "...";
    debug()<<"Window "<<focus<<' '<<win_name<<" | "<<win_type<<std::endl;

    // _NET_WM_WINDOW_TYPE(ATOM) = _NET_WM_WINDOW_TYPE_DIALOG, _NET_WM_WINDOW_TYPE_NORMAL

    char **argv_return;
    int argc_return;
    if(XGetCommand(display, focus, &argv_return, &argc_return)){
      for(int i=0; i<argc_return;i++){
        printf(" | %s",argv_return[i]);
      }
      printf("\n");
      XFreeStringList(argv_return);
    }
  }

  bool _is_mouse_click(int idx, int button, int x, int y){
    if(events.size()<=idx) return false;

    if(events.size()<(idx+1)){
      printf("Assert Fail: xevent queue not enough for a mouse click @ %d\n",idx);
      return false;
    }

    return (events[idx].type == ButtonPress
         && events[idx+1].type==ButtonRelease
         && events[idx].mouse.button==events[idx+1].mouse.button);
  }

  bool _is_key_stroke(int idx){
    if(events.size()<=idx) return false;

    if(events.size()<(idx+1)){
      printf("Assert Fail: xevent queue not enough for a key stroke @ %d\n",idx);
      return false;
    }

    return (events[idx].type == KeyPress
         && events[idx+1].type==KeyRelease
         && events[idx].key.kc==events[idx+1].key.kc);
  }

  //-- void xreplay_config_window(long mask){
  //--   Window focus = xhelper.focus();
  //--   if(xhelper.isRootWindow(focus)) return;

  //--   std::string wname = xhelper.getWindowName(focus);

  //--   //if(mask & (StructureNotifyMask | FocusChangeMask))
  //--   if((mask & (StructureNotifyMask|ExposureMask))  && (mask & FocusChangeMask)){
  //--       int fwx, fwy;
  //--       unsigned int fww, fwh;
  //--       xhelper.getWindowSize(focus, fwx, fwy, fww, fwh);
  //--       // TODO: add x::window wait
  //--       // use x::sync 1000  ;# XSync and wait 1000ms
  //--       if(windows_map.find(focus)!=windows_map.end()){
  //--         XWindowAttributes a = windows_map[focus];
  //--         if(a.x=fwx && a.y==fwy && a.width==fww && a.height==fwh) {
  //--           printf("#x::info# Window: %s (%d,%d) %dx%d\n", wname.c_str(), fwx, fwy, fww, fwh);
  //--           return;
  //--         }
  //--       }
  //--       XWindowAttributes a = xhelper.getWindowAttributes(focus);
  //--       windows_map[focus] = a;
  //--       printf("#x::info# Window: %s (%d,%d) %dx%d\n", wname.c_str(), fwx, fwy, fww, fwh);
  //--       // printf("x::window info\n");
  //--       // printf("x::window config %d %d %d %d\n", fwx, fwy, fww, fwh);
  //--       // TODO:  x::window wait
  //--       int nwindow = xhelper.WindowCount();
  //--       xhelper.WindowCount(focus, true);
  //--       xhelper.getWindowParents(focus);
  //--       // printf("x::window wait -count %d\n",nwindow);
  //--       printf("#// x::window info ; x::sleep 1000 ; x::window config %d %d %d %d\n", fwx-7, fwy-24, fww, fwh);
  //--   }
  //-- }

  //-- void xreplay_config_window(){
  //--   Window focus = xhelper.focus();
  //--   if(xhelper.isRootWindow(focus)) return;

  //--   int fwx, fwy;
  //--   unsigned int fww, fwh;
  //--   xhelper.getWindowSize(focus, fwx, fwy, fww, fwh);
  //--   XWindowAttributes a = xhelper.getWindowAttributes(focus);
  //--   std::string wname = xhelper.getWindowName(focus);
  //--   printf("#x::info# Window: %s (%d,%d) %dx%d\n", wname.c_str(), fwx, fwy, fww, fwh);
  //--   printf("x::window info\n");
  //--   //printf("x::window config %d %d %d %d\n", fwx, fwy, fww, fwh);
  //--   printf("x::sleep 1000 ; x::window config %d %d %d %d\n", fwx-7, fwy-24, fww, fwh);
  //-- }

  std::deque<UIEvent> events;
  //std::map<Window, XWindowAttributes> windows_map;
  std::map<Window, WindowInfo> windows;

  void processEvent_mouse_move(int pos){
      std::vector<XPoint> points;
      long mask=0;

      int i=pos, n=events.size();
      int max_i = n-1;
      bool hasPress=false, hasRelease=false, hasMotion=false;
      int button;

      if(events[i].type == ButtonPress){
        points.push_back(events[i].mouse.loc);
        button = events[i].mouse.button;
        hasPress=true;
        i++;
      }

      for( ; i<n && events[i].type==MotionNotify; i++){
        hasMotion=true;
        if(events[i].mask!=0 || i==pos || i==max_i){
          points.push_back(events[i].mouse.loc);
          mask |= events[i].mask;
        }
      }

      if(hasPress && i<n && events[i].type == ButtonRelease){
        XPoint &prev_loc = points.back();
        XPoint &curr_loc = events[i].mouse.loc;
        if(!(curr_loc.x==prev_loc.x && curr_loc.y==prev_loc.y) ) points.push_back(events[i].mouse.loc);
        hasRelease=true;
        i++;
      }

      if(hasPress && hasMotion && hasRelease){ // drag
          printf("x::mouse drag %d", button);
          for(int j=0; j<points.size(); j++) printf(" %d %d", points[j].x, points[j].y);
          printf("\n");
          events.erase(events.begin(), events.begin()+i);
          return;
      }

      if(!hasPress && hasMotion){ // move
          printf("x::mouse move");
          for(int j=0; j<points.size(); j++) printf(" %d %d", points[j].x, points[j].y);
          printf("\n");
          events.erase(events.begin(), events.begin()+i);
          return;
      }

      if(hasPress){ // press
          printf("x::mouse press %d %d %d\n", button,  events[pos].mouse.loc.x, events[pos].mouse.loc.y);
      }
      if(hasMotion && points.size()>1){ // move
          printf("x::mouse move");
          for(int j=1; j<points.size(); j++) printf(" %d %d", points[j].x, points[j].y);
          printf("\n");
      }
      if(hasRelease){ // release
          printf("x::mouse release %d %d %d\n", button,  events[pos].mouse.loc.x, events[pos].mouse.loc.y); // TODO
      }

      events.erase(events.begin(), events.begin()+i);
      return;
  }

  void processEvent_mouse_press(){ // return true to continue process
    if(events.size()<=1){
      //debug()<<"Error: Invalid event queue. First is ButtonPress but queue size=1"<<std::endl;
      return;
    }

    int button = events[0].mouse.button; //   1=left, 2=middle, 3=right, 4=up, 5=down
    int x = events[0].mouse.x, y = events[0].mouse.y;
    int n = events[0].mouse.n;

    switch(events[1].type){
      case ButtonRelease: {   // click, otherwise an error
        if(!_is_mouse_click(0, button, x, y)){
          debug()<<"Error: Invalid event queue. Invalid click: press "<<button<<" release "<<events[1].mouse.button<<std::endl;
          return;
        }

        events[1].mouse.n +=1;              // increase click number
        events[1].mask |= events[0].mask;
        events[1].mouse.delay = events[1].time - events[0].time;
        events.pop_front();
        return;
      } break;
      case MotionNotify: {
        return processEvent_mouse_move(0);
      } break;
      case LASTEvent: {
        printf("x::mouse press %d %d %d\n", button, x, y);
        events.pop_front(); // events.pop_front();
      } break;
      case ButtonPress:
      case KeyPress:
      case KeyRelease:
      default:
        debug()<<"Error: Invalid event queue. Last event after ButtonPress is "<< EventName[events[1].type] <<std::endl;
        events.pop_front();
    }
    return;
  }

  void processEvent_mouse_release(){
    if(events.size()<=1){
      debug()<<"Error: Invalid event queue. First is ButtonRelease but queue size=1"<<std::endl;
      return;
    }


    int button = events[0].mouse.button; //   1=left, 2=middle, 3=right, 4=up, 5=down
    int x = events[0].mouse.x, y = events[0].mouse.y;
    int n = events[0].mouse.n;

    if(n==0){
      printf("x::mouse release %d %d %d\n", button, x, y);
      events.pop_front();
      return;
    }

    if( _is_mouse_click(1, button, x, y) && (events[1].time-events[0].time)<500 ){
      events[2].mouse.n = n + 1; // increase click number
      events[2].mask |= events[0].mask | events[1].mask;
      events.pop_front(); events.pop_front();
      return;
    }

    // output click
    int mask = events[0].mask;

    //-- if(mask == 0){ // caused no Event Mask
    //--   events.pop_front();
    //--   return;
    //-- }


    printf("x::mouse click %d %d %d", button, x, y);
    if(events[0].mouse.delay<5) printf(" -delay %d", events[0].mouse.delay);
    if(n>1)              printf(" -repeat %d",n);
    printf("\n");

    // TODO:
    //xreplay_config_window(mask);

    // drop a no effect click
    events.pop_front();
    return;
  }


  /***************************************************************
   *
   * Main Function to process UI Event
   * XEvent is also tracked to guess the meaning of UI Event
   *
   ***************************************************************/

  void processEvent(){
    if(events.empty()) return;
    int n_events = events.size();

    //debug()<<"processEvent size="<<events.size()<<std::endl;
      UIEvent &xev = events[0];
      switch(xev.type){
        case ButtonPress: {
          flush_keys();
          //debug()<<"processEvent_mouse_press()"<<std::endl;
          processEvent_mouse_press();
        } break;
        case ButtonRelease: {
          //debug()<<"processEvent_mouse_release()"<<std::endl;
          processEvent_mouse_release();
        } break;
        case MotionNotify: {
          processEvent_mouse_move(0);
        } break;
        case KeyPress: {
  	  KeyCode kc = xev.key.kc;
          KeySym ks = XKeycodeToKeysym(display, kc, 0);
          if(ks == XK_Shift_L || ks == XK_Shift_R) { 
            evt.hasShift = true; // TODO:
            events.pop_front();
          }else if(ks == XK_Control_L || ks == XK_Control_R) {
            evt.hasCtrl = true;
            events.pop_front();
          }else if(ks == XK_Alt_L || ks == XK_Alt_R) {
            evt.hasAlt = true;
            events.pop_front();
          }else{
            char c = xhelper.KeyCode2Char(kc, evt.hasShift);
            switch(c){
              case '\n':
                //keychars += "\\n";
                flush_keys();
                printf("x::key stroke Return\n");
                break;
              case '\t':
                //keychars += "\\t";
                flush_keys();
                printf("x::key stroke Tab\n");
                break;
              case '\0': { // key not recognized, maybe backspace, delete, etc
                flush_keys();
                const char *kchar = XKeysymToString(ks);
                printf("x::key stroke %s\n",kchar);
                break;
              }
              default: {
                if(evt.hasCtrl || evt.hasAlt){
                  flush_keys();
                  printf("x::key ctrl ");
                  if(evt.hasCtrl) printf("Ctrl+");
                  if(evt.hasAlt) printf("Alt+");
                  if(evt.hasShift) printf("Shift+");
                  printf("%c\n",c);
                }else{
                  keychars += c;
                }
              }
            }

            if(_is_key_stroke(0)) { // to save a new call
              events.pop_front();
              events.pop_front();
            }else{
              events.pop_front();
            }
          }
        } break;
        case KeyRelease: {
  	  KeyCode kc = xev.key.kc;
          KeySym ks = XKeycodeToKeysym(display, kc, 0);
          char c;
          if(ks == XK_Shift_L || ks == XK_Shift_R) {
            evt.hasShift = false;
            events.pop_front();
          }else if(ks == XK_Control_L || ks == XK_Control_R) {
            evt.hasCtrl = false;
            events.pop_front();
          }else if(ks == XK_Alt_L || ks == XK_Alt_R) {
            evt.hasAlt = false;
            events.pop_front();
          }else{
            // ignore release of key, because release is a kind of reset
            // Usually, it's key press trigger the event
            events.pop_front();
          }
        } break;
        case LASTEvent: {
          if(xev.focus.window!=0){
            Window focus = xev.focus.window;
            WindowInfo winfo = windows[focus];
            std::string wtype = winfo.type;
            std::string wname = winfo.name;
            std::string wsize = xhelper.Rectangle2Geometry(winfo.rect);
            printf("x::window wait %s -type %s -name \"%s\" -geometry %s\n", (winfo.attrs.override_redirect==False?"-focus":""), wtype.c_str(), wname.c_str(), wsize.c_str());
	    if(events.size()>=2){
	      Time diff = events[1].time - events[0].time;
              if(diff>100) printf("x::sleep %d\n", diff);
	    }
            events.pop_front();
          }
        } break;
        default:
          debug()<<"pop_front event_type="<<EventName[xev.type]<<std::endl;
          events.pop_front();
      } // end switch

    if(events.size()<n_events && !events.empty()) processEvent();
  }

  // std::vector<KeyCode> keystack;
  std::string keychars;
  void flush_keys(){
    if(keychars.size()>0){
      printf("x::key type \"%s\"\n", keychars.c_str());
      keychars = "";
    }
  }



  void onMousePress(UIEvent evt){
    this->mouse.time_press = evt.time;
    this->processEvent();
  }
  void onMouseRelease(UIEvent evt){
  }
  void onMouseMotion(UIEvent evt){
  }

  void onKeyPress(UIEvent evt){
    KeyCode kc = evt.key.kc;
    if(kc == this->hotkey_stop) {
      this->stop();
    }else if(kc == this->hotkey_pause) {
      //grecord->stop();
    }
  }

  void onKeyRelease(UIEvent evt){
    this->processEvent();
  }

};




/*****************************************************************
* XRecordInterceptData.category:
*   XRecordStartOfData
*   XRecordFromServer
*   XRecordFromClient
*   XRecordClientStarted
*   XRecordClientDied
*   XRecordEndOfData
*****************************************************************/


//-- void EventCallback(XPointer p, XRecordInterceptData *idata) {
//--   GUIRecord *grecord = (GUIRecord *) p;
//--   //if(grecord->nEvent < 0) grecord->nEvent = 0;
//--   //grecord->nEvent ++;
//-- 
//--   //-- if(idata!=NULL){
//--   //--   //-- xEvent *xev = (xEvent *)idata->data;
//--   //--   //-- int type = xev->u.u.type;
//--   //--   printf("type = %d\n", 1);
//--   //--   XRecordFreeData(idata);
//--   //-- }
//--   //-- return;
//-- 
//--   //XRecordFreeData(idata); return;
//-- 
//--   if (XRecordStartOfData == idata->category){
//--     debug()<<"XRecordStartOfData"<<std::endl;
//--     grecord->init();
//--     XRecordFreeData(idata);
//--     return;
//--   }
//--   if (XRecordEndOfData == idata->category){
//--     debug()<<"XRecordEndOfData"<<std::endl;
//--     grecord->stop(true);
//--     XRecordFreeData(idata);
//--     return;
//--   }
//-- 
//--   if (XRecordFromClient == idata->category) {
//--     xEvent *xev = (xEvent *)idata->data;
//--     int type = xev->u.u.type;
//--     debug()<<"XRecordFromClient "<<type<<' '<<(type<36?EventName[type]:"...") <<std::endl;
//--     XRecordFreeData(idata);
//--     return;
//--   }
//-- 
//--   if (XRecordFromServer != idata->category && XRecordFromClient != idata->category) {
//--     debug()<<"!XRecordFromServer"<<std::endl;
//--     XRecordFreeData(idata);
//--     return;
//--   }
//-- 
//--   Time server_time = idata->server_time;
//-- 
//--     Display *display = grecord->display;
//-- 
//--     //Display *disp = (Display *)p;
//-- 
//--     xEvent *xev = (xEvent *)idata->data;
//--     int type = xev->u.u.type;
//--     int keyPress = 0;
//-- 
//-- 
//-- 
//--     timeval now;
//--     gettimeofday(&now, NULL);
//--     long secDiff  = (now.tv_sec - grecord->evt.now.tv_sec);
//--     long usecDiff = ((now.tv_usec - grecord->evt.now.tv_usec) / 1000);
//--     long delay    = ((secDiff * 1000) + usecDiff);
//-- 
//--     grecord->evt.delay = delay;
//--     grecord->evt.now   = now;
//--     //printf("DEBUG: delay = %ld\n",delay);
//-- 
//-- 
//--     if(type>MotionNotify){
//--       if(grecord->nEvent < 0) grecord->nEvent = 0;
//--       grecord->nEvent ++;
//--     }
//-- 
//--     switch (type) {
//--       case MotionNotify:
//--       case EnterNotify:
//--       case LeaveNotify:
//--       case KeymapNotify:
//--         break;
//--       case ButtonPress:
//--       case ButtonRelease:
//--       case KeyPress:
//--       case KeyRelease:
//--          break;
//--       default:
//--          //debug()<<"event: "<<type<<' '<<(type<36?EventName[type]:"...") <<std::endl;
//--          break;
//--     }
//-- 
//--     switch (type) {
//--       case ButtonPress:
//--       case MotionNotify:
//--       case ButtonRelease: {
//--         UIEvent evt;
//--         evt.mouse.type   = xev->u.u.type;
//--         evt.mouse.button = xev->u.u.detail;
//--         evt.mouse.serial = xev->u.u.sequenceNumber;
//--         evt.mouse.time   = xev->u.keyButtonPointer.time;
//--         evt.mouse.mask   = 0;
//-- 
//--         evt.mouse.x      = xev->u.keyButtonPointer.rootX;
//--         evt.mouse.y      = xev->u.keyButtonPointer.rootY;
//--         evt.mouse.state  = xev->u.keyButtonPointer.state;
//--         evt.mouse.n      = 0;
//--         grecord->events.push_back(evt);
//-- 
//--         if(0 || type!=MotionNotify){
//--           debug()<<"mouse: "<<type<<' '<< EventName[type]<<' '<< int(evt.mouse.button) <<" @ ("<<evt.mouse.x<<','<<evt.mouse.y<<") "
//--                  <<evt.mouse.state<<' '<< xev->u.keyButtonPointer.event
//--                  <<' '<<xev->u.keyButtonPointer.eventX<<','<< xev->u.keyButtonPointer.eventY
//--                  <<std::endl;
//--         }
//--         //if(type==ButtonRelease || type==ButtonPress) grecord->processEvent();
//--         if(type==ButtonPress) grecord->processEvent();
//--       } break;
//--       case KeyPress: {
//--         KeyCode kc = xev->u.u.detail;
//--         if(kc == grecord->hotkey_stop) {
//--           grecord->stop();
//--           break;
//--         }
//--         if(kc == grecord->hotkey_pause) {
//--           //grecord->stop();
//--           break;
//--         }
//--       }
//--       case KeyRelease: {
//--         UIEvent evt;
//--         evt.key.type   = xev->u.u.type;
//--         evt.key.kc     = xev->u.u.detail;
//--         evt.key.serial = xev->u.u.sequenceNumber;
//--         evt.key.time   = xev->u.keyButtonPointer.time;
//--         evt.key.mask   = 0;
//-- 
//--         evt.key.state  = xev->u.keyButtonPointer.state;
//-- 
//--         KeyCode kc = xev->u.u.detail;
//--         if(kc == grecord->hotkey_pause || kc == grecord->hotkey_stop) break;
//-- 
//--         grecord->events.push_back(evt);
//--         //debug()<<"key: "<<type<<' '<< EventName[type]<<' '<<evt.key.kc<<' '<<evt.key.state<<std::endl;
//--         if(type==KeyRelease) grecord->processEvent();
//--       } break;
//--       case FocusIn: {
//--         debug()<<"mask FocusChangeMask "<<EventName[type]<<' '<<xev->u.focus.window<<std::endl;
//--         if(!grecord->events.empty()) grecord->events.back().mask |= FocusChangeMask;
//-- 
//--         Window focus = xev->u.focus.window;
//--         Window realfocus = grecord->xhelper.focus();
//--         if(focus==realfocus || grecord->windows.find(focus)!=grecord->windows.end() ){
//--           std::string wtype = grecord->xhelper.getWindowType(focus);
//--           std::string wname = grecord->xhelper.getWindowName(focus);
//--           std::string wsize = grecord->xhelper.getWindowSize(focus);
//--           debug()<<"Change Focus "<<focus<<' '<<realfocus<<' '<<wtype<<' '<<wname<<' '<<wsize<<std::endl;
//--         }
//--         //-- grecord->events.push_back(*xev);
//--         //-- grecord->processEvent();
//--       } break;
//--       case FocusOut: {
//--         debug()<<"mask FocusChangeMask "<<EventName[type]<<' '<<xev->u.focus.window<<std::endl;
//--         if(!grecord->events.empty()) grecord->events.back().mask |= FocusChangeMask;
//--       
//--       } break;
//--       case Expose: {
//--         debug()<<"mask ExposureMask "<< xev->u.expose.window <<std::endl;
//--         if(!grecord->events.empty()) grecord->events.back().mask |= ExposureMask;
//--       } break;
//--       case GraphicsExpose: {
//--         debug()<<"mask GCGraphicsExposures for GraphicsExpose "<<GCGraphicsExposures<<std::endl;
//--         if(!grecord->events.empty()) grecord->events.back().mask |= GCGraphicsExposures;
//--       } break;
//--       case NoExpose: {
//--         //-- debug()<<"mask GCGraphicsExposures for NoExpose"<<GCGraphicsExposures<<std::endl;
//--         //-- if(!grecord->events.empty()) grecord->events.back().mask |= GCGraphicsExposures;
//--       } break;
//--       case CreateNotify: {
//--         Window window = xev->u.createNotify.window;
//--         Window parent = xev->u.createNotify.parent;
//--         int    width  = xev->u.createNotify.width;
//--         int    height = xev->u.createNotify.height;
//--         int    x      = xev->u.createNotify.x;
//--         int    y      = xev->u.createNotify.y;
//--         debug()<<"mask GCGraphicsExposures for "<<EventName[type]<<' '<<window<<'<'<<parent<<' '<<width<<'x'<<height<<'+'<<x<<'+'<<y<<std::endl;
//--       } break;
//--       case DestroyNotify: {
//--         Window window = xev->u.destroyNotify.window;
//--         Window event  = xev->u.destroyNotify.event;
//--         debug()<<"mask GCGraphicsExposures for "<<EventName[type]<<' '<<window<<' '<<event<<std::endl;
//--       } break;
//-- 
//-- /* ****** Below Are Key Event For Window Management ****** */
//--       case MapNotify: {
//--         Window window = xev->u.unmapNotify.window;
//--         Window event  = xev->u.unmapNotify.event;
//--         BOOL   override = xev->u.mapNotify.override;
//--         std::string wtype = grecord->xhelper.getWindowType(window);
//--         std::string wname = grecord->xhelper.getWindowName(window);
//--         std::string wsize = grecord->xhelper.getWindowSize(window);
//-- 
//--         int wstate = -1;
//--         XWMHints *whints = XGetWMHints(display, window);
//--         if(whints != NULL){
//--           if(whints->flags & StateHint){
//--             wstate = whints->initial_state;
//--           }
//--         }
//-- 
//--         if(window==event){
//--           debug()<<"mask StructureNotifyMask for MapNotify "<<window<<' '<< (int)override <<" @"<<event<<' '<<wtype<<' '<<wname<<' '<<wsize<<' '<<wstate<<std::endl;
//--           if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
//--         }else{
//--           debug()<<"mask SubstructureNotifyMask for MapNotify "<<window<<" @"<<event<<' '<<wtype<<std::endl;
//--           if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureNotifyMask;
//--         }
//--         grecord->windows[window] = 1;
//--       }   break;
//--       case ReparentNotify: {
//--         Window window = xev->u.reparent.window;
//--         Window parent = xev->u.reparent.parent;
//--         Window event  = xev->u.reparent.event;
//--         if(window==event){
//--           debug()<<"mask StructureNotifyMask for ReparentNotify "<< window <<'<'<<parent<<" @"<<event<<std::endl;
//--           if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
//--         }else{
//--           debug()<<"mask SubstructureNotifyMask for ReparentNotify "<< window <<'<'<<parent<<" @"<<event<<std::endl;
//--           if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureNotifyMask;
//--         }
//--       } break;
//--       case ConfigureNotify: {
//--         Window window = xev->u.configureNotify.window;
//--         Window event  = xev->u.configureNotify.event;
//--         int     width  = xev->u.configureNotify.width;
//--         int     height = xev->u.configureNotify.height;
//--         int     x      = xev->u.configureNotify.x;
//--         int     y      = xev->u.configureNotify.y;
//--         if(window==event){
//--           debug()<<"mask StructureNotifyMask for ConfigureNotify "<< window <<' '<<width<<'x'<<height<<'+'<<x<<'+'<<y<<std::endl;
//--           if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
//--         }else{
//--           debug()<<"mask SubstructureNotifyMask for ConfigureNotify "<< window <<' '<<width<<'x'<<height<<'+'<<x<<'+'<<y<<std::endl;
//--           if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureNotifyMask;
//--         }
//--       } break;
//--       case UnmapNotify: {
//--         Window window = xev->u.unmapNotify.window;
//--         Window event  = xev->u.unmapNotify.event;
//--         grecord->windows_map.erase(window);
//--         grecord->windows.erase(window);
//--         debug()<<"mask StructureNotifyMask for UnmapNotify "<< window <<' '<<event<<std::endl;
//--         if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
//--       }  break;
//--       case MapRequest: {
//--         Window window = xev->u.mapRequest.window;
//--         Window parent = xev->u.mapRequest.parent;
//--         //debug()<<"mask SubstructureRedirectMask for MapRequest "<<window<<' '<<parent<<std::endl;
//--         if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureRedirectMask;
//--       } break;
//--       case VisibilityNotify: {
//--         Window window = xev->u.visibility.window;
//--         CARD8  state  = xev->u.visibility.state;
//--         debug()<<"mask VisibilityChangeMask for VisibilityNotify "<<window<<' '
//--           << (state==VisibilityUnobscured?"VisibilityUnobscured":(state==VisibilityFullyObscured?"VisibilityFullyObscured":"VisibilityPartiallyObscured"))
//--           <<std::endl;
//--       } break;
//--       case EnterNotify:
//--       case LeaveNotify:
//--       case KeymapNotify:
//--         break;
//--       case PropertyNotify:{
//--         // it's possible Copy action
//--         //-- Window window = xev->u.property.window;
//--         //-- Atom atom     = xev->u.property.atom;
//--         //-- char *atom_name = XGetAtomName(display,atom);
//--         //-- debug()<<"mask PropertyChangeMask "<<window<<' '<<atom_name<<std::endl;
//--         //-- // _NET_USER_TIME
//--         //-- if(!grecord->events.empty()) grecord->events.back().mask |= PropertyChangeMask;
//--         //-- XFree(atom_name);
//--       } break;
//--       case SelectionRequest: {
//--           // INFO: use SelectionRequest when Paste
//--       } break;
//--       default:
//--          // VisibilityChangeMask
//--          //std::cout<<"event type: "<<type<<std::endl;
//--          debug()<<"event: "<<type<<' '<<(type<36?EventName[type]:"...") <<std::endl;
//--          break;
//--     }
//--     ////
//--     //HandleEvent(re);
//--     ////
//--   //grecord->nextEvent();
//--   if (idata != NULL)  XRecordFreeData(idata);
//-- }

void xrecord_event_callback_relative(XPointer p, XRecordInterceptData *idata) {
  GUIRecord *grecord = (GUIRecord *) p;

  if (XRecordStartOfData == idata->category){
    debug()<<"XRecordStartOfData"<<std::endl;
    grecord->init();

    if (idata != NULL)  XRecordFreeData(idata);
    return;
  }

  if (XRecordEndOfData == idata->category){
    debug()<<"XRecordEndOfData"<<std::endl;

    UIEvent evt;
    evt.type   = LASTEvent;
    grecord->events.push_back(evt);

    grecord->processEvent();
    grecord->stop(true);
    XRecordFreeData(idata);
    return;
  }

  if (XRecordFromClient == idata->category) {
    xEvent *xev = (xEvent *)idata->data;
    int type = xev->u.u.type;
    debug()<<"XRecordFromClient "<<type<<' '<<(type<36?EventName[type]:"...") <<std::endl;
    XRecordFreeData(idata);
    return;
  }

  if (XRecordFromServer != idata->category && XRecordFromClient != idata->category) {
    debug()<<"!XRecordFromServer"<<std::endl;
    XRecordFreeData(idata);
    return;
  }

  Time server_time = idata->server_time;

  Display *display = grecord->display;


  xEvent *xev = (xEvent *)idata->data;
  int type = xev->u.u.type;
  int keyPress = 0;



  timeval now;
  gettimeofday(&now, NULL);
  long secDiff  = (now.tv_sec - grecord->evt.now.tv_sec);
  long usecDiff = ((now.tv_usec - grecord->evt.now.tv_usec) / 1000);
  long delay    = ((secDiff * 1000) + usecDiff);

  grecord->evt.delay = delay;
  grecord->evt.now   = now;
  //printf("DEBUG: delay = %ld\n",delay);


  if(type>MotionNotify){
    if(grecord->nEvent < 0) grecord->nEvent = 0;
    grecord->nEvent ++;
  }

    switch (type) {
      case ButtonPress:
      case MotionNotify:
      case ButtonRelease: {
        //if(type==ButtonRelease || type==ButtonPress) grecord->processEvent();

        UIEvent evt;
        evt.type   = xev->u.u.type;
        evt.serial = xev->u.u.sequenceNumber;
        evt.time   = xev->u.keyButtonPointer.time;
        evt.mask   = 0;

        evt.mouse.button = xev->u.u.detail;
        //evt.mouse.x      = xev->u.keyButtonPointer.rootX;
        //evt.mouse.y      = xev->u.keyButtonPointer.rootY;
        evt.mouse.x      = xev->u.keyButtonPointer.rootX - grecord->focus.rect.x;
        evt.mouse.y      = xev->u.keyButtonPointer.rootY - grecord->focus.rect.y;
        evt.mouse.state  = xev->u.keyButtonPointer.state;
        evt.mouse.n      = 0;
        evt.mouse.delay  = 0;

        grecord->events.push_back(evt);

        if(0 || type!=MotionNotify){
          debug()<<"mouse: "<<type<<' '<< EventName[type]<<' '<< int(evt.mouse.button) <<" @ ("<<evt.mouse.x<<','<<evt.mouse.y<<") "
                 <<evt.mouse.state<<' '<< xev->u.keyButtonPointer.event
                 <<' '<<xev->u.keyButtonPointer.eventX<<','<< xev->u.keyButtonPointer.eventY
                 <<' '<< evt.time <<' ' << (evt.time -  grecord->mouse.time_press)<<' '<<server_time
                 <<std::endl;
        }

        if(type==ButtonPress) {
	  grecord->onMousePress(evt);
	}else if(type==ButtonRelease) {
	  grecord->onMouseRelease(evt);
	}else{
	  grecord->onMouseMotion(evt);
	}
      } break;
      case KeyPress:
      case KeyRelease: {
        UIEvent evt;
        evt.type   = xev->u.u.type;
        evt.serial = xev->u.u.sequenceNumber;
        evt.time   = xev->u.keyButtonPointer.time;
        evt.mask   = 0;

        evt.key.key    = xev->u.u.detail;
        evt.key.kc     = xev->u.u.detail;
        evt.key.state  = xev->u.keyButtonPointer.state;

        KeyCode kc = xev->u.u.detail;
        // if(kc == grecord->hotkey_pause || kc == grecord->hotkey_stop) break;

        grecord->events.push_back(evt);
        //debug()<<"key: "<<type<<' '<< EventName[type]<<' '<<evt.key.kc<<' '<<evt.key.state<<std::endl;

        if(type==KeyPress){
	  grecord->onKeyPress(evt);
	}else{
	  grecord->onKeyRelease(evt);
          // if(type==KeyRelease) grecord->processEvent();
	}
      } break;
      case FocusIn: {
        debug()<<"mask FocusChangeMask "<<EventName[type]<<' '<<xev->u.focus.window<<std::endl;
        if(!grecord->events.empty()) grecord->events.back().mask |= FocusChangeMask;

        Window focus = xev->u.focus.window;
        if(grecord->windows.find(focus)!=grecord->windows.end()){

          std::string wtype = grecord->xhelper.getWindowType(focus);
          std::string wname = grecord->xhelper.getWindowName(focus);
          std::string wsize = grecord->xhelper.getWindowGeometry(focus);
          Window      wfocus =  grecord->xhelper.focus();

          XRectangle wrect = grecord->xhelper.getWindowRectangle(focus);

          debug()<<"Change Focus "<<focus<<' '<<wtype<<' '<<wname<<' '<<wsize<<std::endl;

          grecord->focus.window = focus;
          grecord->focus.rect   = wrect;

          grecord->windows[focus].type = wtype;
          grecord->windows[focus].name = wname;
          grecord->windows[focus].rect = wrect;

          //grecord->focus.style  = (focus==wfocus);

          // TODO:
          //-- UIEvent evt;
          //-- evt.type   = xev->u.u.type;
          //-- evt.serial = xev->u.u.sequenceNumber;
          //-- evt.time   = xev->u.keyButtonPointer.time;
          //-- evt.mask   = 0;

          //-- evt.focus.window = focus;
          //-- evt.focus.type   = wtype;
          //-- evt.focus.name   = wname;
          //-- evt.focus.rect   = grecord->xhelper.getWindowRectangle(focus);
          //-- grecord->events.push_back(evt);

          if(!(!grecord->events.empty() && grecord->events.back().type==LASTEvent)){
            UIEvent evt;
            evt.type   = LASTEvent;
	    evt.time   = server_time;
            evt.detail = xev->u.u.detail;
            evt.serial = xev->u.u.sequenceNumber;
            evt.mask   = 0;
            grecord->events.push_back(evt);
          }

          grecord->events.back().focus.window = focus;
          //grecord->events.back().focus.normal = (focus==wfocus);

          //-- grecord->processEvent();
          //-- printf("x::window wait %s -type %s -name \"%s\" -geometry %s\n", (focus==wfocus?"-focus":""), wtype.c_str(), wname.c_str(), wsize.c_str());

        }
        //-- grecord->processEvent();

      } break;
      case FocusOut: {
        debug()<<"mask FocusChangeMask "<<EventName[type]<<' '<<xev->u.focus.window<<std::endl;
        // if(!grecord->events.empty()) grecord->events.back().mask |= FocusChangeMask;
      } break;
      case Expose: {
        debug()<<"mask ExposureMask "<< xev->u.expose.window <<std::endl;
        if(!grecord->events.empty()) grecord->events.back().mask |= ExposureMask;
      } break;
      case GraphicsExpose: {
        debug()<<"mask GCGraphicsExposures for GraphicsExpose "<<GCGraphicsExposures<<std::endl;
        if(!grecord->events.empty()) grecord->events.back().mask |= GCGraphicsExposures;
      } break;
      case NoExpose: {
        //-- debug()<<"mask GCGraphicsExposures for NoExpose"<<GCGraphicsExposures<<std::endl;
        //-- if(!grecord->events.empty()) grecord->events.back().mask |= GCGraphicsExposures;
      } break;
      case CreateNotify: {
        Window window = xev->u.createNotify.window;
        Window parent = xev->u.createNotify.parent;
        int    width  = xev->u.createNotify.width;
        int    height = xev->u.createNotify.height;
        int    x      = xev->u.createNotify.x;
        int    y      = xev->u.createNotify.y;
        debug()<<"mask GCGraphicsExposures for "<<EventName[type]<<' '<<window<<'<'<<parent<<' '<<width<<'x'<<height<<'+'<<x<<'+'<<y<<std::endl;
      } break;
      case DestroyNotify: {
        Window window = xev->u.destroyNotify.window;
        Window event  = xev->u.destroyNotify.event;
        debug()<<"mask GCGraphicsExposures for "<<EventName[type]<<' '<<window<<' '<<event<<std::endl;
      } break;

/* ****** Below Are Key Event For Window Management ****** */
      case MapNotify: {
        Window window = xev->u.unmapNotify.window;
        Window event  = xev->u.unmapNotify.event;
        BOOL   override = xev->u.mapNotify.override;
        std::string wtype = grecord->xhelper.getWindowType(window);
        std::string wname = grecord->xhelper.getWindowName(window);
        std::string wsize = grecord->xhelper.getWindowGeometry(window);

        int wstate = -1;
        XWMHints *whints = XGetWMHints(display, window);
        if(whints != NULL){
          if(whints->flags & StateHint){
            wstate = whints->initial_state;
          }
        }


  

        if(window==event){
          debug()<<"mask StructureNotifyMask for MapNotify "<<window<<' '<< (int)override <<" @"<<event<<' '<<wtype<<' '<<wname<<' '<<wsize<<' '<<wstate<<std::endl;
          if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
        }else{
          debug()<<"mask SubstructureNotifyMask for MapNotify "<<window<<" @"<<event<<' '<<wtype<<std::endl;
          if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureNotifyMask;
        }

        XWindowAttributes wattrs;
        XGetWindowAttributes(display, window, &wattrs);
        WindowInfo winfo;
        winfo.attrs = wattrs;
        grecord->windows[window] = winfo;
      }   break;
      case ReparentNotify: {
        Window window = xev->u.reparent.window;
        Window parent = xev->u.reparent.parent;
        Window event  = xev->u.reparent.event;
        if(window==event){
          debug()<<"mask StructureNotifyMask for ReparentNotify "<< window <<'<'<<parent<<" @"<<event<<std::endl;
          if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
        }else{
          debug()<<"mask SubstructureNotifyMask for ReparentNotify "<< window <<'<'<<parent<<" @"<<event<<std::endl;
          if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureNotifyMask;
        }
      } break;
      case ConfigureNotify: {
        Window window = xev->u.configureNotify.window;
        Window event  = xev->u.configureNotify.event;
        int     width  = xev->u.configureNotify.width;
        int     height = xev->u.configureNotify.height;
        int     x      = xev->u.configureNotify.x;
        int     y      = xev->u.configureNotify.y;
        if(window==event){
          debug()<<"mask StructureNotifyMask for ConfigureNotify "<< window <<' '<<width<<'x'<<height<<'+'<<x<<'+'<<y<<std::endl;
          if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
        }else{
          debug()<<"mask SubstructureNotifyMask for ConfigureNotify "<< window <<' '<<width<<'x'<<height<<'+'<<x<<'+'<<y<<std::endl;
          if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureNotifyMask;
        }
      } break;
      case UnmapNotify: {
        Window window = xev->u.unmapNotify.window;
        Window event  = xev->u.unmapNotify.event;
        //grecord->windows_map.erase(window);
        grecord->windows.erase(window);
        debug()<<"mask StructureNotifyMask for UnmapNotify "<< window <<' '<<event<<std::endl;
        if(!grecord->events.empty()) grecord->events.back().mask |= StructureNotifyMask;
      }  break;
      case MapRequest: {
        Window window = xev->u.mapRequest.window;
        Window parent = xev->u.mapRequest.parent;
        //debug()<<"mask SubstructureRedirectMask for MapRequest "<<window<<' '<<parent<<std::endl;
        if(!grecord->events.empty()) grecord->events.back().mask |= SubstructureRedirectMask;
      } break;
      case VisibilityNotify: {
        Window window = xev->u.visibility.window;
        CARD8  state  = xev->u.visibility.state;
        debug()<<"mask VisibilityChangeMask for VisibilityNotify "<<window<<' '
          << (state==VisibilityUnobscured?"VisibilityUnobscured":(state==VisibilityFullyObscured?"VisibilityFullyObscured":"VisibilityPartiallyObscured"))
          <<std::endl;
      } break;
      case EnterNotify:
      case LeaveNotify:
      case KeymapNotify:
        break;
      case PropertyNotify:{
        // it's possible Copy action
        //-- Window window = xev->u.property.window;
        //-- Atom atom     = xev->u.property.atom;
        //-- char *atom_name = XGetAtomName(display,atom);
        //-- debug()<<"mask PropertyChangeMask "<<window<<' '<<atom_name<<std::endl;
        //-- // _NET_USER_TIME
        //-- if(!grecord->events.empty()) grecord->events.back().mask |= PropertyChangeMask;
        //-- XFree(atom_name);
      } break;
      case SelectionRequest: {
          // INFO: use SelectionRequest when Paste
      } break;
      default:
         // VisibilityChangeMask
         //std::cout<<"event type: "<<type<<std::endl;
         debug()<<"event: "<<type<<' '<<(type<36?EventName[type]:"...") <<std::endl;
         break;
    }
    ////
    //HandleEvent(re);
    ////
  //grecord->nextEvent();
  if (idata != NULL)  XRecordFreeData(idata);
}

/**********************************************************************
*
* Main Function
*    xrecord record
***********************************************************************/
int main(int argc, const char *argv[]){
  if(argc>=2 && strcmp(argv[1],"record")==0){ // record
    GUIRecord grecord;
    // printf("xdotool search -sync -name \"%s\" windowfocus\n","xterm");
    printf("load ./XReplay.so ;# x::init\n");
    printf("x::connect\n");
    OldErrorHandler = XSetErrorHandler(IgnoreBadWindow);
    if(argc>=3){
      //grecord.start(EventCallback, true);   // async
      grecord.start(xrecord_event_callback_relative, true);   // async
    }else{
      // grecord.start(EventCallback, false);
      grecord.start(xrecord_event_callback_relative, false);  // sync
    }
    //grecord.close();
  }else{                                    // record
    XRecordControl xrecord;
    //xrecord.start(EventCallback);
    xrecord.close();
  }
  return 0;
}
