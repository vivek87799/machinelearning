/*----------------------------------------------------------------------
  File    : nstats.h
  Contents: management of normalization statistics
  Author  : Christian Borgelt
  History : 2003.08.12 file created
            2004.08.12 description and parse function added
            2011.12.15 functions nst_clear() and nst_wgt() added
            2013.03.20 sizes and lengths changed to type size_t
            2013.06.13 dimension and indices changed to DIMID
----------------------------------------------------------------------*/
#ifndef __NSTATS__
#define __NSTATS__
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#ifdef NST_PARSE
#ifndef SCN_SCAN
#define SCN_SCAN
#endif
#include "scanner.h"
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#ifdef EXTDATA

#ifndef DIMID
#define DIMID       ptrdiff_t   /* dimension identifier */
#endif

#else

#ifndef DIMID
#define DIMID       int         /* dimension identifier */
#endif

#endif
/*--------------------------------------------------------------------*/

#define int         1           /* to check definitions */
#define long        2           /* for certain types */
#define ptrdiff_t   3

/*--------------------------------------------------------------------*/

#if   DIMID==int
#ifndef DIMID_MAX
#define DIMID_MAX   INT_MAX     /* maximum dimension identifier */
#endif
#ifndef DIMID_SGN
#define DIMID_SGN   INT_MIN     /* sign of dimension identifier */
#endif
#ifndef DIMID_FMT
#define DIMID_FMT   "d"         /* printf format code for int */
#endif
#define strtodim(s,p)   (int)strtol(s,p,0)

#elif DIMID==long
#ifndef DIMID_MAX
#define DIMID_MAX   LONG_MAX    /* maximum dimension identifier */
#endif
#ifndef DIMID_SGN
#define DIMID_SGN   LONG_MIN    /* sign of dimension identifier */
#endif
#ifndef DIMID_FMT
#define DIMID_FMT   "ld"        /* printf format code for long */
#endif
#define strtodim(s,p)   strtol(s,p,0)

#elif DIMID==ptrdiff_t
#ifndef DIMID_MAX
#define DIMID_MAX   PTRDIFF_MAX /* maximum dimension identifier */
#endif
#ifndef DIMID_SGN
#define DIMID_SGN   PTRDIFF_MIN /* sign of dimension identifier */
#endif
#ifndef DIMID_FMT
#ifdef _MSC_VER
#define DIMID_FMT   "Id"        /* printf format code for ptrdiff_t */
#else
#define DIMID_FMT   "td"        /* printf format code for ptrdiff_t */
#endif                          /* MSC still does not support C99 */
#endif
#define strtodim(s,p)   (ptrdiff_t)strtoll(s,p,0)

#else
#error "DIMID must be either 'int', 'long' or 'ptrdiff_t'"
#endif

/*--------------------------------------------------------------------*/

#undef int                      /* remove preprocessor definitions */
#undef long                     /* needed for the type checking */
#undef ptrdiff_t

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- normalization statistics --- */
  DIMID  dim;                   /* dimension of data space */
  double wgt;                   /* weight of registered patterns */
  double *mins;                 /* minimal data values */
  double *maxs;                 /* maximal data values */
  double *sums;                 /* sums of data values */
  double *sqrs;                 /* sums of squared data values */
  double *offs;                 /* offsets for data scaling */
  double facs[1];               /* factors for data scaling */
} NSTATS;                       /* (numerical statistics) */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern NSTATS* nst_create (DIMID dim);
extern void    nst_delete (NSTATS *nst);
extern NSTATS* nst_clone  (NSTATS *nst);
extern DIMID   nst_dim    (NSTATS *nst);
extern double  nst_wgt    (NSTATS *nst);

extern void    nst_clear  (NSTATS *nst);
extern void    nst_reg    (NSTATS *nst, const double *vec, double wgt);
extern void    nst_range  (NSTATS *nst, DIMID idx,
                           double min, double max);
extern void    nst_expand (NSTATS *nst, DIMID idx, double factor);
extern void    nst_scale  (NSTATS *nst, DIMID idx,
                           double off, double fac);

extern double  nst_min    (NSTATS *nst, DIMID idx);
extern double  nst_max    (NSTATS *nst, DIMID idx);
extern double  nst_offset (NSTATS *nst, DIMID idx);
extern double  nst_factor (NSTATS *nst, DIMID idx);

extern void    nst_norm   (NSTATS *nst, const double *vec, double *res);
extern void    nst_denorm (NSTATS *nst, const double *vec, double *res);
extern void    nst_center (NSTATS *nst, double *vec);
extern void    nst_spans  (NSTATS *nst, double *vec);

extern int     nst_desc   (NSTATS *nst, FILE *file,
                           const char *indent, int maxlen);
#ifdef NST_PARSE
extern NSTATS* nst_parse  (SCANNER *scan, DIMID dim);
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define nst_delete(s)     free(s)
#define nst_dim(s)        ((s)->dim)
#define nst_wgt(s)        ((s)->wgt)
#define nst_min(s,i)      ((s)->mins[i])
#define nst_max(s,i)      ((s)->maxs[i])
#define nst_offset(s,i)   ((s)->offs[i])
#define nst_factor(s,i)   ((s)->facs[i])

#endif
