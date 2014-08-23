DLL  = true-tpl.dll
RES  = true-tpl.res
DEF  = true-tpl.def
OBJS = true-tpl.obj console.obj winmem.obj lstring.obj eicoll.obj tcoll.obj farintf.obj cregexp.obj clocale.obj process.obj
LIBS = kernel32.lib advapi32.lib user32.lib shell32.lib libcmt.lib oldnames.lib ole32.lib
INCLUDE = $(INCLUDE);.\Far-Hpp

CC_FLAGS = /c /Gy /G4 /O1 /W4 /Og
# /G4 - optimize for 486
# /O1 - minimize size
# /W4 - warning level

LN_FLAGS = /opt:nowin98 /DLL /MAP /NODEFAULTLIB

all: $(DLL)

.cpp.obj:
  cl $(CC_FLAGS) $<

.rc.res:
  rc $<

$(OBJS): makefile

$(DLL): $(OBJS) $(RES)
  link $(LN_FLAGS) /def:$(DEF) $(OBJS) $(LIBS) $(RES)

clean:
  del $(DLL)
  del $(RES)
  del $(OBJS)
  del *.exp
  del *.lib
  del *.map
