#-----------------------------------------------------------------------
# File    : matrix.mak
# Contents: build vector and matrix management modules
# Author  : Christian Borgelt
# History : 2003.01.27 file created
#           2006.07.20 adapted to Visual Studio 8
#           2016.04.20 completed dependencies on header files
#-----------------------------------------------------------------------
THISDIR  = ..\..\matrix\src
UTILDIR  = ..\..\util\src

CC       = cl.exe
DEFS     = /D WIN32 /D NDEBUG /D _CONSOLE /D _CRT_SECURE_NO_WARNINGS
CFLAGS   = /nologo /W3 /O2 /GS- $(DEFS) /c
INCS     = /I $(UTILDIR)

LD       = link.exe
LDFLAGS  = /nologo /subsystem:console /incremental:no
LIBS     = 

HDRS     = $(UTILDIR)\escape.h   $(UTILDIR)\tabread.h \
           $(UTILDIR)\error.h    matrix.h     
OBJ1_O   = $(UTILDIR)\escape.obj $(UTILDIR)\tabread.obj mat_rdwr.obj 
OBJ2_O   = $(OBJ1_O) matrix2.obj
OBJ3_O   = $(OBJ2_O) matrix3.obj
OBJ4_O   = $(OBJ1_O) matrix4.obj
PRGS     = transp.exe invert.exe solve.exe eigen.exe match.exe

#-----------------------------------------------------------------------
# Build Programs
#-----------------------------------------------------------------------
all:          $(PRGS)

transp.exe:   $(OBJ1_O) transp.obj matrix.mak
	$(LD) $(LDFLAGS) $(OBJ1_O) transp.obj $(LIBS) /out:$@

invert.exe:   $(OBJ2_O) invert.obj matrix.mak
	$(LD) $(LDFLAGS) $(OBJ2_O) invert.obj $(LIBS) /out:$@

solve.exe:    $(OBJ2_O) solve.obj matrix.mak
	$(LD) $(LDFLAGS) $(OBJ2_O) solve.obj  $(LIBS) /out:$@

eigen.exe:    $(OBJ3_O) eigen.obj matrix.mak
	$(LD) $(LDFLAGS) $(OBJ3_O) eigen.obj  $(LIBS) /out:$@

match.exe:    $(OBJ4_O) match.obj matrix.mak
	$(LD) $(LDFLAGS) $(OBJ4_O) match.obj  $(LIBS) /out:$@

#-----------------------------------------------------------------------
# Main Programs
#-----------------------------------------------------------------------
transp.obj:   matrix.h transp.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) transp.c /Fo$@

invert.obj:   matrix.h invert.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) invert.c /Fo$@

solve.obj:    matrix.h solve.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) solve.c /Fo$@

eigen.obj:    matrix.h eigen.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) eigen.c /Fo$@

match.obj:    matrix.h match.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) match.c /Fo$@

#-----------------------------------------------------------------------
# Vector and Matrix Management
#-----------------------------------------------------------------------
matrix1.obj:  matrix.h matrix1.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) matrix1.c /Fo$@

mat_rdwr.obj: $(UTILDIR)\tabread.h
mat_rdwr.obj: matrix.h matrix1.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) /D MAT_RDWR matrix1.c /Fo$@

matrix2.obj:  matrix.h matrix2.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) matrix2.c /Fo$@

matrix3.obj:  matrix.h matrix3.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) matrix3.c /Fo$@

matrix4.obj:  matrix.h matrix4.c matrix.mak
	$(CC) $(CFLAGS) $(INCS) matrix4.c /Fo$@

#-----------------------------------------------------------------------
# External Modules
#-----------------------------------------------------------------------
$(UTILDIR)\escape.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak escape.obj
	cd $(THISDIR)
$(UTILDIR)\tabread.obj:
	cd $(UTILDIR)
	$(MAKE) /f util.mak tabread.obj
	cd $(THISDIR)

#-----------------------------------------------------------------------
# Install
#-----------------------------------------------------------------------
install:
	-@copy *.exe ..\..\..\bin

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
localclean:
	-@erase /Q *~ *.obj *.idb *.pch $(PRGS)

clean:
	$(MAKE) /f matrix.mak localclean
	cd $(UTILDIR)
	$(MAKE) /f util.mak clean
	cd $(THISDIR)
