/*----------------------------------------------------------------------
  File    : params.c
  Contents: command line parameter retrieval
  Author  : Christian Borgelt
  History : 2003.06.05 file created
            2010.10.29 functions getintvar() and getdblvar() added
            2010.10.29 functions getintvar() and getdblvar() simplified
----------------------------------------------------------------------*/
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include "params.h"

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/

int getints (char *s, char **end, int n, ...)
{                               /* --- get integer parameters */
  va_list args;                 /* list of variable arguments */
  int     k = 0, t;             /* parameter counter, buffer */

  assert(s && end);             /* check the function arguments */
  va_start(args, n);            /* get variable arguments */
  while (k < n) {               /* traverse the arguments */
    t = (int)strtol(s, end, 0); /* get the next parameter and */
    if (*end == s) break;       /* check for an empty parameter */
    *(va_arg(args, int*)) = t;  /* store the parameter */
    k++;                        /* and count it */
    s = *end; if (*s++ != ':') break;
  }                             /* check for a colon */
  va_end(args);                 /* end argument evaluation */
  return k;                     /* return the number of parameters */
}  /* getints() */

/*--------------------------------------------------------------------*/

int getdbls (char *s, char **end, int n, ...)
{                               /* --- get double parameters */
  va_list args;                 /* list of variable arguments */
  int     k = 0;                /* parameter counter */
  double  t;                    /* temporary buffer */

  assert(s && end);             /* check the function arguments */
  va_start(args, n);            /* get variable arguments */
  while (k < n) {               /* traverse the arguments */
    t = strtod(s, end);         /* get the next parameter and */
    if (*end == s) break;       /* check for an empty parameter */
    *(va_arg(args, double*)) = t;  /* store the parameter */
    k++;                           /* and count it */
    s = *end; if (*s++ != ':') break;
  }                             /* check for a colon */
  va_end(args);                 /* end argument evaluation */
  return k;                     /* return the number of parameters */
}  /* getdbls() */

/*--------------------------------------------------------------------*/

int getintvec (char *s, char **end, int n, int *params)
{                               /* --- get integer parameter array */
  int k = 0, t;                 /* parameter counter, buffer */

  assert(s && end               /* check the function arguments */
  &&    (params || (n <= 0)));
  while (k < n) {               /* traverse the parameters */
    t = (int)strtol(s, end, 0); /* get the next parameter and */
    if (*end == s) break;       /* check for an empty parameter */
    params[k++] = t;            /* store and count the parameter */
    s = *end; if (*s++ != ':') break;
  }                             /* check for a colon */
  return k;                     /* return the number of parameters */
}  /* getintvec() */

/*--------------------------------------------------------------------*/

int getdblvec (char *s, char **end, int n, double *params)
{                               /* --- get double parameter array */
  int    k = 0;                 /* parameter counter */
  double t;                     /* temporary buffer */

  assert(s && end               /* check the function arguments */
  &&    (params || (n <= 0)));
  while (k < n) {               /* traverse the parameters */
    t = strtod(s, end);         /* get the next parameter and */
    if (*end == s) break;       /* check for an empty parameter */
    params[k++] = t;            /* store and count the parameter */
    s = *end; if (*s++ != ':') break;
  }                             /* check for a colon */
  return k;                     /* return the number of parameters */
}  /* getdblvec() */

/*--------------------------------------------------------------------*/

int getintvar (char *s, char **end, int **p)
{                               /* --- get integer parameter array */
  int k, n;                     /* parameter counters */

  assert(s && end && p);        /* check the function arguments */
  for (n = k = 0; s[k]; k++)    /* traverse the string and */
    if (s[k] == ':') n++;       /* count the number separators */
  *p = (int*)malloc((size_t)++n *sizeof(int));
  return (*p) ? getintvec(s, end, n, *p) : -1;
}  /* getintvar() */

/*--------------------------------------------------------------------*/

int getdblvar (char *s, char **end, double **p)
{                               /* --- get double parameter array */
  int k, n;                     /* parameter counters */

  assert(s && end && p);        /* check the function arguments */
  for (n = k = 0; s[k]; k++)    /* traverse the string and */
    if (s[k] == ':') n++;       /* count the number separators */
  *p = (double*)malloc((size_t)++n *sizeof(double));
  return (*p) ? getdblvec(s, end, n, *p) : -1;
}  /* getdblvar() */
