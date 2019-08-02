# Makefile for WordClus
# Last change: 01aug2019

PACKAGE = wordclus

#########################################################################
# define the locations of all the files

# the top of the source tree
TOP = .

# where to find FramepaC-ng include files
FP = ./framepac/framepac
FPT = ./framepac/template

# where to put the compiled library and required header files
INSTDIR = $(TOP)/include
LIBINSTDIR = $(TOP)/lib

# where to put the compiled main program
BINDIR = $(TOP)/bin

# where to look for header files from other modules when compiling
INCLUDEDIRS = -I./framepac -I$(TOP)/include

# the header files needed by applications using this library
HEADERS = wordclus.h

# the object modules to be included in the library file
OBJS = build/wctrmvec$(OBJ) \
	build/wcclust$(OBJ) \
	build/wcbatch$(OBJ) \
	build/wcdelim$(OBJ) \
	build/wcoutput$(OBJ) \
	build/wcmain$(OBJ) \
	build/gencorpus$(OBJ) \
	build/wcidhash$(OBJ) \
	build/wcglobal$(OBJ) \
	build/wcpairmap$(OBJ) \
	build/wcparam$(OBJ)

# the library archive file for this module
LIBRARY = $(PACKAGE)$(LIB)

# the executables to generate
EXES = bin/wordclus

# files to be included in the source distribution archive
DISTFILES = COPYING LICENSE README makefile makefile.?nx *.h *$(C)

#########################################################################
# compilation options

## uncommment the following line to enable corpora up to 1 trillion tokens instead of 4 billion
#CFLAGS += -DHUGE_CORPUS

# select the default debugging level.  This can be overridden by commandline
#   0=fully optimized for distribution, 1=include debug info, 2=disable optimizations
BUILD_DBG?=0

# select default bitness (32/64).  This can be overridden by commandline
#BITS?=32
BITS?=64

# include multi-threading support?  This is the default, can be overridden by commandline
THREADS?=1
#THREADS?=0

# enable execution profiling?
#PROFILE?=-pg

# enable a release build with all debugging support turned off?
#NODEBUG?= -DNDEBUG

ifndef GDB
#GDB = -ggdb3
endif

# enable one of the various sanitizers?  This is the default, and can be overridden by commandline
#  options: 0=disabled, 1=threads, 2=address, 3=leak, 4=memory, 5=undefined-behavior
SANE?=0

#########################################################################
# define the compiler and its options

CC = g++ --std=c++11
CCLINK=$(CC)

LINKBITS=-m$(BITS)
CPUDEF=-march=native

# compiler warnings to enable
WARN=-Wall -Wextra
WARN+= -Wno-deprecated -Wshadow -Wcast-align -Wmissing-noreturn -Wmissing-format-attribute
WARN+= -Wunused-result -Wno-multichar -Wpacked
#WARN+=-Wdisabled-optimization -Wpadded
#WARN+=-pedantic

ifeq ($(THREADS),1)
  PTHREAD=-pthread
else
  PTHREAD=-DFrSINGLE_THREADED
endif

ifeq ($(SANE),1)
SANITIZE=-fsanitize=thread -fPIC
LINKSAN=-fPIC -pie
else ifeq ($(SANE),2)
SANITIZE=-fsanitize=address -fno-omit-frame-pointer
else ifeq ($(SANE),3)
SANITIZE=-fsanitize=leak -fno-omit-frame-pointer
else ifeq ($(SANE),4)
SANITIZE=-fsanitize=memory -fno-omit-frame-pointer
else ifeq ($(SANE),5)
SANITIZE=-fsanitize=undefined -DSANITIZING
endif

# now build the complete set of compiler flags
CFLAGS= $(WARN) $(CPUDEF) $(PTHREAD) $(INCLUDEDIRS)
CFLAGS +=$(PROFILE)
CFLAGS +=$(NODEBUG)
CFLAGS +=$(LINKBITS)
CFLAGS +=$(EXTRAINC)
CFLAGS +=$(SANITIZE)
CFLAGS +=$(SHAREDLIB)
CFLAGS +=$(COMPILE_OPTS)

CFLAGEXE = -L$(LIBINSTDIR) $(PROFILE)

ifeq ($(BUILD_DBG),2)
  CFLAGS += -ggdb3 -O0 -fno-inline -g3
else
ifeq ($(BUILD_DBG),1)
  CFLAGS += -ggdb3 -Og -g3
else
  CFLAGS += -O3 -fexpensive-optimizations -g$(DBGLVL) $(GDB)
endif
endif

# and finally, build the complete set of linker flags
LINKFLAGS = $(LINKBITS) $(PTHREAD)
LINKFLAGS+= $(LINKTYPE)
LINKFLAGS+= $(SANITIZE)
LINKFLAGS+= $(LINKSAN)

#########################################################################
# define the various extensions in use

OBJ = .o
EXE = 
LIB = .a
C = .C

#########################################################################
# define the object module librarian and its options

LIBRARIAN = ar
LIBFLAGS = rucl
LIBOBJS = $(OBJS)

#########################################################################
# define the library indexer and its options

LIBINDEXER = ranlib
LIBIDXFLAGS = $(LIBRARY)

#########################################################################
# define file copy/deletion/etc. programs

RM = rm -f
CP = cp -p
ZIP = zip
ZIPFLAGS = -qo9
BITBUCKET = >&/dev/null
TOUCH = touch

#########################################################################
## standard targets

all:	framepac $(EXES)

help:
	@echo "The makefile for $(PACKAGE) understands the following targets:"
	@echo "  all       perform a complete build of library and test program"
	@echo "  lib       build the library only"
	@echo "  install   build and install the library"
	@echo "  clean     erase intermediate files from the build"
	@echo "  veryclean erase all intermediate and backup files"
	@echo "  bootstrap install files needed by packages on which we depend"
	@echo "  tags      rebuild TAGS database"
	@echo "  tar       pack up all distribution files in a tar archive"
	@echo "  compress  pack up all distribution files in a compressed tar file"
	@echo "  zip       pack up all distribution files in a PKzip archive"
	@echo "  purify	   build a Purified executable (requires Purify)"
	@echo "  help      you're looking at it..."

lib:	$(LIBRARY)

clean:
	-$(RM) build/*$(OBJ) $(EXES) $(LIBRARY) ; true

veryclean: clean
	-$(RM) *.BAK *.CKP *~ "#*#"
	(cd framepac ; $(MAKE) clean)

install: $(LIBINSTDIR)/$(LIBRARY)

bootstrap:
	@mkdir -p build

tags:
	etags --c++ *.h *$(C) \
		$(FP)/*.h $(TOP)/FramepaC-ng/src/*$(C) $(TOP)/FramepaC-ng/template/*.cc

tar:
	$(RM) $(PACKAGE).tar
	tar chf $(PACKAGE).tar $(DISTFILES)

compress:	tar
	compress $(PACKAGE).tar

xz:	tar
	xz -8 $(PACKAGE).tar

zip:
	$(RM) $(PACKAGE).zip
	$(ZIP) $(ZIPFLAGS) $(PACKAGE) $(DISTFILES)

#########################################################################
## executables

bin/wordclus$(EXE): build/wordclus$(OBJ) $(LIBRARY)
	$(CCLINK) $(LINKFLAGS) $(CFLAGEXE) -o $@ $^ -lframepacng

$(LIBINSTDIR)/$(LIBRARY): $(LIBRARY)
	$(CP) $(HEADERS) $(INSTDIR)
	$(CP) $< $@

#########################################################################

# the dependencies for each module of the full package
build/gencorpus$(OBJ):	gencorpus$(C) wordclus.h wcbatch.h wcparam.h $(FP)/threadpool.h \
			$(FP)/progress.h $(FP)/string.h $(FP)/symboltable.h $(FP)/texttransforms.h $(FP)/words.h 
build/wcbatch$(OBJ):		wcbatch$(C) wordclus.h wcbatch.h wcparam.h \
			$(FP)/texttransforms.h $(FP)/threadpool.h
build/wcclust$(OBJ):		wcclust$(C) wordclus.h wctrmvec.h wcparam.h \
			$(FP)/message.h $(FP)/symboltable.h
build/wcdelim$(OBJ):		wcdelim$(C) wordclus.h
build/wcglobal$(OBJ):	wcglobal$(C) wordclus.h
build/wcidhash$(OBJ):	wcidhash$(C) wordclus.h $(FPT)/hashtable.cc
build/wcmain$(OBJ):		wcmain$(C) wordclus.h wcbatch.h wcpair.h wctrmvec.h wcparam.h
build/wcoutput$(OBJ):	wcoutput$(C) wordclus.h wctrmvec.h
build/wcpairmap$(OBJ):	wcpairmap$(C) wcpair.h
build/wcparam$(OBJ):		wcparam$(C) wcparam.h wordclus.h $(FP)/cluster.h $(FP)/stringbuilder.h \
			$(FP)/texttransforms.h
build/wctrmvec$(OBJ):	wctrmvec$(C) wordclus.h wctrmvec.h $(FP)/memory.h $(FP)/symboltable.h

build/wordclus$(OBJ):	wordclus$(C) wordclus.h wcparam.h \
			$(FP)/argparser.h $(FP)/symboltable.h $(FP)/memory.h $(FP)/timer.h \
			$(FP)/stringbuilder.h

wcbatch.h:		$(FP)/file.h
	$(TOUCH) $@

wcparam.h:		$(FP)/contextcoll.h $(FP)/cstring.h $(FP)/hashtable.h
	$(TOUCH) $@

wctrmvec.h:		$(FP)/list.h $(FP)/vecsim.h wcparam.h
	$(TOUCH) $@

wordclus.h:		$(FP)/cluster.h $(FP)/file.h $(FP)/hashtable.h $(FP)/list.h \
			$(FP)/threshold.h $(FP)/wordcorpus.h
	$(TOUCH) $@

#########################################################################
## library

$(LIBRARY): $(OBJS)
	-$(RM) $(LIBRARY)
	$(LIBRARIAN) $(LIBFLAGS) $(LIBRARY) $(LIBOBJS)
	$(LIBINDEXER) $(LIBIDXFLAGS)

framepac/framepacng.a:
	(cd framepac ; make lib)

#########################################################################
## submodule initialization

framepac:
	@[[ -e ../framepac-ng/framepac ]] && echo "Linking to local install of FramepaC-ng" && ln -s ../framepac-ng framepac ; true
	@[[ -e ../framepac/framepac ]] && echo "Linking to local install of FramepaC-ng" && ln -s ../framepac framepac ; true
	@[[ ! -e framepac ]] && [[ -e .git ]] && echo "Fetching FramepaC-ng" && git submodule add ../framepac-ng.git framepac && git submodule update --init
	@[[ -e framepac ]] || (echo "Please install FramepaC-ng in subdir 'framepac'" ;exit 1)

#########################################################################
## default compilation rule

.C.o: ; $(CC) $(CFLAGS) $(CPUTYPE) -c $<

build/%$(OBJ) : %$(C)
	@mkdir -p build
	$(CC) $(CFLAGS) $(CPUTYPE) -c -o $@ $<

.SUFFIXES: $(OBJ) .C $(C) .cpp

# End of Makefile

