# Makefile for Pangloss EBMT module
#   (using GCC under Unix/Linux or Borland/Watcom C++ under MS-DOS/Win32)
# Last change: 01nov15

#########################################################################
# define the locations of all the files

# the top of the source tree
TOP = ..

# where to put the compiled library and required header files
INSTDIR = $(TOP)/include
LIBINSTDIR = $(TOP)/lib

# where to put the compiled main program
BINDIR = $(TOP)/bin

# where to look for header files from other modules when compiling
INCLUDEDIRS = -I$(TOP)/include

#########################################################################
# define the compiler and its options
#
# uncomment the appropriate line for your compiler/OS below:
# (also check the 'include' file for further options)

#include makefile.unx		# GNU C under Unix
include makefile.lnx		# GNU C under Linux (default: x86 version)
#!include makefile.bcc		# Borland C++ under MS-DOS
#!include makefile.wcc		# Watcom C++ under MS-DOS, for DOS/4GW extender
#!include makefile.nt		# Watcom C++, building Windows NT executable
#!include makefile.vc		# Visual C++ under Windows (NT/95)

#########################################################################
#  define the various files to be used

# the name of the package being built by this makefile
PACKAGE = wordclus

# the object modules to be included in the library file
OBJS = wcwrdinf$(OBJ) wctrmvec$(OBJ) wcclust$(OBJ) wcanalyz$(OBJ) \
	wcconfig$(OBJ) wcgram$(OBJ) wcbatch$(OBJ) \
	wcoutput$(OBJ) wcutil$(OBJ) wcmain$(OBJ) \
	wctoken$(OBJ) wcglobal$(OBJ)

# the header files needed by applications using this library
HEADERS = wordclus.h

# files to be included in the source distribution archive
DISTFILES = COPYING GPL.txt makefile makefile.?nx makefile.?cc makefile.?? *.h *$(C) \
	objs.lst vcobjs.lst rblib.bat

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
	$(RM) *$(OBJ)
	$(RM) $(LIBRARY)
	$(RM) $(TESTPROG)$(EXE)

veryclean: clean
	$(RM) *.BAK
	$(RM) *.CKP *~
	$(RM) "#*#"

install: $(LIBINSTDIR)/$(LIBRARY)

system: $(BINDIR)/$(TESTPROG)$(EXE)

bootstrap:

tags:
	etags --c++ *.h *$(C) $(TOP)/ebmt/src/*.h $(TOP)/ebmt/src/*$(C) \
		$(TOP)/framepac/src/*.h $(TOP)/framepac/src/*$(C)

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
wcanalyz$(OBJ):		wcanalyz$(C) wordclus.h wctoken.h
wcbatch$(OBJ):		wcbatch$(C) wordclus.h wcbatch.h wcglobal.h
wcclust$(OBJ):		wcclust$(C) wordclus.h
wcconfig$(OBJ):		wcconfig$(C) wordclus.h wcconfig.h wcglobal.h
wcglobal$(OBJ):		wcglobal$(C) wordclus.h wcglobal.h wcconfig.h
wcgram$(OBJ):		wcgram$(C) wordclus.h wctoken.h wcglobal.h
wcmain$(OBJ):		wcmain$(C) wordclus.h wcbatch.h wcglobal.h
wcoutput$(OBJ):		wcoutput$(C) wordclus.h
wctoken$(OBJ):		wctoken$(C) wctoken.h wordclus.h wcglobal.h
wctrmvec$(OBJ):		wctrmvec$(C) wordclus.h wcglobal.h
wcutil$(OBJ):		wcutil$(C) wordclus.h
wcwrdinf$(OBJ):		wcwrdinf$(C) wordclus.h

$(TESTPROG)$(OBJ):	$(TESTPROG)$(C) wordclus.h wcconfig.h wcglobal.h

# End of Makefile

