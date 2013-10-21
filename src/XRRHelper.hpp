#ifndef _XRRHelper_hpp_
#define _XRRHelper_hpp_


#include "debug.hpp"

#include <X11/Xatom.h>


class XRRHelper {

public:
  Display *display;
  Window  root;

public:
  XRRHelper():display(NULL){}
  XRRHelper(Display *display):display(display){
    root=XDefaultRootWindow(display);
    init_keysym();
  }

  int WindowCount(Window window=0, bool verbose=false){
      Window    wRoot;
      Window    wParent;
      Window   *wChild;
      unsigned  nChildren;
      if(window==0) window = root;
      if(0 != XQueryTree(display, window, &wRoot, &wParent, &wChild, &nChildren)) {
        if(verbose){
           printf("XQueryTree: /%d/%d/[%d]/+%d\n", wRoot, wParent, window, nChildren);
        }
        return nChildren;
      }
      return -1;
  }

  Window findWindow(std::string type, std::string name, Window window=0){
      Window    wRoot;
      Window    wParent;
      Window   *wChild;
      unsigned  nChildren=0;
      Window found = 0;
      if(window==0) window = root;

      if(0 == XQueryTree(display, window, &wRoot, &wParent, &wChild, &nChildren)) return 0;

      if(nChildren==0) return 0;

      printf("nChildren = %d %d\n", window, nChildren);
      for(int i=nChildren-1;i>=0 && found==0; i--){
          std::string _wtype = getWindowType(wChild[i]);
          std::string _wname = getWindowName(wChild[i]);
          if(_wtype==type && _wname==name){
            found=wChild[i];
          }else{
            found = findWindow(type, name, wChild[i]);
          }
      }
      XFree(wChild);
      return found;
  }

  int getWindowParents(Window window){
      Window    wRoot;
      Window    wParent;
      Window   *wChild;
      unsigned  nChildren;
      printf("Window Parents: %d",window);
      while(1){
        if(window==root) break;
        if(0 == XQueryTree(display, window, &wRoot, &wParent, &wChild, &nChildren)) break;
        printf("<%d", wParent);
        window=wParent;
      }
      printf("\n");
      return -1;
  }

  Window getWindowParent(Window window){
      Window    wRoot;
      Window    wParent = 0;
      Window   *wChild;
      unsigned  nChildren;
      if(XQueryTree(display, window, &wRoot, &wParent, &wChild, &nChildren)) return wParent;
      return wParent;
  }

  Window focus(){
    Window window;
    int    revert_to_return;
    XGetInputFocus(display, &window, &revert_to_return);
    return window;
  }
  bool isRootWindow(Window wid){
    return wid==root;
  }

  std::string getWindowName(Window wid){
    if(wid==0) wid = focus();
    char *wname;
    Status status = XFetchName(display, wid, &wname); // TODO: use XGetWMName instead?
    if(status && wname!=NULL){
      std::string rv(wname);
      XFree(wname);
      return rv;
    }
    return "";
  }

  int getWindowSize(Window wid, int &x, int &y, unsigned int &width, unsigned int &height){
    XRectangle rect = getWindowRectangle(wid);
    x = rect.x; y=rect.y; width=rect.width; height=rect.height;
    return 1;
  }

  XRectangle getWindowRectangle(Window wid){
    if(wid==0) wid = focus();
    Window root_return;
    unsigned int border_width_return;
    unsigned int depth_return;
    int wx, wy;
    int x, y; 
    unsigned int width, height;
    XGetGeometry(display, wid, &root_return, &wx, &wy, &width, &height, &border_width_return, &depth_return);
    Window unused_child;
    //Window root = XDefaultRootWindow(display);
    // TODO:
    XTranslateCoordinates(display, getWindowParent(wid), root, wx, wy, &x, &y, &unused_child);

    XRectangle rv;
    rv.x = x; rv.y = y; rv.width=width; rv.height=height;
    return rv;
  }


  std::string getWindowGeometry(Window wid){
    XRectangle rect = getWindowRectangle(wid);

    char buf[64];
    sprintf(buf, "%dx%d%+d%+d", rect.width, rect.height, rect.x, rect.y);
    return std::string(buf);
  }

  std::string Rectangle2Geometry(XRectangle rect){
    char buf[64];
    sprintf(buf, "%dx%d%+d%+d", rect.width, rect.height, rect.x, rect.y);
    return std::string(buf);
  }

  //-- XWindowAttributes getWindowAttributes(Window wid){
  //--   if(wid==0) wid = focus();
  //--   XWindowAttributes wattrs;
  //--   XGetWindowAttributes(display, wid, &wattrs);


  //--   int root_x,root_y;
  //--   Window unused_child;
  //--   //TODO
  //--   XTranslateCoordinates(display, wid, root, wattrs.x, wattrs.y, &root_x, &root_y, &unused_child);
  //--   wattrs.x = root_x; wattrs.y = root_y;
  //--   return wattrs;
  //-- }

  std::map<KeySym, char> lut;
  std::map<std::string, KeySym> s2k;


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

s2k["BAC"] =  XK_BackSpace;
s2k["BKS"] =  XK_BackSpace;
s2k["BRE"] =  XK_Break;
s2k["BS"] =  XK_BackSpace;
s2k["CAN"] =  XK_Cancel;
s2k["CAP"] =  XK_Caps_Lock;
s2k["DEL"] =  XK_Delete;
s2k["DOW"] =  XK_Down;
s2k["END"] =  XK_End;
s2k["ENT"] =  XK_Return;
s2k["ESC"] =  XK_Escape;
s2k["F10"] =  XK_F10;
s2k["F11"] =  XK_F11;
s2k["F12"] =  XK_F12;
s2k["F1"] =  XK_F1;
s2k["F2"] =  XK_F2;
s2k["F3"] =  XK_F3;
s2k["F4"] =  XK_F4;
s2k["F5"] =  XK_F5;
s2k["F6"] =  XK_F6;
s2k["F7"] =  XK_F7;
s2k["F8"] =  XK_F8;
s2k["F9"] =  XK_F9;
s2k["HEL"] =  XK_Help;
s2k["HOM"] =  XK_Home;
s2k["INS"] =  XK_Insert;
s2k["LAL"] =  XK_Alt_L;
s2k["LCT"] =  XK_Control_L;
s2k["LEF"] =  XK_Left;
s2k["LMA"] =  XK_Meta_L;
s2k["LSH"] =  XK_Shift_L;
s2k["LSK"] =  XK_Super_L;
s2k["MNU"] =  XK_Menu;
s2k["NUM"] =  XK_Num_Lock;
s2k["\n"] =  XK_Return;
s2k["PGD"] =  XK_Next;
s2k["PGU"] =  XK_Prior;
s2k["PRT"] =  XK_Print;
s2k["RAL"] =  XK_Alt_R;
s2k["RCT"] =  XK_Control_R;
s2k["RIG"] =  XK_Right;
s2k["RMA"] =  XK_Meta_R;
s2k["RSH"] =  XK_Shift_R;
s2k["RSK"] =  XK_Super_R;
s2k["SCR"] =  XK_Scroll_Lock;
s2k["SPA"] =  XK_space;
s2k["SPC"] =  XK_space;
s2k["TAB"] =  XK_Tab;
s2k["\t"] =  XK_Tab;
s2k["UP"] =  XK_Up;
s2k["&"] =  XK_ampersand;
s2k["^"] =  XK_asciicircum;
s2k["~"] =  XK_asciitilde;
s2k["*"] =  XK_asterisk;
s2k["@"] =  XK_at;
s2k["\\"] =  XK_backslash;
s2k["|"] =  XK_bar;
s2k["{"] =  XK_braceleft;
s2k[";"] =  XK_braceright;
s2k["["] =  XK_bracketleft;
s2k["]"] =  XK_bracketright;
s2k[":"] =  XK_colon;
s2k[","] =  XK_comma;
s2k["$"] =  XK_dollar;
s2k["="] =  XK_equal;
s2k["!"] =  XK_exclam;
s2k["`"] =  XK_grave;
s2k[">"] =  XK_greater;
s2k["<"] =  XK_less;
s2k["-"] =  XK_minus;
s2k["#"] =  XK_numbersign;
s2k["("] =  XK_parenleft;
s2k[")"] =  XK_parenright;
s2k["%"] =  XK_percent;
s2k["."] =  XK_period;
s2k["+"] =  XK_plus;
s2k["?"] =  XK_question;
s2k["\""] =  XK_quotedbl;
s2k["'"] =  XK_quoteright;
s2k[";"] =  XK_semicolon;
s2k["/"] =  XK_slash;
s2k[" "] =  XK_space;
s2k["_"] =  XK_underscore;

  }



  char KeyCode2Char(KeyCode kc, bool hasShift){

    KeySym ks = XKeycodeToKeysym(display, kc, hasShift?1:0);


    if(lut.find(ks)!=lut.end()) return lut[ks];

    const char *kchars = XKeysymToString(ks);
    // printf("DEBUG: kchars=%s\n",kchars);

    if(strlen(kchars)==1){
     char c = kchars[0];
     if(c>='a' && c<='z' && hasShift) c = toupper(c);
     return c;
    }
    return '\0';
  }

  KeyCode Char2KeyCode(char key, bool &need_shift){
    const char buf[2] = {key,'\0'};
    return String2KeyCode(buf, need_shift);
  }

  KeyCode String2KeyCode(const char *keyname, bool &need_shift){

    KeySym ksym = XStringToKeysym(keyname);

    if(ksym==NoSymbol){
      std::map<std::string, KeySym>::iterator it = s2k.find(keyname);
      if(it != s2k.end()) ksym = it->second;
    }
    if(ksym==NoSymbol){
      printf("Error: unknown key name: %s\n", keyname);
      return 0;
    }

    KeyCode kc = XKeysymToKeycode(display, ksym);

    need_shift=false;
    KeySym ksl, ksu;
    XConvertCase(ksym, &ksl, &ksu);
    int syms = 0;
    KeySym *kss = XGetKeyboardMapping(display, kc, 1, &syms);
    if(ksym==ksl && ksym==ksu && ksym==kss[0]) need_shift=false;
    else if(ksym==ksl && ksl!=ksu)             need_shift=false;
    else                                       need_shift=true;
    XFree(kss);

    return kc;
  }

  /***********************************************************
   * e.g.  Ctrl+Shift+Alt+F;
   ***********************************************************/
  KeyCode String2KeyCode(const char *name, bool &hasCtrl, bool &hasShift, bool &hasAlt){
    std::string key(name);

    char buf[64];
    strcpy(buf, name);
    for(char *pch = strtok(buf,"+"); pch!=NULL; pch=strtok (NULL, "+")){
      if(strcmp(pch,"Ctrl")==0) hasCtrl = true;
      else if(strcmp(pch,"Shift")==0) hasShift= true;
      else if(strcmp(pch,"Alt")==0) hasAlt = true;
      else key=pch;
    }
    bool need_shift;
    KeyCode kc = String2KeyCode(key.c_str(), need_shift);
    hasShift = (hasCtrl||hasAlt)?hasShift:need_shift;
    return kc;
  }

  std::string getWindowPropery(Window window, Atom property){
    Atom type;
    int format;
    unsigned long len, remaining;
    unsigned char* data;
    // TODO: check whether it is a X Window ID?
    int size = 64;  // 64*4=256 bytes
    int result = XGetWindowProperty( display,
                                     window,
                                     property,
                                     0, // Offset
                                     size, // Result length to return
                                     false, // Delete false
                                     AnyPropertyType, // Requested type // XA_STRING
                                     &type,      // Actual return type
                                     &format,    // Actual return format
                                     &len,       // Actual returned length.
                                     &remaining, // Bytes remaining after the return
                                     &data );
     if (result != Success ) {
       std::cout<<"FAIL to XGetWindowProperty"<<std::endl;
       return "";
     }

     if(remaining>0){
       size = size*4 + remaining;
       XFree(data);
       result = XGetWindowProperty(display,
                                   window,
                                   property,
                                   0, // Offset
                                   size, // Result length to return
                                   false, // Delete false
                                   AnyPropertyType,  // Requested type
                                   &type,      // Actual return type
                                   &format,    // Actual return format
                                   &len,       // Actual returned length.
                                   &remaining, // Bytes remaining after the return
                                   &data );
     }
     std::string value;
     if(format==XA_STRING){
       value = std::string((const char *)data);
     }else if(format==XA_ATOM){
       char *atom_name = XGetAtomName(display, *(Atom *)data);
       value = atom_name;
       XFree(atom_name);
     }
     XFree(data);
     return value;
}

  std::string getWindowPropery(Window window, const char *name){
    Atom property = XInternAtom(display, name, false );
    return getWindowPropery(window, property);
  }

  std::string getWindowType(Window window){
    Atom property = XInternAtom(display, "_NET_WM_WINDOW_TYPE", true);
    if(property==None) {
      printf("Atom %s not exist\n", "_NET_WM_WINDOW_TYPE");
      return "----";
    }
    Atom type;
    int format;
    unsigned long len, remaining;
    unsigned char* data;
    // TODO: check whether it is a X Window ID?
    int size = 64;  // 64*4=256 bytes
    int result = XGetWindowProperty( display,
                                     window,
                                     property,
                                     0, // Offset
                                     size, // Result length to return
                                     false, // Delete false
                                     AnyPropertyType, // Requested type // XA_STRING
                                     &type,      // Actual return type
                                     &format,    // Actual return format
                                     &len,       // Actual returned length.
                                     &remaining, // Bytes remaining after the return
                                     &data );
     if (result != Success ) {
       std::cout<<"FAIL to XGetWindowProperty"<<std::endl;
       return "";
     }

     if(remaining>0){
       size = size*4 + remaining;
       XFree(data);
       result = XGetWindowProperty(display,
                                   window,
                                   property,
                                   0, // Offset
                                   size, // Result length to return
                                   false, // Delete false
                                   AnyPropertyType,  // Requested type
                                   &type,      // Actual return type
                                   &format,    // Actual return format
                                   &len,       // Actual returned length.
                                   &remaining, // Bytes remaining after the return
                                   &data );
     }
     //std::cout<<"format="<<format<<" type="<<type<<" nitems="<<len<<std::endl;
     std::string value;
     if(type==XA_STRING){
       value = std::string((const char *)data);
     }else if(type==XA_ATOM && len>0){
       char *atom_name = XGetAtomName(display, *(Atom *)data);
       value = atom_name;
       XFree(atom_name);
     }
     XFree(data);

     // _NET_WM_WINDOW_TYPE_NORMAL
     size_t prefix_len = strlen("_NET_WM_WINDOW_TYPE_");
     if(value.size()>prefix_len) return value.substr(prefix_len);
     return value;
}

  void moveWindow(Window window, const char *geometry){
      int x_return, y_return; 
      unsigned int width_return, height_return;
      int bitmask = XParseGeometry(geometry, &x_return, &y_return, &width_return, &height_return);
      debug()<<"plan to move window as "<<geometry<<std::endl;
      if(bitmask & (XValue | YValue)){
        XRectangle rect = getWindowRectangle(window);
        debug()<<"plan to move window 2 "<<rect.x<<' '<<rect.y<<' '<<x_return<<' '<<y_return<<std::endl;
        if(!(rect.x==x_return && rect.y==y_return)){
          debug()<<"move window to "<<geometry<<std::endl;
          XMoveWindow(display, window, x_return, y_return);
        }
      }
  }
};

#endif
