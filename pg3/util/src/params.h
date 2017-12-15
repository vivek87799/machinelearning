/*----------------------------------------------------------------------
  File    : params.h
  Contents: command line parameter retrieval
  Author  : Christian Borgelt
  History : 2003.06.05 file created
            2010.10.29 functions getintvar() and getdblvar() added
----------------------------------------------------------------------*/
#ifndef __PARAMS__
#define __PARAMS__

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern int getints   (char *s, char **end, int n, ...);
extern int getdbls   (char *s, char **end, int n, ...);

extern int getintvec (char *s, char **end, int n, int    *params);
extern int getdblvec (char *s, char **end, int n, double *params);

extern int getintvar (char *s, char **end, int    **p);
extern int getdblvar (char *s, char **end, double **p);

#endif
