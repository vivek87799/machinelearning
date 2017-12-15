#-----------------------------------------------------------------------
# File    : makefile    (directory: mlp)
# Contents: commands to build multilayer perceptron programs
# Author  : Christian Borgelt
# History : 2001.02.28 file created
#           2001.05.13 programs mlpt and mlpx added
#           2002.01.22 adapted to changed table makefile
#           2003.03.10 programs renamed to achieve consistent scheme
#           2004.08.11 external module attmap added
#           2004.08.12 adapted to new modules parse and nstats
#           2007.03.16 special matrix versions removed
#           2008.08.11 adapted to name change from vecops to arrays
#           2013.08.09 modified CFBASE to higher warning level
#           2016.04.20 creation of dependency files added
#-----------------------------------------------------------------------
SHELL    = /bin/bash
THISDIR  = ../../mlp/src
UTILDIR  = ../../util/src
MATDIR   = ../../matrix/src
TABLEDIR = ../../table/src

CC       = gcc -std=c99
CFBASE   = -Wall -Wextra -Wno-unused-parameter -Wconversion \
           -pedantic -c $(ADDFLAGS)
CFLAGS   = $(CFBASE) -DNDEBUG -O3 -funroll-loops
# CFLAGS   = $(CFBASE) -DNDEBUG -O3 -funroll-loops -DMLP_TABFN
# CFLAGS   = $(CFBASE) -DNDEBUG -O3 -funroll-loops -DMLP_TANH
# CFLAGS   = $(CFBASE) -DNDEBUG -O3 -funroll-loops -DMLP_TANH -DMLP_TABFN
# CFLAGS   = $(CFBASE) -g
# CFLAGS   = $(CFBASE) -g -DSTORAGE
INCS     = -I$(UTILDIR) -I$(MATDIR) -I$(TABLEDIR)

LD       = gcc
LDFLAGS  = $(ADDFLAGS)
LIBS     = -lm $(ADDLIBS)

# ADDOBJS  = $(UTILDIR)/storage.o

HDRS_1   = $(UTILDIR)/nstats.h   $(UTILDIR)/scanner.h  \
           $(MATDIR)/matrix.h
HDRS_2   = $(HDRS_1)             $(UTILDIR)/fntypes.h  \
           $(TABLEDIR)/attset.h  $(TABLEDIR)/attmap.h  \
           $(TABLEDIR)/table.h
HDRS     = $(HDRS_2)             $(UTILDIR)/error.h    \
           $(UTILDIR)/tabread.h  $(UTILDIR)/tabwrite.h mlp.h
OBJS     = $(UTILDIR)/arrays.o   $(UTILDIR)/escape.o   \
           $(UTILDIR)/tabread.o  $(UTILDIR)/tabwrite.o \
           $(UTILDIR)/scanner.o  $(UTILDIR)/nst_pars.o \
           $(UTILDIR)/random.o   $(MATDIR)/mat_rdwr.o  \
           $(TABLEDIR)/attset1.o $(TABLEDIR)/attset2.o \
           $(TABLEDIR)/attset3.o $(TABLEDIR)/attmap.o  \
           mlp_ext.o $(ADDOBJS)
MLPT_O   = $(OBJS)               $(UTILDIR)/params.o   \
           $(TABLEDIR)/table1.o  $(TABLEDIR)/tab2ro.o mlpt.o
MLPX_O   = $(OBJS) \
           $(TABLEDIR)/table1.o  $(TABLEDIR)/tab2ro.o mlpx.o
MLPS_O   = $(OBJS) mlps.o

PRGS     = mlpt mlpx mlps

#-----------------------------------------------------------------------
# Build Programs
#-----------------------------------------------------------------------
all:          $(PRGS)

mlpt:         $(MLPT_O)  makefile
	$(LD) $(LDFLAGS) $(MLPT_O) $(LIBS) -o $@

mlpx:         $(MLPX_O)  makefile
	$(LD) $(LDFLAGS) $(MLPX_O) $(LIBS) -o $@

mlps:         $(MLPS_O)  makefile
	$(LD) $(LDFLAGS) $(MLPS_O) $(LIBS) -o $@

#-----------------------------------------------------------------------
# Main Programs
#-----------------------------------------------------------------------
mlpt.o:       $(HDRS) $(UTILDIR)/random.h $(UTILDIR)/params.h
mlpt.o:       mlpt.c makefile
	$(CC) $(CFLAGS) $(INCS) mlpt.c -o $@

mlpt.d:       mlpt.c makefile
	$(CC) -MM $(CFLAGS) $(INCS) mlpt.c > mlpt.d

mlpx.o:       $(HDRS)
mlpx.o:       mlpx.c makefile
	$(CC) $(CFLAGS) $(INCS) mlpx.c -o $@

mlpx.d:       mlpx.c makefile
	$(CC) -MM $(CFLAGS) $(INCS) mlpx.c > mlpx.d

mlps.o:       $(HDRS)
mlps.o:       mlps.c makefile
	$(CC) $(CFLAGS) $(INCS) mlps.c -o $@

mlps.d:       mlps.c makefile
	$(CC) -MM $(CFLAGS) $(INCS) mlps.c > mlps.d

#-----------------------------------------------------------------------
# Multilayer Perceptron Management
#-----------------------------------------------------------------------
mlp.o:        $(HDRS_1)
mlp.o:        mlp.h mlp.c makefile
	$(CC) $(CFLAGS) $(INCS) -DMLP_PARSE mlp.c -o $@

mlp.d:        mlp.c
	$(CC) -MM $(CFLAGS) $(INCS) -DMLP_PARSE mlp.c > mlp.d

mlp_ext.o:    $(HDRS_2)
mlp_ext.o:    mlp.h mlp.c makefile
	$(CC) $(CFLAGS) $(INCS) -DMLP_PARSE -DMLP_EXTFN mlp.c -o $@

mlp_ext.d:    mlp.c
	$(CC) -MM $(CFLAGS) $(INCS) -DMLP_PARSE -DMLP_EXTFN \
              mlp.c > mlp_ext.d

#-----------------------------------------------------------------------
# External Modules
#-----------------------------------------------------------------------
$(UTILDIR)/arrays.o:
	cd $(UTILDIR);  $(MAKE) arrays.o   ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/escape.o:
	cd $(UTILDIR);  $(MAKE) escape.o   ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/tabread.o:
	cd $(UTILDIR);  $(MAKE) tabread.o  ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/tabwrite.o:
	cd $(UTILDIR);  $(MAKE) tabwrite.o ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/scanner.o:
	cd $(UTILDIR);  $(MAKE) scanner.o  ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/nst_pars.o:
	cd $(UTILDIR);  $(MAKE) nst_pars.o ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/random.o:
	cd $(UTILDIR);  $(MAKE) random.o   ADDFLAGS="$(ADDFLAGS)"
$(UTILDIR)/params.o:
	cd $(UTILDIR);  $(MAKE) params.o   ADDFLAGS="$(ADDFLAGS)"
$(MATDIR)/mat_rdwr.o:
	cd $(MATDIR);   $(MAKE) mat_rdwr.o ADDFLAGS="$(ADDFLAGS)"
$(TABLEDIR)/attset1.o:
	cd $(TABLEDIR); $(MAKE) attset1.o  ADDFLAGS="$(ADDFLAGS)"
$(TABLEDIR)/attset2.o:
	cd $(TABLEDIR); $(MAKE) attset2.o  ADDFLAGS="$(ADDFLAGS)"
$(TABLEDIR)/attset3.o:
	cd $(TABLEDIR); $(MAKE) attset3.o  ADDFLAGS="$(ADDFLAGS)"
$(TABLEDIR)/attmap.o:
	cd $(TABLEDIR); $(MAKE) attmap.o   ADDFLAGS="$(ADDFLAGS)"
$(TABLEDIR)/table1.o:
	cd $(TABLEDIR); $(MAKE) table1.o   ADDFLAGS="$(ADDFLAGS)"
$(TABLEDIR)/tab2ro.o:
	cd $(TABLEDIR); $(MAKE) tab2ro.o   ADDFLAGS="$(ADDFLAGS)"

#-----------------------------------------------------------------------
# Source Distribution Packages
#-----------------------------------------------------------------------
dist:
	$(MAKE) clean
	cd ../..; rm -f mlp.zip mlp.tar.gz; \
        zip -rq mlp.zip    mlp/{src,ex,doc} \
                table/src/{attset.h,attset[123].c,attmap.[ch]} \
                table/src/{table.h,table[12].c} \
                table/src/{makefile,table.mak} table/doc \
                matrix/src/{matrix.h,matrix1.c} \
                matrix/src/{makefile,matrix.mak} matrix/doc \
                util/src/{fntypes.h,error.h,params.[ch]} \
                util/src/{random.[ch],nstats.[ch]} \
                util/src/{arrays.[ch],escape.[ch],symtab.[ch]} \
                util/src/{tabread.[ch],tabwrite.[ch],scanner.[ch]} \
                util/src/{makefile,util.mak} util/doc; \
        tar cfz mlp.tar.gz mlp/{src,ex,doc} \
                table/src/{attset.h,attset[123].c,attmap.[ch]} \
                table/src/{table.h,table[12].c} \
                table/src/{makefile,table.mak} table/doc \
                matrix/src/{matrix.h,matrix1.c} \
                matrix/src/{makefile,matrix.mak} matrix/doc \
                util/src/{fntypes.h,error.h,params.[ch]} \
                util/src/{random.[ch],nstats.[ch]} \
                util/src/{arrays.[ch],escape.[ch],symtab.[ch]} \
                util/src/{tabread.[ch],tabwrite.[ch],scanner.[ch]} \
                util/src/{makefile,util.mak} util/doc; \

#-----------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------
install:
	cp $(PRGS) $(HOME)/bin

#-----------------------------------------------------------------------
# Clean up
#-----------------------------------------------------------------------
clean:
	$(MAKE) localclean
	cd $(UTILDIR);  $(MAKE) clean
	cd $(MATDIR);   $(MAKE) localclean
	cd $(TABLEDIR); $(MAKE) localclean

localclean:
	rm -f *.d *.o *~ *.flc core $(PRGS)
