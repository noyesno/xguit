
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>
#include <X11/Xatom.h>

#include <X11/keysym.h>
#include <X11/StringDefs.h>
#include <X11/extensions/record.h>     // -lXtst
#include <X11/extensions/XTest.h>     // -lXtst

#include <tcl.h>
#include <string>
#include <algorithm>
#include <map>
#include <iostream>


#include "debug.hpp"

#include "XRRHelper.hpp"

struct XEnv {
  Display *display;
  long     delay_after_click;
  struct {
    bool relative;
  } mouse;


  struct {
    long wait;   // wait after findWindow
    long click;  // wait after mouse click
    long move;   // wait between mouse move
    long stroke; // wait after key stroke
  } delay;

  struct {
    Window     window;
    XRectangle rect;
  } focus;
};


XEnv xenv;
XRRHelper xhelper;



class XRepalyHelper {
  Display *display;

public:
  XRepalyHelper(Display *disp):display(disp){
  }


};

int x_window2ppm(Display *display, Window window, int x, int y, int width, int height, const char *ofile){
    XImage* ximg = XGetImage(display, window, x, y, width, height, AllPlanes, ZPixmap);

    unsigned long red_mask   = ximg->red_mask;
    unsigned long green_mask = ximg->green_mask;
    unsigned long blue_mask  = ximg->blue_mask;

    int n_blue=0, n_green=0, n_red=0;
    for(unsigned long v = blue_mask; v!=0;  n_blue++)  v=v>>1;
    for(unsigned long v = green_mask>>n_blue; v!=0; n_green++) v=v>>1;
    for(unsigned long v = red_mask>>(n_blue+n_green); v!=0;   n_red++)   v=v>>1;
    int max_blue  = blue_mask;
    int max_green = green_mask>>n_blue;
    int max_red   = red_mask>>(n_blue+n_green);

    FILE *fout = fopen(ofile, "w");

    fprintf(fout, "P3 %d %d %d\n", width, height, 255);
    fprintf(fout, "# depth = %d\n", ximg->depth);
    fprintf(fout, "# mask  = %04x %04x %04x\n", red_mask, green_mask, blue_mask);
    int nbits_mask = ximg->depth/3;
    //TODO: if bits per color not the same?
    for (int yp=0; yp<height; yp++) {
      for (int xp=0; xp<width; xp++) {
        unsigned long pix = XGetPixel(ximg, xp, yp);

        unsigned int red   = (pix & red_mask)  >> (n_blue+n_green);
        unsigned int green = (pix & green_mask)>> n_blue;
        unsigned int blue  =  pix & blue_mask;
        //TODO: How to scale color??
        fprintf(fout, "%ld %ld %ld\n", red*255/max_red, green*255/max_green, blue*255/max_blue);
        //fprintf(fout, "%ld %ld %ld\n", red<<(8-n_red), green<<(8-n_green), blue<<(8-n_blue));

        //-- XColor color;
        //-- color.pixel = pix;
        //-- XQueryColor(display, XDefaultColormap(display, XDefaultScreen(display)), &color);
        //-- fprintf(fout, "%d %d %d\n", color.red/255, color.green/255, color.blue/255);
      }
    }
    fclose(fout);
    XDestroyImage(ximg);
    return 1;
}





int TclObjCmd_x_connect(ClientData clientData, Tcl_Interp *interp,
        int objc,Tcl_Obj *CONST objv[]) {
    XEnv *xenv = (XEnv *)(clientData);

    const char *display_name = NULL;
    if(objc==2){
      display_name = Tcl_GetString(objv[1]);
    }
    display_name = XDisplayName(display_name);
    if(strlen(display_name)==0){
      printf("Error: No display name specified\n");
      return TCL_ERROR;
    }

    Display *display = XOpenDisplay(display_name);
    if(display == NULL){
      printf("Error: Fail to connect display %s\n", display_name);
      return TCL_ERROR;
    }
    printf("Info: Use display %s\n", display_name);

    int eventnum = 0, errornum = 0, majornum = 0, minornum = 0;

    if (!XTestQueryExtension(display, &eventnum, &errornum, &majornum, &minornum)) {
       printf("Error: XTest is not supported on display %s\n", "null");
    }

    printf("Info: Use XTest %d.%d\n",majornum,minornum);

    DefaultScreen(display);

    XSynchronize(display, True);
    XTestGrabControl(display, True);

    xenv->display = display;
    xenv->delay_after_click = 800000; // 800ms, 1s = 1000000us
    xhelper = XRRHelper(display);
    return TCL_OK;
}

int TclObjCmd_x_sleep(ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[]) {

  int ms;
  Tcl_GetIntFromObj(interp, objv[1], &ms);
  usleep(ms*1000);
  return TCL_OK;
}


int TclObjCmd_x_mouse (ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[]) {
  XEnv *xenv = (XEnv *)(clientData);

  std::string action = Tcl_GetString(objv[1]);
  int argv_idx = 1;

  Display *display = xenv->display;
  if(action=="move"){
    int x, y;
    Tcl_GetIntFromObj(interp, objv[2], &x);
    Tcl_GetIntFromObj(interp, objv[3], &y);

    if(xenv->mouse.relative){
      x = x + xenv->focus.rect.x; y = y + xenv->focus.rect.y;
    }

    XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
    XSync(display, False);
  }else if(action=="click"){ // x::mouse click 1 x y -repeat 2 -delay 3
    int button;
    Tcl_GetIntFromObj(interp, objv[2], &button);
    if(objc>=5){
      int x,y;
      Tcl_GetIntFromObj(interp, objv[3], &x);
      Tcl_GetIntFromObj(interp, objv[4], &y);
      if(xenv->mouse.relative){
        x = x + xenv->focus.rect.x; y = y + xenv->focus.rect.y;
      }
      XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
      XSync(display, False);
    }
    int ms_delay = 100;
    int n_repeat = 1;
    for(int i=3;i<objc;i++){
      std::string arg = Tcl_GetString(objv[i]);
      if(arg=="-delay"){ Tcl_GetIntFromObj(interp, objv[++i], &ms_delay); }
      else if(arg=="-repeat"){ Tcl_GetIntFromObj(interp, objv[++i], &n_repeat); }
    }

    usleep(xenv->delay_after_click);
    XTestFakeButtonEvent(display, button, True, CurrentTime);
    usleep(ms_delay*1000); // 100ms interval between click TODO
    XTestFakeButtonEvent(display, button, False, CurrentTime);
    XSync(display, False); // XFlush(display);
    usleep(xenv->delay_after_click);
    // TODO: double click
  }else if(action=="press"){
    int button;
    Tcl_GetIntFromObj(interp, objv[2], &button);
    if(objc==5){
      int x,y;
      Tcl_GetIntFromObj(interp, objv[3], &x);
      Tcl_GetIntFromObj(interp, objv[4], &y);
      if(xenv->mouse.relative){
        x = x + xenv->focus.rect.x; y = y + xenv->focus.rect.y;
      }
      XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
      XFlush(display);
    }
    usleep(xenv->delay_after_click);
    XTestFakeButtonEvent(display, button, True, CurrentTime);
    XSync(display, False); // XFlush(display);
    usleep(xenv->delay_after_click);
  }else if(action=="release"){
    int button;
    Tcl_GetIntFromObj(interp, objv[2], &button);
    if(objc==5){
      int x,y;
      Tcl_GetIntFromObj(interp, objv[3], &x);
      Tcl_GetIntFromObj(interp, objv[4], &y);
      if(xenv->mouse.relative){
        x = x + xenv->focus.rect.x; y = y + xenv->focus.rect.y;
      }
      XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
      XFlush(display);
    }
    usleep(xenv->delay_after_click);
    XTestFakeButtonEvent(display, button, False, CurrentTime);
    XSync(display, False); // XFlush(display);
    usleep(xenv->delay_after_click);
  }else if(action=="drag"){
    int button;
    Tcl_GetIntFromObj(interp, objv[2], &button);

    int x,y;
    // move
    Tcl_GetIntFromObj(interp, objv[3], &x);
    Tcl_GetIntFromObj(interp, objv[4], &y);
    if(xenv->mouse.relative){
      x = x + xenv->focus.rect.x; y = y + xenv->focus.rect.y;
    }
    XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
    // press
    usleep(xenv->delay_after_click);
    XTestFakeButtonEvent(display, button, True, CurrentTime);
    usleep(xenv->delay_after_click);

    for(int i=5; i<objc; i+=2){
      Tcl_GetIntFromObj(interp, objv[i], &x);
      Tcl_GetIntFromObj(interp, objv[i+1], &y);
      if(xenv->mouse.relative){
        x = x + xenv->focus.rect.x; y = y + xenv->focus.rect.y;
      }
      XTestFakeMotionEvent(display, -1, x, y, CurrentTime);
    }
    //release
    usleep(xenv->delay_after_click);
    XTestFakeButtonEvent(display, button, False, CurrentTime);
    XSync(display, False); // XFlush(display);
    usleep(xenv->delay_after_click);
  }

  return TCL_OK;
}

int TclObjCmd_x_keyboard (ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[]) {
  XEnv *xenv = (XEnv *)(clientData);

  std::string action = Tcl_GetString(objv[1]);
  int argv_idx = 1;

  Display *display = xenv->display;

  if(action=="press"){
    const char *keyname = Tcl_GetString(objv[2]);
    bool hasShift=false;
    KeyCode kc = xhelper.String2KeyCode(keyname, hasShift);
    if(kc){
      XTestFakeKeyEvent(display, kc, True, CurrentTime);
      XSync(display, False); // XFlush(display);
    }
  }else if(action=="release"){
    const char *keyname = Tcl_GetString(objv[2]);
    bool hasShift=false;
    KeyCode kc = xhelper.String2KeyCode(keyname, hasShift);
    if(kc){
      XTestFakeKeyEvent(display, kc, False, CurrentTime);
    }
    XSync(display, False); // XFlush(display);
  }else if(action=="stroke" || action=="click"){

    const char *keyname = Tcl_GetString(objv[2]);
    bool hasCtrl=false, hasShift=false, hasAlt=false;
    KeyCode kc = xhelper.String2KeyCode(keyname, hasCtrl, hasShift, hasAlt);

    if(kc==0){
      return TCL_OK;
    }

    if(hasCtrl){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Control_L), True, CurrentTime);
    }
    if(hasAlt){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Alt_L), True, CurrentTime);
    }
    if(hasShift){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), True, CurrentTime); // press SHIFT
    }

    XTestFakeKeyEvent(display, kc, True,  CurrentTime); // is_press
    XTestFakeKeyEvent(display, kc, False, CurrentTime); // is_press = false
    if(hasShift){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), False, CurrentTime); // release SHIFT
    }
    if(hasAlt){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Alt_L), False, CurrentTime);
    }
    if(hasCtrl){
      XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Control_L), False, CurrentTime);
    }
    XSync(display, False); // XFlush(display);
    usleep(50000); // 50ms
  }else if(action=="type"){
    const char *chars = Tcl_GetString(objv[2]);
    for(int i=0; i<strlen(chars); i++){
      char c = chars[i];
      bool need_shift=false;
      KeyCode kc = xhelper.Char2KeyCode(c, need_shift);
      if(need_shift){
        XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), True, CurrentTime); // press SHIFT
      }
      XTestFakeKeyEvent(display, kc, True,  CurrentTime); // is_press
      XTestFakeKeyEvent(display, kc, False, CurrentTime); // is_press = false
      if(need_shift){
        XTestFakeKeyEvent(display, XKeysymToKeycode(display, XK_Shift_L), False, CurrentTime); // release SHIFT
      }
      usleep(30000); // 30ms
    }
    XSync(display, False); // XFlush(display);
  }

  return TCL_OK;
}

int TclObjCmd_x_window (ClientData clientData, Tcl_Interp *interp,
        int objc, Tcl_Obj *CONST objv[]) {
  XEnv *xenv = (XEnv *)(clientData);
  Display *display = xenv->display;

  Window focus;
  int    revert_to_return;
  XGetInputFocus(display, &focus, &revert_to_return);
  if(focus==None){ // wid |PointerRoot | None
    printf("No Focus Window\n");
    return TCL_OK;
  }

  if(objc == 1){
    Tcl_SetObjResult(interp,  Tcl_NewIntObj(focus));
    return TCL_OK;
  }
  std::string action = Tcl_GetString(objv[1]);
  int argv_idx = 1;

  if(action=="name"){
    char *wname;
    XFetchName(display, focus, &wname);
    // TODO: use XGetWMName instead?
    Tcl_SetObjResult(interp, Tcl_NewStringObj(wname, strlen(wname)));
    XFree(wname);
    return TCL_OK;
  }else if(action=="size"){
    XWindowAttributes wattrs;
    XGetWindowAttributes(display, focus, &wattrs);

    int root_x,root_y;
    Window unused_child;
    Window root = XDefaultRootWindow(display);
    XTranslateCoordinates(display, focus, root, wattrs.x, wattrs.y, &root_x, &root_y, &unused_child);

    Tcl_Obj *rv = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(root_x));
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(root_y));
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(wattrs.width));
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(wattrs.height));
    Tcl_SetObjResult(interp, rv);
    return TCL_OK;
  }else if(action=="info"){
    //-- XRRHelper xhelper(display);
    //-- int wx, wy, wwidth, wheight;
    //-- xhelper.getWindowSize(focus, wx, wy, wwidth, wheight);
    if(objc>=3) {
      int v;
      Tcl_GetIntFromObj(interp, objv[2], &v);
      focus = v;
    }

    char *wname;
    XFetchName(display, focus, &wname);
    // TODO: use XGetWMName instead?
    Tcl_SetObjResult(interp, Tcl_NewStringObj(wname, strlen(wname)));

    XWindowAttributes wattrs;
    XGetWindowAttributes(display, focus, &wattrs);

    int root_x,root_y;
    Window unused_child;
    Window root = XDefaultRootWindow(display);
    XTranslateCoordinates(display, focus, root, wattrs.x, wattrs.y, &root_x, &root_y, &unused_child);

    printf("Window #%x: depth=%d, override=%d, root=%ld @ (%d,%d) %dx%d | %s\n", focus, wattrs.depth,
            wattrs.override_redirect, wattrs.root,
            root_x, root_y, wattrs.width, wattrs.height, wname
    );
    Tcl_Obj *rv = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(root_x));
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(root_y));
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(wattrs.width));
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(wattrs.height));
    Tcl_ListObjAppendElement(interp, rv, Tcl_NewIntObj(wattrs.root));
    Tcl_SetObjResult(interp, rv);
    XFree(wname);
    return TCL_OK;
  }else if(action=="move"){
    if(objc<4) return TCL_ERROR;

    int x, y;
    Tcl_GetIntFromObj(interp, objv[2], &x);
    Tcl_GetIntFromObj(interp, objv[3], &y);
    int ret = XMoveWindow(display, focus, x, y);
    XSync(display, False); // XFlush(display);
    Tcl_SetObjResult(interp,  Tcl_NewIntObj(ret));
    usleep(200000); // 100ms
    return TCL_OK;
  }else if(action=="resize"){
    if(objc<4) return TCL_ERROR;

    int x, y;
    Tcl_GetIntFromObj(interp, objv[2], &x);
    Tcl_GetIntFromObj(interp, objv[3], &y);
    int ret = XResizeWindow(display, focus, x, y);
    XSync(display, False); // XFlush(display);
    Tcl_SetObjResult(interp,  Tcl_NewIntObj(ret));
    usleep(200000); // 100ms
    return TCL_OK;
  }else if(action=="config"){
    if(objc<6) return TCL_ERROR;

    // TODO: use  XMoveResizeWindow()?
    XWindowChanges wc;
    Tcl_GetIntFromObj(interp, objv[2], &(wc.x));
    Tcl_GetIntFromObj(interp, objv[3], &(wc.y));
    Tcl_GetIntFromObj(interp, objv[4], &(wc.width));
    Tcl_GetIntFromObj(interp, objv[5], &(wc.height));
    int ret = XConfigureWindow(display, focus, CWX|CWY|CWWidth|CWHeight, &wc);
    XSync(display, False); // XFlush(display);
    Tcl_SetObjResult(interp,  Tcl_NewIntObj(ret));
    usleep(200000); // 100ms
    return TCL_OK;
  }else if(action=="attr"){
    // TODO: need FIX
    const char *name = Tcl_GetString(objv[2]);
    std::string value = xhelper.getWindowPropery(focus, name);
    Tcl_SetObjResult(interp,  Tcl_NewStringObj(value.c_str(),value.size()));
    return TCL_OK;
  }else if(action=="wait"){

    bool find_focus = false;
    std::string wname;
    std::string wtype;
    std::string wrect;

    for(int i=2;i<objc;i++){
      const char *str = Tcl_GetString(objv[i]);
      if(strcmp("-focus",str)==0){
        find_focus = true;
      }else if(strcmp("-name",str)==0){
        wname = Tcl_GetString(objv[++i]);
      }else if(strcmp("-type",str)==0){
        wtype = Tcl_GetString(objv[++i]);
      }else if(strcmp("-geometry",str)==0){
        wrect = Tcl_GetString(objv[++i]);
      }
    }

    debug()<<"wait window "<<(find_focus?"-focus":"")<<" -type "<<wtype<<" -name "<<wname<<' '<<wrect<<std::endl;
    Window found=0;
    while(found==0){
      if(find_focus){
        Window focus = xhelper.focus();
        std::string _wtype = xhelper.getWindowType(focus);
        std::string _wname = xhelper.getWindowName(focus);
        if(_wtype==wtype && _wname==wname) found=focus;
      }else{
        found = xhelper.findWindow(wtype, wname);
      }
      usleep(xenv->delay.wait); // default 200ms
    }

    if(found!=0){
      xhelper.moveWindow(found, wrect.c_str());
      XRectangle rect = xhelper.getWindowRectangle(found);
      xenv->focus.rect = rect;
    }

    Tcl_SetObjResult(interp,  Tcl_NewIntObj(found));
    return TCL_OK;
  }else if(action=="find"){

      const char *wtype= Tcl_GetString(objv[2]);

      Window found = xhelper.findWindow(wtype,"");
 

      Tcl_SetObjResult(interp,  Tcl_NewIntObj(found));
  }else if(action=="image"){
    if(objc<3) return TCL_ERROR;
    int x=0, y=0, width=130, height=130;

    const char *ofile = Tcl_GetString(objv[2]);

    if(objc==7){
      Tcl_GetIntFromObj(interp, objv[3], &x);
      Tcl_GetIntFromObj(interp, objv[4], &y);
      Tcl_GetIntFromObj(interp, objv[5], &width);
      Tcl_GetIntFromObj(interp, objv[6], &height);
    }else{
      XWindowAttributes wattrs;
      XGetWindowAttributes(display, focus, &wattrs);
      x=wattrs.x; y=wattrs.y; width=wattrs.width; height=wattrs.height;
    }

    int rv = x_window2ppm(display, focus, x, y, width, height, ofile);
    return rv?TCL_OK:TCL_ERROR;
  }

  return TCL_OK;
}


#ifdef __cplusplus
extern "C" {
#endif
/*
 *  xreplay::open
 *  xreplay::close
 *
 *  xreplay::key press L
 *  xreplay::key release L
 *  xreplay::key stroke L
 *
 *  xreplay click left
 *
 *  xreplay mousedn 1 x y
 *  xreplay mouseup 1 x y
 *  xreplay mousedrag x1 y1 x2 y2
 *
 *  xreplay keydn
 *  xreplay keyup
 *  xreplay keytype
 *
 *  xreplay winmove
 *  xreplay winresize
 *
 *  xreplay assert
 *  xreplay assert window {{x $x} {y $y} {width $width} {height $height}} ;# XWindowChanges
 *  xreplay windows
 */

//int __declspec(dllexport) XReplay_Init (Tcl_Interp *interp) {



int Xreplay_Init (Tcl_Interp *interp) {

#ifdef USE_TCL_STUBS
  // -DUSE_TCL_STUBS && -ltclstub8.3
    if(Tcl_InitStubs(interp,"8.1",0)==NULL) {
        return TCL_ERROR;
    }
#else
    if(Tcl_PkgRequire(interp, "Tcl", "8.3", 0)==NULL){
        return TCL_ERROR;
    }
#endif

    xenv.mouse.relative = true;
    xenv.delay.wait = 200000; // 200ms

    Tcl_CreateObjCommand (interp,"::x::connect",TclObjCmd_x_connect,
            (ClientData)(&xenv), (Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand (interp,"::x::mouse", TclObjCmd_x_mouse,
            (ClientData)(&xenv),(Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateObjCommand (interp,"::x::key", TclObjCmd_x_keyboard,
            (ClientData)(&xenv),(Tcl_CmdDeleteProc *)NULL);

    Tcl_CreateObjCommand (interp,"::x::sleep", TclObjCmd_x_sleep,
            (ClientData)(&xenv),(Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateObjCommand (interp,"::x::window", TclObjCmd_x_window,
            (ClientData)(&xenv),(Tcl_CmdDeleteProc *)NULL);
    //Tcl_PkgProvide(interp,"XReplay","0.1");

    // Tcl_ParseArgsObjv(); // No Available in early Tcl 

    return TCL_OK;
}

#ifdef __cplusplus
}
#endif
