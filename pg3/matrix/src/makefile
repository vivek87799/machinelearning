#-----------------------------------------------------------------------
# File    : makefile    (directory: matrix)
# Contents: build vector and matrix management modules
# Author  : Christian Borgelt
# History : 1999.05.19 file created
#           2001.04.29 module split into 3 parts
#           2001.10.17 programs transp, invert, solve added
#           2004.02.12 program eigen added (eigenvalues/-vectors)
#           2008.09.20 matrix4.c added (special purpose functions)
#           2013.08.09 modified CFBASE to higher warning level
#           2016.04.20 creation of dependency files added
#-----------------------------------------------------------------------
SHELL    = /bin/bash
THISDIR  = ../../matrix/src
UTILDIR  = ../../util/src

CC       = gcc -std=c99
# CC       = g++
CFBASE  = -Wall -Wextra -Wno-unused-parameter -Wconversion \
          -pedantic -c $(ADDFLAGS)
CFLAGS   = $(CFBASE) -DNDEBUG -O3 -funroll-loops
# CFLAGS   = $(CFBASE) -g
# CFLAGS   = $(CFBASE) -g -DSTORAGE
INC      = -I$(UTILDIR)

LD       = gcc
LDFLAGS  = $(ADDFLAGS)
LIBS     = -lm $(ADDLIBS)

# ADDOBJS  = $(UTILDIR)/storage.o

HDRS     = $(UTILDIR)/tabread.h $(UTILDIR)/escape.h \
           $(UTILDIR)/error.h   matrix.h
OBJ1_O   = mat_rdwr.o $(UTILDIR)/tabread.o $(UTILDIR)/escape.o
OBJ2_O   = $(OBJ1_O) matrix2.o $(ADDOBJS)
OBJ3_O   = $(OBJ2_O) matrix3.o $(ADDOBJS)
OBJ4_O   = $(OBJ1_O) matrix4.o $(ADDOBJS)
PRGS     = transp invert solve eigen match

#-----------------------------------------------------------------------
# Build Programs
#-----------------------------------------------------------------------
all:          $(PRGS)

transp:       $(OBJ1_O) transp.o makefile
	$(LD) $(LDFLAGS) $(OBJ1_O) transp.o $(LIBS) -o $@

invert:       $(OBJ2_O) invert.o makefile
	$(LD) $(LDFLAGS) $(OBJ2_O) invert.o $(LIBS) -o $@

solve:        $(OBJ2_O) solve.o makefile
	$(LD) $(LDFLAGS) $(OBJ2_O) solve.o  $(LIBS) -o $@

eigen:        $(OBJ3_O) eigen.o makefile
	$(LD) $(LDFLAGS) $(OBJ3_O) eigen.o  $(LIBS) -o $@

match:        $(OBJ4_O) match.o makefile
	$(LD) $(LDFLAGS) $(OBJ4_O) match.o  $(LIBS) -o $@

#-----------------------------------------------------------------------
# Main Programs
#-----------------------------------------------------------------------
transp.o:     $(HDRS)
transp.o:     transp.c makefile
	$(CC) $(CFLAGS) $(INC) transp.c -o $@

transp.d:     transp.c
	$(CC) -MM $(CFLAGS) $(INC) transp.c > transp.d

invert.o:     $(HDRS)
invert.o:     invert.c makefile
	$(CC) $(CFLAGS) $(INC) invert.c -o $@

invert.d:     invert.c
	$(CC) -MM $(CFLAGS) $(INC) invert.c > invert.d

solve.o:      $(HDRS)
solve.o:      solve.c makefile
	$(CC) $(CFLAGS) $(INC) solve.c -o $@

solve.d:      solve.c
	$(CC) -MM $(CFLAGS) $(INC) solve.c > solve.d

eigen.o:      $(HDRS)
eigen.o:      eigen.c makefile
	$(CC) $(CFLAGS) $(INC) eigen.c -o $@

eigen.d:      eigen.c
	$(CC) -MM $(CFLAGS) $(INC) eigen.c > eigen.d

match.o:      $(HDRS)
match.o:      match.c makefile
	$(CC) $(CFLAGS) $(INC) match.c -o $@

match.d:      match.c
	$(CC) -MM $(CFLAGS) $(INC) match.c > match.d

#-----------------------------------------------------------------------
# Vector and Matrix Management
#-----------------------------------------------------------------------
matrix1.o:    matrix.h matrix1.c makefile
	$(CC) $(CFLAGS) $(INC) matrix1.c -o $@

matrix1.d:    matrix1.c
	$(CC) -MM $(CFLAGS) $(INC) matrix1.c > matrix1.d

mat_rdwr.o:   $(UTILDIR)/tabread.h
mat_rdwr.o:   matrix.h matrix1.c makefile
	$(CC) $(CFLAGS) $(INC) -DMAT_RDWR matrix1.c -o $@

mat_rdwr.d:   matrix1.c
	$(CC) -MM $(CFLAGS) $(INC) -DMAT_RDWR matrix1.c > mat_rdwr.d

matrix2.o:    matrix.h matrix2.c makefile
	$(CC) $(CFLAGS) $(INC) matrix2.c -o $@

matrix2.d:    matrix2.c
	$(CC) -MM $(CFLAGS) $(INC) matrix2.c > matrix2.d

matrix3.o:    matrix.h matrix3.c makefile
	$(CC) $(CFLAGS) $(INC) matrix3.c -o $@

matrix3.d:    matrix3.c
	$(CC) -MM $(CFLAGS) $(INC) matrix3.c > matrix3.d

matrix4.o:    matrix.h matrix4.c makefile
	$(CC) $(CFLAGS) $(INC) matrix4.c -o $@

matrix4.d:    matrix4.c
	$(CC) -MM $(CFLAGS) $(INC) matrix4.c > matrix4.d

#-----------------------------------------------------------------------
# External Modules
#-----------------------------------------------------------------------
$(UTILDIR)/escape.o:
	cd $(UTILDIR); $(MAKE) escape.o  ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/tabread.o:
	cd $(UTILDIR); $(MAKE) tabread.o ADDFLAGS="$(ADDFLAGS)"

#-----------------------------------------------------------------------
# Source Distribution Packages
#-----------------------------------------------------------------------
dist:
	$(MAKE) clean
	cd ../..; rm -f matrix.zip matrix.tar.gz; \
        zip -rq matrix.zip    matrix/{src,ex,doc} \
            util/src/{escape.[ch],tabread.[ch],error.h} \
            util/src/{makefile,util.mak} util/doc; \
        tar cfz matrix.tar.gz matrix/{src,ex,doc} \
            util/src/{escape.[ch],tabread.[ch],error.h} \
            util/src/{makefile,util.mak} util/doc

#-----------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------
install:
	cp $(PRGS) $(HOME)/bin

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
localclean:
	rm -f *.d *.o *~ *.flc core $(PRGS)

clean:
	$(MAKE) localclean
	cd $(UTILDIR); $(MAKE) clean
