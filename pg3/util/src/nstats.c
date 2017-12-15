/*----------------------------------------------------------------------
  File    : nstats.c
  Contents: management of normalization statistics
  Author  : Christian Borgelt
  History : 2003.08.12 file created
            2004.08.12 description and parse function added
            2011.12.09 adapted to modified scanner module
            2011.12.15 bug in function nst_reg() fixed (weight)
            2013.03.20 sizes and lengths changed to type size_t
            2013.06.13 dimension and indices changed to DIMID
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include "nstats.h"

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define BLKSIZE    64           /* block size for parsing */

#ifndef INFINITY
#define INFINITY   (DBL_MAX+DBL_MAX)
#endif
#ifdef _MSC_VER
#ifndef snprintf
#define snprintf   _snprintf
#endif
#endif                          /* MSC still does not support C99 */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/

NSTATS* nst_create (DIMID dim)
{                               /* --- create numerical statistics */
  NSTATS *nst;                  /* created statistics structure */
  double *p;                    /* to organize the memory */

  assert(dim > 0);              /* check the function argument */
  nst = (NSTATS*)malloc(sizeof(NSTATS)
                      +(6*(size_t)dim-1) *sizeof(double));
  if (!nst) return NULL;        /* create a statistics structure */
  nst->dim  = dim;              /* and initialize the fields */
  nst->offs = p  = nst->facs +dim;
  nst->mins = p += dim;         /* organize the arrays */
  nst->maxs = p += dim;
  nst->sums = p += dim;
  nst->sqrs = p += dim;
  nst_clear(nst);               /* initialize the arrays */
  return nst;                   /* return created structure */
}  /* nst_create() */

/*--------------------------------------------------------------------*/

NSTATS* nst_clone (NSTATS *nst)
{                               /* --- create numerical statistics */
  NSTATS *clone;                /* created statistics structure */
  size_t z;                     /* size of statistics structure */
  double *p;                    /* to organize the memory */

  assert(nst);                  /* check the function argument */
  z = sizeof(NSTATS) +(6*(size_t)nst->dim-1) *sizeof(double);
  clone = (NSTATS*)malloc(z);   /* compute memory block size and */
  if (!clone) return NULL;      /* create a statistics structure */
  memcpy(clone, nst, z);        /* copy the statistics structure */
  clone->offs = p  = clone->facs +nst->dim;
  clone->mins = p += nst->dim;  /* organize the arrays */
  clone->maxs = p += nst->dim;
  clone->sums = p += nst->dim;
  clone->sqrs = p += nst->dim;
  return clone;                 /* return the created clone */
}  /* nst_clone() */

/*--------------------------------------------------------------------*/

void nst_clear (NSTATS *nst)
{                               /* --- clear numerical statistics */
  DIMID i;                      /* loop variable */

  assert(nst);                  /* check the function argument */
  nst->wgt = 0;                 /* clear the data vector counter */
  memset(nst->offs, 0, (size_t)nst->dim *sizeof(double));
  memset(nst->sums, 0, (size_t)nst->dim *sizeof(double));
  memset(nst->sqrs, 0, (size_t)nst->dim *sizeof(double));
  for (i = 0; i < nst->dim; i++) {
    nst->mins[i] = +INFINITY;   /* traverse the arrays and */
    nst->maxs[i] = -INFINITY;   /* initialize the ranges of values */
    nst->facs[i] = 1;           /* and the scaling factors */
  }
}  /* nst_clear() */

/*--------------------------------------------------------------------*/

void nst_reg (NSTATS *nst, const double *vec, double wgt)
{                               /* --- register a data vector */
  DIMID  i;                     /* loop variable */
  double t;                     /* temporary buffer */

  assert(nst && vec);           /* check the function arguments */
  if (!vec) {                   /* if to terminate registration */
    if (nst->wgt <= 0) return;  /* check for registered data vectors */
    for (i = 0; i < nst->dim; i++) {
      nst->offs[i] = nst->sums[i] /nst->wgt;
      t            = nst->sqrs[i] -nst->offs[i] *nst->sums[i];
      nst->facs[i] = (t > 0) ? sqrt(nst->wgt /t) : 1;
    } }                         /* estimate the scaling parameters */
  else if (wgt > 0) {           /* if to register a data vector */
    nst->wgt += wgt;            /* sum the data vector weight */
    for (i = 0; i < nst->dim; i++) {
      if (vec[i] < nst->mins[i]) nst->mins[i] = vec[i];
      if (vec[i] > nst->maxs[i]) nst->maxs[i] = vec[i];
      nst->sums[i] += t = wgt *vec[i];
      nst->sqrs[i] +=       t *vec[i];
    }                           /* update the ranges of values and */
  }                             /* sum the values and their squares */
}  /* nst_reg() */

/*--------------------------------------------------------------------*/

void nst_range (NSTATS *nst, DIMID idx, double min, double max)
{                               /* --- set range of values */
  DIMID i;                      /* loop variable */

  assert(nst && (idx < nst->dim));  /* check the arguments */
  if (idx < 0) { i = nst->dim; idx = 0; }
  else         { i = idx +1; }  /* get index range to set */
  while (--i >= idx) {          /* and traverse it */
    nst->mins[i] = min;         /* set the minimal */
    nst->maxs[i] = max;         /* and the maximal value */
  }                             /* for all dimensions in range */
}  /* nst_range() */

/*--------------------------------------------------------------------*/

void nst_expand (NSTATS *nst, DIMID idx, double factor)
{                               /* --- expand range of values */
  DIMID  i;                     /* loop variable */
  double t;                     /* change of minimal/maximal value */

  assert(nst                    /* check the function arguments */
     && (idx < nst->dim) && (factor >= 0));
  if (idx < 0) { i = nst->dim; idx = 0; }
  else         { i = idx +1; }  /* get index range to expand */
  factor = (factor-1) *0.5;     /* and factor for expansion */
  while (--i >= idx) {          /* traverse the index range */
    t = (nst->maxs[i] -nst->mins[i]) *factor;
    nst->mins[i] -= t;          /* adapt the minimal */
    nst->maxs[i] += t;          /* and   the maximal value */
  }                             /* for all dimensions in range */
}  /* nst_expand() */

/*--------------------------------------------------------------------*/

void nst_scale (NSTATS *nst, DIMID idx, double off, double fac)
{                               /* --- set (linear) scaling */
  DIMID i;                      /* loop variable */

  assert(nst && (idx < nst->dim));  /* check the arguments */
  if (idx < 0) { i = nst->dim; idx = 0; }
  else         { i = idx +1; }  /* get index range to set */
  while (--i >= idx) {          /* and traverse it */
    nst->offs[i] = off;         /* set the offset */
    nst->facs[i] = fac;         /* and the scaling factor */
  }                             /* for all dimensions in range */
}  /* nst_scale() */

/*--------------------------------------------------------------------*/

void nst_norm (NSTATS *nst, const double *vec, double *res)
{                               /* --- normalize a data vector */
  DIMID i;                      /* loop variable */
  assert(nst && vec && res);    /* check the function arguments */
  for (i = 0; i < nst->dim; i++)/* normalize the data vector */
    res[i] = nst->facs[i] *(vec[i] -nst->offs[i]);
}  /* nst_norm() */

/*--------------------------------------------------------------------*/

void nst_denorm (NSTATS *nst, const double *vec, double *res)
{                               /* --- denormalize a vector */
  DIMID i;                      /* loop variable */
  assert(nst && vec && res);    /* check the function arguments */
  for (i = 0; i < nst->dim; i++)/* denormalize the data vector */
    res[i] = (vec[i] / nst->facs[i]) +nst->offs[i];
}  /* nst_denorm() */

/*--------------------------------------------------------------------*/

void nst_center (NSTATS *nst, double *vec)
{                               /* --- get center of data space */
  DIMID i;                      /* loop variable */
  assert(nst && vec);           /* check the function arguments */
  for (i = 0; i < nst->dim; i++)/* compute the center vector */
    vec[i] = 0.5 *nst->mins[i] +0.5 *nst->maxs[i];
}  /* nst_center() */

/*--------------------------------------------------------------------*/

void nst_spans (NSTATS *nst, double *vec)
{                               /* --- get spans of dimensions */
  DIMID i;                      /* loop variable */
  assert(nst && vec);           /* check the function arguments */
  for (i = 0; i < nst->dim; i++)/* compute the spans */
    vec[i] = nst->maxs[i] -nst->mins[i];
}  /* nst_spans() */

/*--------------------------------------------------------------------*/

int nst_desc (NSTATS *nst, FILE *file, const char *indent, int maxlen)
{                               /* --- describe norm. statistics */
  DIMID i;                      /* loop variable */
  int   pos, ind;               /* position in output line */
  char  buf[64];                /* buffer for output */

  for (i = nst->dim; --i >= 0;) /* check for non-identity scaling */
    if ((nst->offs[i] != 0) || (nst->facs[i] != 1)) break;
  if (i < 0) return 0;          /* if only identity scaling, abort */
  if (!indent) indent = "";     /* ensure a proper indentation string */
  fputs(indent,        file);   /* write the indentation and */
  fputs("scales   = ", file);   /* start the scaling parameters */
  ind = (int)strlen(indent);    /* compute the indentation */
  pos = ind +11;                /* and the starting position */
  for (i = 0; i < nst->dim; i++) {
    pos += snprintf(buf, sizeof(buf), "[%.16g, %.16g]",
                    nst->offs[i], nst->facs[i]);
    if (i > 0) {                /* format the scaling parameters */
      if (pos+3 <= maxlen) { fputs(", ", file);         pos += 2;   }
      else { fprintf(file, ",\n%s           ", indent); pos  = ind; }
    }                           /* print separator and indentation */
    fputs(buf, file);           /* print formatted offset and factor */
  }
  fputs(";\n", file);           /* terminate the scales list */
  return ferror(file);          /* return the write status */
}  /* nst_desc() */

/*--------------------------------------------------------------------*/
#ifdef NST_PARSE

static DIMID parse (SCANNER *scan, DIMID dim, double **buf)
{                               /* --- parse normalization statistics */
  DIMID  k, n = 0;              /* loop variable, counter */
  double *p;                    /* to access the statistics elements */

  assert(scan);                 /* check the function arguments */
  if ((scn_token(scan) != T_ID) /* check whether 'scales' follows */
  ||  (strcmp(scn_value(scan), "scales") != 0))
    SCN_ERROR(scan, E_STREXP, "scales");
  SCN_NEXT(scan);               /* consume 'scales' */
  SCN_CHAR(scan, '=');          /* consume '=' */
  for (k = 0; (dim <= 0) || (k < dim); k++) {
    if (k >  0) { SCN_CHAR(scan, ','); }
    if (k >= n) {               /* if the statistics array is full */
      if (dim > 0) n  = dim;    /* compute the new array size */
      else         n += (n > BLKSIZE) ? n >> 1 : BLKSIZE;
      p = (double*)realloc(*buf, ((size_t)n+(size_t)n) *sizeof(double));
      if (!p) SCN_ERROR(scan, E_NOMEM);
      *buf = p;                 /* enlarge the buffer array */
    }                           /* then note factor and offset */
    p = *buf +k +k;             /* get the element to set */
    SCN_CHAR(scan, '[');        /* consume '[' */
    if (scn_token(scan) != T_NUM) SCN_ERROR(scan, E_NUMEXP);
    p[0] = strtod(scn_value(scan), NULL);
    SCN_NEXT(scan);             /* consume the offset */
    SCN_CHAR(scan, ',');        /* consume '[' */
    if (scn_token(scan) != T_NUM) SCN_ERROR(scan, E_NUMEXP);
    p[1] = strtod(scn_value(scan), NULL);
    SCN_NEXT(scan);             /* consume the factor */
    SCN_CHAR(scan, ']');        /* consume '[' */
    if ((dim <= 0) && (scn_token(scan) != ',')) {
      k++; break; }             /* check for more scaling params. */
  }
  SCN_CHAR(scan, ';');          /* consume ';' */
  return k;                     /* return 'ok' */
}  /* parse() */

/*--------------------------------------------------------------------*/

NSTATS* nst_parse (SCANNER *scan, DIMID dim)
{                               /* --- parse normalization statistics */
  NSTATS *nst;                  /* created normalization statistics */
  double *buf = NULL;           /* buffer for reading */

  assert(scan);                 /* check the function arguments */
  scn_first(scan);              /* get first token (if not started) */
  dim = parse(scan,dim, &buf);  /* parse normalization statistics */
  if (dim < 0) { if (buf) free(buf); return NULL; }
  nst = nst_create(dim);        /* create a statistics structure */
  if (!nst)    { free(buf); return NULL; }
  for (buf += dim +dim; --dim >= 0; ) {
    nst->facs[dim] = *--buf;    /* copy the buffered values */
    nst->offs[dim] = *--buf;    /* into the corresponding arrays */
  }
  free(buf);                    /* delete the read buffer */
  return nst;                   /* return the created structure */
}  /* nst_parse() */

#endif
