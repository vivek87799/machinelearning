#-----------------------------------------------------------------------
# File    : mlp.mak
# Contents: commands to build neural network programs
# Author  : Christian Borgelt
# History : 2003.01.27 file created
#           2006.07.20 adapted to Visual Studio 8
#           2007.03.16 special matrix versions removed
#           2016.04.20 completed dependencies on header files
#-----------------------------------------------------------------------
THISDIR  = ..\..\mlp\src
UTILDIR  = ..\..\util\src
MATDIR   = ..\..\matrix\src
TABLEDIR = ..\..\table\src

CC       = cl.exe
DEFS     = /D WIN32 /D NDEBUG /D _CONSOLE /D _CRT_SECURE_NO_WARNINGS
CFLAGS   = /nologo /W3 /O2 /GS- $(DEFS) /c
INCS     = /I $(UTILDIR) /I $(MATDIR) /I $(TABLEDIR)

LD       = link.exe
LDFLAGS  = /nologo /subsystem:console /incremental:no
LIBS     = 

HDRS_1   = $(UTILDIR)\nstats.h     $(UTILDIR)\scanner.h    \
           $(MATDIR)\matrix.h
HDRS_2   = $(HDRS_1)               $(UTILDIR)\fntypes.h    \
           $(TABLEDIR)\attset.h    $(TABLEDIR)\attmap.h    \
           $(TABLEDIR)\table.h
HDRS     = $(HDRS_2)               $(UTILDIR)\error.h      \
           $(UTILDIR)\tabread.h    $(UTILDIR)\tabwrite.h mlp.h
OBJS     = $(UTILDIR)\arrays.obj   $(UTILDIR)\escape.obj   \
           $(UTILDIR)\tabread.obj  $(UTILDIR)\tabwrite.obj \
           $(UTILDIR)\scanner.obj  $(UTILDIR)\nst_pars.obj \
           $(UTILDIR)\random.obj   $(MATDIR)\mat_rdwr.obj  \
           $(TABLEDIR)\attset1.obj $(TABLEDIR)\attset2.obj \
           $(TABLEDIR)\attset3.obj $(TABLEDIR)\attmap.obj  \
           mlp_ext.obj
MLPT_O   = $(OBJS)                 $(UTILDIR)\params.obj   \
           $(TABLEDIR)\table1.obj  $(TABLEDIR)\tab2ro.obj mlpt.obj
MLPX_O   = $(OBJS)\
           $(TABLEDIR)\table1.obj  $(TABLEDIR)\tab2ro.obj mlpx.obj
MLPS_O   = $(OBJS) mlps.obj

PRGS     = mlpt.exe mlpx.exe mlps.exe

#-----------------------------------------------------------------------
# Build Programs
#-----------------------------------------------------------------------
all:          $(PRGS)

mlpt.exe:     $(MLPT_O)  mlp.mak
	$(LD) $(LDFLAGS) $(MLPT_O) $(LIBS) /out:$@

mlpx.exe:     $(MLPX_O)  mlp.mak
	$(LD) $(LDFLAGS) $(MLPX_O) $(LIBS) /out:$@

mlps.exe:     $(MLPS_O)  mlp.mak
	$(LD) $(LDFLAGS) $(MLPS_O) $(LIBS) /out:$@

#-----------------------------------------------------------------------
# Main Programs
#-----------------------------------------------------------------------
mlpt.obj:     $(HDRS) $(UTILDIR)\random.h $(UTILDIR)\params.h
mlpt.obj:     mlpt.c mlp.mak
	$(CC) $(CFLAGS) $(INCS) mlpt.c /Fo$@

mlpx.obj:     $(HDRS)
mlpx.obj:     mlpx.c mlp.mak
	$(CC) $(CFLAGS) $(INCS) mlpx.c /Fo$@

mlps.obj:     $(HDRS)
mlps.obj:     mlps.c mlp.mak
	$(CC) $(CFLAGS) $(INCS) mlps.c /Fo$@

#-----------------------------------------------------------------------
# Multilayer Perceptron Management
#-----------------------------------------------------------------------
mlp.obj:      $(HDRS_1)
mlp.obj:      mlp.h mlp.c mlp.mak
	$(CC) $(CFLAGS) $(INCS) /D MLP_PARSE mlp.c /Fo$@

mlp_ext.obj:  $(HDRS_2)
mlp_ext.obj:  mlp.h mlp.c mlp.mak
	$(CC) $(CFLAGS) $(INCS) /D MLP_PARSE /D MLP_EXTFN mlp.c /Fo$@

#-----------------------------------------------------------------------
# External Modules
#-----------------------------------------------------------------------
$(UTILDIR)\arrays.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak arrays.obj
	cd $(THISDIR)
$(UTILDIR)\escape.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak escape.obj
	cd $(THISDIR)
$(UTILDIR)\tabread.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak tabread.obj
	cd $(THISDIR)
$(UTILDIR)\tabwrite.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak tabwrite.obj
	cd $(THISDIR)
$(UTILDIR)\scanner.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak scanner.obj
	cd $(THISDIR)
$(UTILDIR)\nst_pars.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak nst_pars.obj
	cd $(THISDIR)
$(UTILDIR)\random.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak random.obj
	cd $(THISDIR)
$(UTILDIR)\params.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak params.obj
	cd $(THISDIR)
$(MATDIR)\mat_rdwr.obj:
	cd $(MATDIR)
	$(MAKE) /f matrix.mak mat_rdwr.obj
	cd $(THISDIR)
$(MATDIR)\matrix2.obj:
	cd $(MATDIR)
	$(MAKE) /f matrix.mak matrix2.obj
	cd $(THISDIR)
$(TABLEDIR)\attset1.obj:
	cd $(TABLEDIR)
	$(MAKE) /f table.mak attset1.obj
	cd $(THISDIR)
$(TABLEDIR)\attset2.obj:
	cd $(TABLEDIR)
	$(MAKE) /f table.mak attset2.obj
	cd $(THISDIR)
$(TABLEDIR)\attset3.obj:
	cd $(TABLEDIR)
	$(MAKE) /f table.mak attset3.obj
	cd $(THISDIR)
$(TABLEDIR)\attmap.obj:
	cd $(TABLEDIR)
	$(MAKE) /f table.mak attmap.obj
	cd $(THISDIR)
$(TABLEDIR)\table1.obj:
	cd $(TABLEDIR)
	$(MAKE) /f table.mak table1.obj
	cd $(THISDIR)
$(TABLEDIR)\tab2ro.obj:
	cd $(TABLEDIR)
	$(MAKE) /f table.mak tab2ro.obj
	cd $(THISDIR)

#-----------------------------------------------------------------------
# Install
#-----------------------------------------------------------------------
install:
	-@copy *.exe ..\..\..\bin

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
clean:
	$(MAKE) /f mlp.mak localclean
	cd $(UTILDIR)
	$(MAKE) /f util.mak clean
	cd $(MATDIR)
	$(MAKE) /f matrix.mak localclean
	cd $(TABLEDIR)
	$(MAKE) /f table.mak localclean
	cd $(THISDIR)

localclean:
	-@erase /Q *~ *.obj *.idb *.pch $(PRGS)
