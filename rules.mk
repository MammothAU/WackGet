
CFLAGS=$(DEFS) $(CFLAGS)
RFLAGS=$(DEFS) $(RFLAGS)

OBJECTS=$(RCSOURCES:.rc=.res) $(CSOURCES:.c=.obj)

!IF "$(PCH)" != ""
CFLAGS=$(CFLAGS) /Yc$(PCH)
!ENDIF

all: $(OUT_DIR) $(SUBDIRS) $(TARGETS) $(DLL_TARGETS)

$(OUT_DIR):
	mkdir $@
!IF "$(SUBDIRS)" != ""
$(SUBDIRS): FORCE
	cd $@ && $(MAKE) /E /fNMakefile && cd ..
FORCE:
!ENDIF

!IF "$(TARGETS)" != ""
$(TARGETS): $(OBJECTS)
	$(LINK) $(LDFLAGS) /out:"$@" $** $(LIBS)
!ENDIF
!IF "$(DLL_TARGETS)" != ""
$(DLL_TARGETS): $(OBJECTS)
	$(LINK) /dll $(LDFLAGS) /out:"$@" /implib:"$*.lib" $** $(LIBS)
!ENDIF

clean:
!IF "$(SUBDIRS)" != ""
	for %1 in ($(SUBDIRS)) do if exist %1/NMakefile cd %1 && $(MAKE) /E /fNMakefile $@ && cd ..
!ENDIF
	if exist *.obj del *.obj
	if exist *.pch del *.pch
	if exist *.pdb del *.pdb
	if exist *.res del *.res
	if exist *.aps del *.aps
!IF "$(TARGETS)" != ""
	if exist $(TARGETS:.exe=.*) del $(TARGETS:.exe=.*)
!ENDIF
!IF "$(DLL_TARGETS)" != ""
	if exist $(DLL_TARGETS:.dll=.*) del $(DLL_TARGETS:.dll=.*)
!ENDIF
	if exist WackGet$(VERSION).exe del WackGet$(VERSION).exe
	if exist WackGet$(VERSION).zip del WackGet$(VERSION).zip

install: WackGet$(VERSION).exe WackGet$(VERSION).zip

WackGet$(VERSION).exe: all WackGet.nsi
	makensis /DAPP_VERSION=$(VERSION) WackGet.nsi

WackGet$(VERSION).zip: all WackGet.lst
	rar a -ep -apWackGet WackGet$(VERSION).zip @WackGet.lst
