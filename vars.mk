
VERSION=1.2.4
DEBUG=0
CC=cl
LINK=link
DEFS=/DWIN32 /D_WINDOWS /DOEMRESOURCE /DDEBUG=$(DEBUG) /DVERSION=\"$(VERSION)\"
CFLAGS=/nologo /W3 -I "C:\Program Files\Microsoft Platform SDK\Include"
LDFLAGS=/nologo /machine:ix86 /libpath:$(OUT_DIR)

!IF "$(DEBUG)" == "1"

OUT_DIR=$(TOP)\Debug
DEFS=$(DEFS) /D_DEBUG
CFLAGS=$(CFLAGS) /Od /Zi /MDd /GZ
LDFLAGS=$(LDFLAGS) /debug /debugtype:cv /pdbtype:sept

!ELSE

OUT_DIR=$(TOP)\Release
DEFS=$(DEFS) /DNDEBUG
CFLAGS=$(CFLAGS) /O2 /MD
LDFLAGS=$(LDFLAGS) /incremental:no

!ENDIF

LD_FLAGS=$(LD_FLAGS) /libpath:$(OUT_DIR)
