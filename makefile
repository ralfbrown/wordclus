# Makefile for WordClus
#   (using GCC under Unix/Linux or Borland/Watcom C++ under MS-DOS/Win32)
# Last change: 21sep2018

#########################################################################
# define the locations of all the files

# the top of the source tree
TOP = .

# where to find FramepaC-ng include files
FP = ../FramepaC-ng/framepac
FPT = ../FramepaC-ng/template

# where to put the compiled library and required header files
INSTDIR = $(TOP)/include
LIBINSTDIR = $(TOP)/lib

# where to put the compiled main program
BINDIR = $(TOP)/bin

# where to look for header files from other modules when compiling
INCLUDEDIRS = -I$(TOP)/FramepaC-ng -I$(TOP)/include

#########################################################################
# define the compiler and its options
#
# uncomment the appropriate line for your compiler/OS below:
# (also check the 'include' file for further options)

#include makefile.unx		# GNU C under Unix
include makefile.lnx		# GNU C under Linux (default: x86 version)
#!include makefile.vc		# Visual C++ under Windows (NT/95)


## uncommment the following line to enable corpora up to 1 trillion tokens instead of 4 billion
#CFLAGS += -DHUGE_CORPUS

#########################################################################
#  define the various files to be used

# the name of the package being built by this makefile
PACKAGE = wordclus

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

# the header files needed by applications using this library
HEADERS = wordclus.h

# files to be included in the source distribution archive
DISTFILES = COPYING LICENSE makefile makefile.?nx makefile.?? *.h *$(C)
#	wordclus/*.h src/*$(C)

# the library archive file for this module
LIBRARY = $(PACKAGE)$(LIB)

# the executable to be built for testing the package
TESTPROG = wordclus

# the object modules needed to build the test program
TESTOBJS = $(TESTPROG)$(OBJ)

#########################################################################

all:	$(TESTPROG)$(EXE)

help:
	@echo "The makefile for $(PACKAGE) understands the following targets:"
	@echo "  all       perform a complete build of library and test program"
	@echo "  lib       build the library only"
	@echo "  install   build and install the library"
	@echo "  system    install the test program in the top-level directory"
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
	$(RM) build/*$(OBJ)
	$(RM) $(LIBRARY)
	$(RM) $(TESTPROG)$(EXE)

veryclean: clean
	$(RM) build/*$(OBJ)
	$(RM) *.BAK
	$(RM) *.CKP *~
	$(RM) "#*#"

install: $(LIBINSTDIR)/$(LIBRARY)

system: $(BINDIR)/$(TESTPROG)$(EXE)

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

zip:
	$(RM) $(PACKAGE).zip
	$(ZIP) $(ZIPFLAGS) $(PACKAGE) $(DISTFILES)

purify:	$(LIBRARY) $(TESTOBJS)
	purify $(CCLINK) $(LINKFLAGS) $(CFLAGEXE) $(TESTPROG)$(OBJ) \
		$(TESTOBJS) $(LIBRARY) $(EXTRALIBS) $(USELIBS)

$(TESTPROG)$(EXE): $(LIBRARY) $(TESTOBJS)
	$(CCLINK) $(LINKFLAGS) $(CFLAGEXE) $(TESTOBJS) $(LIBRARY) $(USELIBS)

$(LIBRARY): $(OBJS)
	-$(RM) $(LIBRARY)
	$(LIBRARIAN) $(LIBFLAGS) $(LIBRARY) $(LIBOBJS)
	$(LIBINDEXER) $(LIBIDXFLAGS)

$(LIBINSTDIR)/$(LIBRARY): $(LIBRARY)
	$(CP) $(HEADERS) $(INSTDIR)
	$(CP) $(LIBRARY) $(LIBINSTDIR)

$(BINDIR)/$(TESTPROG)$(EXE): $(TESTPROG)$(EXE)
	$(CP) $(TESTPROG)$(EXE) $(BINDIR)/$(TESTPROG)$(EXE)

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

$(TESTPROG)$(OBJ):	$(TESTPROG)$(C) wordclus.h wcparam.h \
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

# End of Makefile

