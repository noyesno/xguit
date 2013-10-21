
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

#include <X11/keysym.h>
#include <X11/StringDefs.h>
#include <X11/extensions/record.h>     // -lXtst
#include <X11/extensions/XTest.h>     // -lXtst

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>       // usleep()
#include <sys/time.h>
#include <string>
#include <map>
#include <iostream>

#define debug() std::cout<<"DEBUG: "

void EventCallback(XPointer p, XRecordInterceptData *idata);





//-- class X11Helper {
//-- 
//-- 
//--   
//-- //#-- static const KeyNameSymTable kns_table[] = { /* {Name, Sym}, */
//-- //#--         {"BAC", XK_BackSpace},          {"BS", XK_BackSpace},           {"BKS", XK_BackSpace},
//-- //#--         {"BRE", XK_Break},                      {"CAN", XK_Cancel},             {"CAP", XK_Caps_Lock},
//-- //#--         {"DEL", XK_Delete},                     {"DOW", XK_Down},                       {"END", XK_End},
//-- //#--         {"ENT", XK_Return},                     {"ESC", XK_Escape},                     {"HEL", XK_Help},
//-- //#--         {"HOM", XK_Home},                       {"INS", XK_Insert},                     {"LEF", XK_Left},
//-- //#--         {"NUM", XK_Num_Lock},           {"PGD", XK_Next},                       {"PGU", XK_Prior},
//-- //#--         {"PRT", XK_Print},                      {"RIG", XK_Right},                      {"SCR", XK_Scroll_Lock},
//-- //#--         {"TAB", XK_Tab},                        {"UP", XK_Up},                          {"F1", XK_F1},
//-- //#--         {"F2", XK_F2},                          {"F3", XK_F3},                          {"F4", XK_F4},
//-- //#--         {"F5", XK_F5},                          {"F6", XK_F6},                          {"F7", XK_F7},
//-- //#--         {"F8", XK_F8},                          {"F9", XK_F9},                          {"F10", XK_F10},
//-- //#--         {"F11", XK_F11},                        {"F12", XK_F12},                        {"SPC", XK_space},
//-- //#--         {"SPA", XK_space},                      {"LSK", XK_Super_L},            {"RSK", XK_Super_R},
//-- //#--         {"MNU", XK_Menu},    
//-- //#-- 
//-- //#--         {"~", XK_asciitilde},           {"_", XK_underscore},
//-- //#--         {"[", XK_bracketleft},          {"]", XK_bracketright},         {"!", XK_exclam},
//-- //#-- 
//-- //#--         {"\"", XK_quotedbl},            {"#", XK_numbersign},           {"$", XK_dollar},
//-- //#--         {"%", XK_percent},                      {"&", XK_ampersand},            {"'", XK_quoteright},
//-- //#--         {"*", XK_asterisk},                     {"+", XK_plus},                         {",", XK_comma},
//-- //#--         {"-", XK_minus},                        {".", XK_period},                       {"?", XK_question},
//-- //#--         {"<", XK_less},                         {">", XK_greater},                      {"=", XK_equal},
//-- //#--         {"@", XK_at},                           {":", XK_colon},                        {";", XK_semicolon},
//-- //#--         {"\\", XK_backslash},           {"`", XK_grave},                        {"{", XK_braceleft},
//-- //#--         {"}", XK_braceright},           {"|", XK_bar},                          {"^", XK_asciicircum},
//-- //#--         {"(", XK_parenleft},            {")", XK_parenright},           {" ", XK_space},
//-- //#--         {"/", XK_slash},                        {"\t", XK_Tab},                         {"\n", XK_Return},
//-- //#--         {"LSH", XK_Shift_L},            {"RSH", XK_Shift_R},            {"LCT", XK_Control_L},
//-- //#--         {"RCT", XK_Control_R},          {"LAL", XK_Alt_L},                      {"RAL", XK_Alt_R},
//-- //#--     {"LMA", XK_Meta_L},                 {"RMA", XK_Meta_R},
//-- //#-- };
//-- 
//-- 
//-- public:
//-- 
//-- const char *KeysymToChar(KeySym sym) {
//--         size_t x = 0;
//-- 
//--         /* Look for KeySym in order to obtain name */
//--         for (x = 0; x < (sizeof(kns_table) / sizeof(KeyNameSymTable)); x++) {
//--                 if (sym == kns_table[x].Sym) {
//--                         /* Found It */
//--                         return kns_table[x].Name;
//--                 }
//--         }
//-- 
//--         return XKeysymToString(sym);
//-- }
//-- 
//-- };


class GUIPlay {
public:
  Display        *display;

  bool shift;   // whether SHIFT is pressed

  GUIPlay():display(NULL), shift(false){
    this->connect();
  }

  void connect() {
        int eventnum = 0, errornum = 0,
                majornum = 0, minornum = 0;

        /* Get Display Pointer */
        display = XOpenDisplay(NULL);
        if (display == NULL) {
           printf("FAIL to connect X Windows!\n");
           exit(1);
        }

        if (!XTestQueryExtension(display, &eventnum, &errornum, &majornum, &minornum)) {
           printf("XTest is not supported on server %s!\n", "null");
        }

        printf("connected %d.%d!\n",majornum,minornum);

        DefaultScreen(display);

        XSynchronize(display, True);
        XTestGrabControl(display, True);
  }


  void move(int x, int y, bool relative=false){
    // XTestFakeMotionEvent(display, -1, x, y, this->delay);
    if(relative){
      XTestFakeRelativeMotionEvent(display, x, y, CurrentTime);
    }else{
      XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
    }
    XFlush(display);
  }


  KeyCode char2KeyCode(char c, bool &need_shift){
    char buf[2]; buf[0]=c;buf[1]='\0';
    KeySym sym = XStringToKeysym(buf);

    need_shift=false;
    KeySym ksl, ksu;
    XConvertCase(sym, &ksl, &ksu);
    if(sym==ksu && ksl!=ksu) need_shift=true;

    KeyCode kc = XKeysymToKeycode(display, sym);
    return kc;
  }

  void send(char c, int delay=0){
    bool need_shift=false;
    KeyCode kc = char2KeyCode(c, need_shift);
    if(kc==0) return;
    printf("DEBUG: send kc = %d\n",kc);
    if(!shift && need_shift){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), True, CurrentTime); // is_press
    }
    XTestFakeKeyEvent(display, kc, True, 30); // is_press
    XFlush(display);
    XTestFakeKeyEvent(display, kc, False, 30); // KeyUp
    XFlush(display);
    if(!shift && need_shift){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), False, CurrentTime); // is_press
    }
    XFlush(display);
    if(delay>0) usleep(delay*1000);  // unit: ms
  }


};

class GUIRecord {
public:
  Display        *disp;
  Display        *display;
  Display        *udisp;   // User display to record
  XRecordContext  xrctx;
  XRecordRange   *xrr;

  struct {
    Window   w;
    bool     hasShift;
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


public:
  GUIRecord():disp(NULL),xrr(NULL),udisp(NULL){
    init_keysym();

    disp = XOpenDisplay(NULL);
    display = disp;

    if (disp == NULL) {
      fprintf(stderr, "Unable to open display connection.\n");
      exit(1);
    }
    XSynchronize(disp, False);

    udisp = XOpenDisplay(NULL);
    if (udisp == NULL) {
      fprintf(stderr, "Unable to open other display connection.\n");
      exit(1);
    }

    // Ensure extension available 
    int major = 0, minor = 0;
    if (!XRecordQueryVersion(disp, &major, &minor)) {
      fprintf(stderr, "The record extension is unavailable.\n");
      exit(1);
    }

    xrr = XRecordAllocRange();
    if (xrr == NULL) {
      fprintf(stderr, "Range allocation failed.\n");
      exit(1);
    }
    xrr->device_events.first = KeyPress;
    xrr->device_events.last  = MotionNotify;
    //xrr->device_events.last  = FocusIn;


    xrr->delivered_events.first = FocusIn;
    //xrr->delivered_events.last  = FocusOut;
    xrr->delivered_events.last  = ConfigureNotify;
    //xrr->delivered_events.last  = SelectionNotify;
    // INFO: use PropertyNotify when Copy
    // INFO: use SelectionRequest when Paste

    /*
    xrr->core_requests.first = EnterNotify;
    xrr->core_requests.last  = FocusIn;
    xrr->core_replies.first = EnterNotify;
    xrr->core_replies.last  = FocusIn;
    */

    //XRecordClientSpec xrclient = XRecordAllClients;
    XRecordClientSpec xrclient = XRecordCurrentClients;

    xrctx = XRecordCreateContext(disp, 0, &xrclient, 1, &xrr, 1);
    if (!xrctx) {
      fprintf(stderr, "Unable to create context.\n");
      exit(1);
    }

    // Clean out X events in progress
    //XFlush(disp);
    //XFlush(otherDisp);

  }
void getMousePosRelativeWindow(int root_x, int root_y,
             Window win, int &win_x, int &win_y){
    Window root = XDefaultRootWindow(display);
    Window unused_child;
    XTranslateCoordinates(display, root, win, root_x, root_y, &win_x, &win_y, &unused_child);
  }

  void start(){
    // Record...
    XFlush(disp);
    XFlush(udisp);
    evt.buttons = 0;
    evt.pressb  = 0;
    evt.clickb  = 0;
    evt.clickn  = 0;
    evt.w = 0;

    //if (!XRecordEnableContext(xdisp, xrctx, EventCallback, (XPointer)disp)) {
    if (!XRecordEnableContext(udisp, xrctx, EventCallback, (XPointer)this)) {
       fprintf(stderr, "Enable context failed\n");
       //throw new Exception("Fail");
       exit(1);  // TODO
    }
    // blocked ... until StopRecording() is called.
  }

  void stop(){
    printf("DEBUG: Stop Recording ...\n");
    if(!XRecordDisableContext(disp, xrctx)){ // FAIL
      printf("DEBUG: Stop Recording FAIL\n");
    }
    XRecordFreeContext(disp, xrctx);
    XCloseDisplay(disp);
    // XCloseDisplay(udisp);  // will cause fatal
  }

  void close(){
    //XCloseDisplay(otherDisp); // Note: N/A, blocks indefinitely
    XCloseDisplay(disp);
    XCloseDisplay(udisp);
    XFree(xrr);
  }

  std::map<KeySym,char> lut;


  void init_keysym(){
  lut[XK_asciitilde]  = '~';
  lut[XK_asciicircum] = '^';
  lut[XK_underscore] = '_';
  lut[XK_exclam]     = '!';
  lut[XK_bracketleft]     = '[';
  lut[XK_bracketright]    = ']';
  lut[XK_quotedbl]    = '"';
  lut[XK_numbersign]  = '#';
  lut[XK_dollar]      = '$';
  lut[XK_percent]     = '%';
  lut[XK_ampersand]   = '&';
  lut[XK_quoteright]  = '\'';
  //lut[XK_quoteleft]   = '\'';
  lut[XK_asterisk]    = '*';
  lut[XK_plus]        = '+';
  lut[XK_comma]       = ',';
  lut[XK_minus]       = '-';
  lut[XK_period]      = '.';
  lut[XK_question]    = '?';
  lut[XK_less]        = '<';
  lut[XK_greater]     = '>';
  lut[XK_equal]       = '=';
  lut[XK_at]          = '@';
  lut[XK_colon]       = ':';
  lut[XK_semicolon]   = ';';
  lut[XK_backslash]   = '\\';
  lut[XK_grave]       = '`';
  lut[XK_bracketleft] = '{';
  lut[XK_bracketright]= '}';
  lut[XK_bar]         = '|';
  lut[XK_parenleft]   = '(';
  lut[XK_parenright]  = ')';
  lut[XK_space]       = ' ';
  lut[XK_slash]       = '/';
  lut[XK_Tab]         = '\t';
  lut[XK_Return]      = '\n';

     evt.hasShift = false;
     evt.delay    = 0;
  }



  char KeyCode2Char(KeyCode kc){

    KeySym ks = XKeycodeToKeysym(disp, kc, evt.hasShift?1:0);


    if(lut.find(ks)!=lut.end()) return lut[ks];

    const char *kchars = XKeysymToString(ks);
    // printf("DEBUG: kchars=%s\n",kchars);

    if(strlen(kchars)==1){
     char c = kchars[0];
     if(c>='a' && c<='z' && evt.hasShift) c = toupper(c);
     return c;
    }
    return '\0';
  }

  // std::vector<KeyCode> keystack;
  std::string keychars;
  void flush_keys(){
    if(keychars.size()>0){
      printf("xdotool type \"%s\"\n", keychars.c_str());
      //printf("xdotool key Return\n", keychars.c_str());
      keychars = "";
    }
  }

  void keystroke(KeyCode kc,bool is_pressed){
    if(evt.delay > 2000) {
      flush_keys();
    }

    KeySym ks = XKeycodeToKeysym(disp, kc, 0);
    if(is_pressed){
      // keystack.push_back(kc);
      if(ks == XK_Shift_L || ks == XK_Shift_R) { evt.hasShift = true; }
      //printf("keydown %s\n",kchar);
    }else{

      char c;
      if(ks == XK_Shift_L || ks == XK_Shift_R) { 
        evt.hasShift = false;
      }else if((c = KeyCode2Char(kc)) != '\0'){
        switch(c){
          case '\n':
            keychars += "\\n";
            flush_keys();
            printf("xdotool key Return\n");
            break;
          case '\t':
            keychars += "\\t";
            flush_keys();
            printf("xdotool key Tab\n");
            break;
          default:
            keychars += c;
        }
      }else{
        const char *kchar = XKeysymToString(ks);
        flush_keys();
        printf("xdotool keyup %s\n",kchar);
      }
    }
  }

  void onMouseMove(int x, int y){
    flush_keys();
    flushMouse();
    if(evt.buttons){
      if(evt.d > 13){
        //printf("xdotool mousemove -sync %d %d\n",x,y);
        printf("xdotool mousemove %d %d\n",x,y);
        evt.d = 0;
      }
    }
  }

  void flushMouse(){
    if(evt.clickn){
      if(evt.clickn>1){
        printf("xdotool mousemove -sync %d %d click -repeat %d %d\n",evt.pressx, evt.pressy, evt.clickn, evt.clickb);
      }else{
        printf("xdotool mousemove -sync %d %d click %d\n",evt.pressx, evt.pressy, evt.clickb);
      }
      evt.clickn = 0;
      evt.clickb = 0;
    }

    if(evt.pressb){
      printf("xdotool mousemove -sync %d %d mousedown %d\n", evt.pressx, evt.pressy, evt.pressb);
      evt.pressb = 0;
    }
    evt.pressb = 0;
  }

  // TODO: use a FIFO ???
  void onMouseDown(int button, int x, int y){
    flush_keys();
    evt.d = 0; // reset mouse move distance
    if(1 && evt.w > 0){
      int wx, wy;
      getMousePosRelativeWindow(evt.x,evt.y,evt.w,wx,wy);
      std::cout<<"Relative to Window "<<evt.w<<" @ ("<<wx<<','<<wy<<")"<<std::endl;
    }

      evt.buttons |= (1<<button);

      if(evt.pressb != 0 || (evt.clickb!=0 && evt.clickb != button)){
        flushMouse();
        //printf("DEBUG: flush\n");
      //--   printf("xdotool mousemove -sync %d %d mousedown %d\n", evt.pressx, evt.pressy, evt.pressb);
      }
      evt.pressx = evt.x; evt.pressy = evt.y; evt.pressb = button;
      // evt.pressn = 0;
      // printf("xdotool mousemove -sync %d %d mousedown %d\n",evt.x, evt.y, button);
      // printf("xdotool mousedown %d\n",button);
   }

   void onMouseUp(int button, int x, int y){
      flush_keys();
      evt.d = 0;

      evt.buttons &= ~(1<<button);
      if(evt.pressx == evt.x && evt.pressy == evt.y && evt.pressb == button){
        //printf("xdotool mousemove -sync %d %d click %d\n",evt.x, evt.y, button);
        //evt.clickx = evt.x; evt.clicky = evt.y; evt.clickb = button;
        evt.pressb = 0;
        if(evt.clickb == button){ // multiple click same button
          evt.clickn++;
          //printf("DEBUG: click +1\n");
        }else{                    // multiple click different button
          flushMouse();
          evt.clickb = button;
          evt.clickn = 1;
          //printf("DEBUG: click =1\n");
        }
      } else {      // move and release
        flushMouse();
        //printf("xdotool mousemove -sync %d %d mousedown %d\n", evt.pressx, evt.pressy, evt.pressb);
        printf("xdotool mousemove -sync %d %d mouseup %d\n",   evt.x, evt.y, button);
        evt.pressb = 0;
      }
      //printf("xdotool mousemove -sync %d %d mouseup %d\n",evt.x, evt.y, button);
      //printf("xdotool mouseup %d\n",button);
  }
};


int main(int argc, const char *argv[]){
  if(argc>=2 && strcmp(argv[1],"record")==0){ // record
    GUIRecord grecord;
    printf("xdotool search -sync -name \"%s\" windowfocus\n","xterm");
    grecord.start();
  }else if(argc>=2 && strcmp(argv[1],"replay")==0){ // play
    GUIPlay gplay;
    gplay.move(10,10);
    gplay.send('a');
    gplay.send('d',1000);
    gplay.send('e',1000);
    gplay.send('f',1000);
    exit(0);
    //gplay.move(100,100,argc==2?true:false);
  }else{                                    // record
    printf("Invalid args\n");
  }
  return 0;
}

/*
typedef void (*XRecordInterceptProc) (XPointer closure, XRecordInterceptData *recorded_data)
*/
int xdo_window_move(Display *display, Window wid, int x, int y) {
  XWindowChanges wc;
  int ret = 0;
  wc.x = x;
  wc.y = y;

  XMoveWindow(display, wid, x, y);
  //ret = XConfigureWindow(display, wid, CWX | CWY, &wc);
}

 void print_window_attrs(Display *display, Window focus_win, int &wx, int &wy){
        XWindowAttributes wattrs;
        XGetWindowAttributes(display, focus_win, &wattrs);
        char *wname;
        XFetchName(display, focus_win, &wname);
        int root_x,root_y;
        Window unused_child;
        Window root = XDefaultRootWindow(display);
        XTranslateCoordinates(display, focus_win, wattrs.root, wattrs.x, wattrs.y, &root_x, &root_y, &unused_child);


        Window root_return;
        int x_return, y_return;
        unsigned int width_return, height_return;
        unsigned int border_width_return;
        unsigned int depth_return;

        XGetGeometry(display, focus_win, &root_return, &x_return, &y_return, &width_return, 
                      &height_return, &border_width_return, &depth_return);

        debug()<<"Window Info: "<<"("<< wattrs.x <<','<< wattrs.y <<") "<< wattrs.width <<'x'<< wattrs.height<<' '<< focus_win<<' '<<wattrs.root<<std::endl
               <<"             "<<"("<< root_x   <<','<<root_y<<')'<<std::endl
               <<"             "<<"("<< x_return <<','<<y_return<<") "<<width_return<<'x'<<height_return<<' '<<root_return<<(root_return==root?'=':'x')<<std::endl
               <<' '<< wname<< std::endl;

        wx = root_x; wy = root_y; // delta (+7,+24)
}


void EventCallback(XPointer p, XRecordInterceptData *idata) {

  if (XRecordFromServer == idata->category) {

    GUIRecord *grecord = (GUIRecord *) p;
    Display *disp = grecord->disp;
    Display *display = grecord->disp;

    //Display *disp = (Display *)p;

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


    debug()<<"event: "<<type<<std::endl;
    switch (type) {
      case ButtonPress: {
  	int button = xev->u.u.detail;      //   1=left, 2=middle, 3=right, 4=up, 5=down
          //printf("DEBUG: onMouseDown %d @ (%d,%d)\n", button, grecord->evt.x, grecord->evt.y);

           int x = xev->u.keyButtonPointer.rootX, y = xev->u.keyButtonPointer.rootY;
           grecord->evt.x = x;  grecord->evt.y = y;
           grecord->onMouseDown(button, x, y);

           //-- Window focus_return;
           //-- int revert_to_return;
           //-- XGetInputFocus(disp, &focus_return, &revert_to_return);
           //-- std::cout<<"Focus Window Press:"<<focus_return<<' '<<revert_to_return<<std::endl;

           /* // Below values Has no much meaning
           Window event_w = xev->u.keyButtonPointer.event;
           Window event_r = xev->u.keyButtonPointer.root;
           Window event_c = xev->u.keyButtonPointer.child;
           int event_x =  xev->u.keyButtonPointer.eventX;
           int event_y =  xev->u.keyButtonPointer.eventY;
           int root_x =  xev->u.keyButtonPointer.rootX;
           int root_y =  xev->u.keyButtonPointer.rootY;
           std::cout<<"mouse press: "<<(event_w)<<'/'<<event_r<<'/'<<event_c<<' '<<event_x<<' '<<event_y<<' '<<root_x<<' '<<root_y<<' '<<None<<std::endl;
           / */
        } break;
      case ButtonRelease: {
  	int button = xev->u.u.detail;
           //printf("DEBUG: onMouseUp %d @ (%d,%d)\n", button, grecord->evt.x, grecord->evt.y);
           int x = xev->u.keyButtonPointer.rootX, y = xev->u.keyButtonPointer.rootY;
           grecord->evt.x = x;  grecord->evt.y = y;
           grecord->onMouseUp(button, x, y);
        }
        break;
      case KeyPress: {
  	KeyCode kc = xev->u.u.detail;
  	//-- KeySym ks  = XKeycodeToKeysym(disp, kc, 0);
        //-- const char *kchar = XKeysymToString(ks);
        //-- printf("onKeyPress %d %s\n", kc, kchar);
        grecord->keystroke(kc, true);
        }
        break;
      case KeyRelease: {
  	KeyCode kc = xev->u.u.detail;

  	KeySym ks = XKeycodeToKeysym(disp, kc, 0);
  	//KeySym ks2 = XKeycodeToKeysym(disp, kc, 1);
        //printf("DEBUG: keysym %d %d\n",ks,ks2);
        const char *kchar = XKeysymToString(ks);
        //printf("DEBUG: onKeyUp %d %s\n", kc, kchar);
        if(strcmp(kchar, "Escape")==0){
          grecord->stop();
        }
        //std::cout<<"keyup "<<kchar<<std::endl;
        grecord->keystroke(kc,false);
       }
        break;
       case MotionNotify: {
           int x = xev->u.keyButtonPointer.rootX;
           int y = xev->u.keyButtonPointer.rootY;
           //grecord->evt.d += abs(x - grecord->evt.x) + abs(y - grecord->evt.y);
           grecord->evt.d += abs(x - grecord->evt.x) + abs(y - grecord->evt.y);
           grecord->evt.x = x;
           grecord->evt.y = y;
           grecord->onMouseMove(x,y);
         }
         break;
       case EnterNotify:
         printf("onEnter\n");
         break;
       case LeaveNotify:
         printf("onLeave\n");
         break;
       case FocusIn: {
           Window focus_win = xev->u.focus.window;
           grecord->flushMouse();
           Window focus_return; int revert_to_return;
           XGetInputFocus(display, &focus_return, &revert_to_return);
           if(focus_win == focus_return){
             grecord->evt.w = focus_win;
             int wx, wy;
             print_window_attrs(display, focus_win, wx, wy);
             int xdot_x = wx-7, xdot_y = wy-24;
             std::cout<<"xdotool getwindowfocus getwindowgeometry windowmove --sync "<<xdot_x<<' '<<xdot_y<<" getwindowgeometry"<<std::endl;
             // TODO: add size support
             // xdotool getwindowfocus getwindowgeometry windowmove 292 94 getwindowgeometry

             //xdo_window_move(display, focus_win, wx, wy);

             //print_window_attrs(display, focus_win, wx, wy);

             //--XGetGeometry(display, root_return, &root_return, &x_return, &y_return, &width_return, 
             //--         &height_return, &border_width_return, &depth_return);
             //--debug()<<"win root "<<x_return<<' '<<y_return<<' '<<width_return<<' '<<height_return<<std::endl;
/*
    XWindowAttributes attr;
    XGetWindowAttributes(display, focus_win, &attr);
    XTranslateCoordinates(xdo->xdpy, focus_win, attr.root, attr.x, attr.y, &root_x, &root_y, &unused_child);
    debug()<<"root "<<win_x<<','<<win_y<<' '<<root_x<<','<<root_y<<std::endl;
*/


           }
           //TODO: handle more on window stack change
         } break;
       case FocusOut: {
           grecord->flushMouse();
           Window focus_win = xev->u.focus.window;
           if(grecord->evt.w == focus_win){
             char *wname;
             XFetchName(display, focus_win, &wname);
             debug()<<"FocusOut Window: "<<focus_win<<' '<<wname<<std::endl;
             grecord->evt.w = 0;
           }
         } break;
       case KeymapNotify:
       case Expose: case GraphicsExpose: case NoExpose:
         break;
       case CreateNotify:
       case DestroyNotify:
         break;
       case MapNotify:
         debug()<<"MapNotify Window"<<std::endl;
         break;
       case UnmapNotify:
         debug()<<"UnmapNotify Window"<<std::endl;
         break;
       case ConfigureNotify: {
           int w_window = xev->u.configureRequest.window;
           int w_x = xev->u.configureRequest.x;
           int w_y = xev->u.configureRequest.y;
           int w_w = xev->u.configureRequest.width;
           int w_h = xev->u.configureRequest.height;
           debug()<<"Config Window: "<<w_window<<" @ ("<<w_x<<','<<w_y<<") "<<w_w<<'x'<<w_h<<std::endl;
         } break;
       case PropertyNotify:{
        // it's possible Copy action
         } break;
       case SelectionRequest: {
          // INFO: use SelectionRequest when Paste
         } break;
       default:
         //std::cout<<"event type: "<<type<<std::endl;
         break;
    }
    ////
    //HandleEvent(re);
    ////
  }

  if (idata != NULL)  XRecordFreeData(idata);
}
