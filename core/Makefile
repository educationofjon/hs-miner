
PROJ=cBlakeMiner_multiPlatform
OUTFILENAME=cBlakeMiner_multiPlatform
CC=gcc

CFLAGS=-std=c99 -Wall -DUNIX -g -DDEBUG

# Check for 32-bit vs 64-bit
PROC_TYPE = $(strip $(shell uname -m | grep 64))
 
# Check for Mac OS
IS_WIN = $(OS)
OS = $(shell uname -s 2>/dev/null | tr [:lower:] [:upper:])
DARWIN = $(strip $(findstring DARWIN, $(OS)))

PWD = $(shell cygpath -am .)
$(info $$PWD is [${PWD}])
$(info $$PROC_TYPE IS [${PROC_TYPE}])
# MacOS System
ifneq ($(DARWIN),)
	CFLAGS += -DMAC
	LIBS=-framework OpenCL

	ifeq ($(PROC_TYPE),)
		CFLAGS+=-arch i386
	else
		CFLAGS+=-arch x86_64
	endif
else
# Linux OS
ifneq (,$(findstring _NT,$(OS)))
    # Found
    $(info $$OS WINDOWS)
    OUTFILENAME=cBlakeMiner_multiPlatform
else
    # Not found
	OUTFILENAME=cBlakeMiner_multiPlatform_Linux    
endif
LIBS=-lOpenCL
ifeq ($(PROC_TYPE),)
	CFLAGS+=-m32
else
	CFLAGS+=-m64
endif

# Check for Linux-AMD
ifdef AMDAPPSDKROOT
	INC_DIRS=$(PWD)/AMDAPP_3/include
	LIB_DIRS=$(PWD)/AMDAPP_3/lib/x86_64
    #PWD = $(shell cygpath -am $(AMDAPPSDKROOT))
    #INC_DIRS=. $(PWD)/include

	#ifeq ($(PROC_TYPE),)
	#	LIB_DIRS=$(PWD)/lib/x86
	#else
	#	LIB_DIRS=$(PWD)/lib/x86_64
	#endif
else

# Check for Linux-Nvidia
#ifdef CUDA_PATH
#   INC_DIRS= "./CLLib"
#   LIB_DIRS= "./CLLib"
#endif

INC_DIRS=$(PWD)/AMDAPP_3/include
LIB_DIRS=$(PWD)/AMDAPP_3/lib/x86_64

endif
endif

$(PROJ): $(PROJ).c
	$(CC) $(CFLAGS) -o $(OUTFILENAME) $^ $(INC_DIRS:%=-I%) $(LIB_DIRS:%=-L%) $(LIBS) -Wall -Wno-format

.PHONY: clean

clean:
	rm $(PROJ)
