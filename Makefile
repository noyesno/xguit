
all: xrr  xguit

xguit: src/xguit.cpp
	g++ -o $@ $^ -lXtst -L/usr/X11R6/lib64

xrr: XRecord XReplay.so

XRecord: src/XRecord.cpp src/XRRHelper.hpp
	g++ -o $@ $< -lXtst -L/usr/X11R6/lib64

XReplay.so: src/XReplay.cpp
	g++ -shared -fPIC -o $@ -DUSE_TCL_STUBS $< -L/usr/X11R6/lib64 -lXtst -ltclstub8.4


xrecord2tcl:
	grep x:: XRecord.log | sed 's/x::/xreplay::/g' > xreplay.tcl

clean:
	rm -rf xguit XRecord XReplay.so
