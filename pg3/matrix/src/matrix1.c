/*----------------------------------------------------------------------
  File    : matrix1.c
  Contents: general vector and matrix management
            without matrix inversion and equation system solving and
            without eigenvalue and eigenvector functions
  Author  : Christian Borgelt
  History : 1999.04.30 file created
            1999.05.13 vector and matrix read functions completed
            2001.07.14 adapted to modified module tabread
            2001.08.17 VECTOR data type replaced by double*
            2001.10.13 parameters vec and mat added to read functions
            2001.10.15 [vec|mat]_show() made macros, mat_sub() added
            2001.10.25 function mat_subx() added
            2001.11.07 arg. loc added to mat_[clone|copy|diff]()
            2001.11.09 functions mat_diasum() and mat_diaprod() added
            2001.11.10 functions vec_sqrlen() and vec_sqrdist() added
            2001.11.11 function mat_mulvdv() added
            2002.08.07 bug in read error reporting fixed
            2004.03.01 function mat_copy() extended, mat_diadds() added
            2004.04.23 function mat_dialog() added
            2004.08.18 function mat_muldv() added
            2007.02.13 adapted to modified module tabread
            2007.11.23 functions mat_mulmmt() and mat_mulmvmt() added
            2007.11.29 reference to associated vector simplified
            2007.12.01 function vec_unitlen() added
            2007.12.03 several functions added and others extended
            2008.02.28 some vector functions added (e.g. vec_sum())
            2008.03.27 semantics of associated vector simplified
            2008.04.22 lexicographic vector comparison added
            2008.04.25 use of matrix locations made more consistent
            2010.10.11 adapted to modified module tabread
            2010.11.10 interface of vec_readx() and mat_readx() adapted
            2011.06.21 bugs in vec_readx() and mat_readx() fixed
            2013.08.09 adapted to higher compiler warning level
            2013.08.13 adapted to preprocessor definition of DIMID
            2015.07.30 functions vec_[abs]max() and mat_emul() added
----------------------------------------------------------------------*/
#include <stdio.h>
#include <limits.h>
#include <float.h>
#include <assert.h>
#include "matrix.h"
#ifdef STORAGE
#include "storage.h"
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
/* --- matrix flags --- */
#define ROWBLKS    0x0001       /* one memory block per matrix row */
#define ODDPERM    0x0002       /* odd row permutation (LU decomp.) */

/* --- sizes --- */
#define BLKSIZE    256          /* block size for matrices */

/* --- error functions --- */
#define VECERR(c,v) { if (*v) { free(*v);       *v = NULL; } return c; }
#define MATERR(c,m) { if (*m) { mat_delete(*m); *m = NULL; } return c; }

/*----------------------------------------------------------------------
  Basic Vector Functions
----------------------------------------------------------------------*/

double* vec_init (double *vec, DIMID n, DIMID unit)
{                               /* --- initialize a vector */
  assert(vec && (unit < n));    /* check the function arguments */
  memset(vec, 0, (size_t)n*sizeof(double)); /* clear the vector */
  if (unit >= 0) vec[unit] = 1; /* set a unit direction */
  return vec;                   /* return the initialized vector */
}  /* vec_init() */

/*--------------------------------------------------------------------*/

void vec_set (double *vec, DIMID n, double x)
{                               /* --- set vector elements */
  assert(vec);                  /* check the function argument */
  while (--n >= 0) vec[n] = x;  /* set elements to given value */
}  /* vec_set() */

/*--------------------------------------------------------------------*/

double vec_diff (const double *a, const double *b, DIMID n)
{                               /* --- compute max. coord. difference */
  double max = 0, t;            /* maximal difference, buffer */

  assert(a && b);               /* check the function arguments */
  while (--n >= 0) {            /* traverse the vector elements, */
    t = fabs(a[n] -b[n]);       /* compute their differences, */
    if (t > max) max = t;       /* and determine the maximum */
  }                             /* of these differences */
  return max;                   /* return the maximum difference */
}  /* vec_diff() */

/*--------------------------------------------------------------------*/

int vec_cmp (const double *a, const double *b, DIMID n)
{                               /* --- compare vectors lexicograph. */
  DIMID i;                      /* loop variable */

  assert(a && b);               /* check the function arguments */
  for (i = 0; i < n; i++) {     /* traverse the coordinates */
    if (a[i] < b[i]) return -1; /* if the coordinates differ, */
    if (a[i] > b[i]) return +1; /* return the comparison result */
  }
  return 0;                     /* otherwise vectors are equal */
}  /* vec_cmp() */

/*----------------------------------------------------------------------
  Vector Element Operations
----------------------------------------------------------------------*/

double vec_sum (const double *vec, DIMID n)
{                               /* --- sum vector elements */
  double sum = 0;               /* sum of vector elements */

  assert(vec);                  /* check the function argument */
  while (--n >= 0) sum += vec[n];
  return sum;                   /* compute the sum of the elements */
}  /* vec_sum() */

/*--------------------------------------------------------------------*/

double vec_sumrec (const double *vec, DIMID n)
{                               /* --- sum reciprocal values */
  double sum = 0;               /* sum of reciprocal values */

  assert(vec);                  /* check the function argument */
  while (--n >= 0) {            /* traverse the vector elements */
    if (fabs(vec[n]) <= DBL_MIN) return INFINITY;
    sum += 1/vec[n];            /* compute the sum of */
  }                             /* the reciprocal elements */
  return sum;                   /* return the computed sum */
}  /* vec_sumrec() */

/*--------------------------------------------------------------------*/

double vec_sumlog (const double *vec, DIMID n)
{                               /* --- sum logs. of vector elements */
  double sum = 0;               /* sum of logarithms of elements */

  assert(vec);                  /* check the function argument */
  while (--n >= 0) {            /* traverse the vector elements */
    if (fabs(vec[n]) <= DBL_MIN) return -INFINITY;
    sum += log(vec[n]);         /* compute the sum of the logarithms */
  }                             /* of the vector elements */
  return sum;                   /* return the computed sum */
}  /* vec_sumlog() */

/*--------------------------------------------------------------------*/

double vec_prod (const double *vec, DIMID n)
{                               /* --- multiply vector elements */
  double prod = 1;              /* sum of squared elements */

  assert(vec);                  /* check the function argument */
  while (--n >= 0) prod *= vec[n];
  return prod;                  /* multiply the vector elements */
}  /* vec_prod() */

/*--------------------------------------------------------------------*/

double vec_max (const double *vec, DIMID n)
{                               /* --- find maximum element */
  double max = -INFINITY;       /* maximum of vector elements */

  assert(vec);                  /* check the function argument */
  while (--n >= 0) { if (vec[n] > max) max = vec[n]; }
  return max;                   /* multiply the vector elements */
}  /* vec_max() */

/*--------------------------------------------------------------------*/

double vec_absmax (const double *vec, DIMID n)
{                               /* --- find maximum element */
  double max = -INFINITY;       /* maximum of vector elements */

  assert(vec);                  /* check the function argument */
  while (--n >= 0) { if (fabs(vec[n]) > max) max = fabs(vec[n]); }
  return max;                   /* multiply the vector elements */
}  /* vec_absmax() */

/*----------------------------------------------------------------------
  Vector Operations
----------------------------------------------------------------------*/

double vec_sqrlen (const double *vec, DIMID n)
{                               /* --- compute squared vector length */
  double sum = 0;               /* sum of squared elements */

  assert(vec);                  /* check the function argument */
  while (--n >= 0) sum += vec[n] *vec[n];
  return sum;                   /* compute the squared vector length */
}  /* vec_sqrlen() */

/*--------------------------------------------------------------------*/

double* vec_unitlen (double *res, const double *vec, DIMID n)
{                               /* --- make vector of unit length */
  double len;                   /* vector length */

  assert(res && vec);           /* check the function arguments */
  len = vec_len(vec, n);        /* compute the vector length */
  if (len > 0) { len = 1/len;   /* normalize the vector */
    while (--n >= 0) res[n] = vec[n] *len; }
  else if (res != vec) {        /* otherwise set the null vector */
    while (--n >= 0) res[n] = 0; }
  return res;                   /* return the result vector */
}  /* vec_unitlen() */

/*--------------------------------------------------------------------*/

double vec_sqrdist (const double *a, const double *b, DIMID n)
{                               /* --- compute squared distance */
  double sum = 0;               /* sum of element products */
  double t;                     /* element of difference vector */

  assert(a && b);               /* check the function arguments */
  while (--n >= 0) { t = a[n]-b[n]; sum += t*t; }
  return sum;                   /* compute the squared distance */
}  /* vec_sqrdist() */

/*--------------------------------------------------------------------*/

double* vec_add (double *res, DIMID n,
                 const double *a, double k, const double *b)
{                               /* --- add a vector to another */
  assert(res && a && b && (n > 0));  /* check the function arguments */
  if      (k == +1)             /* if to add the vectors */
    while (--n >= 0) res[n] = a[n] +   b[n];
  else if (k == -1)             /* if to subtract the vectors */
    while (--n >= 0) res[n] = a[n] -   b[n];
  else                          /* if to add an arbitrary multiple */
    while (--n >= 0) res[n] = a[n] +k *b[n];
  return res;                   /* return the result vector */
}  /* vec_add() */

/*--------------------------------------------------------------------*/

double* vec_muls (double *res, DIMID n, const double *vec, double k)
{                               /* --- multiply vector with scalar */
  assert(res && vec && (n > 0));/* check the function arguments */
  if      (k == +1)             /* if to copy the vector */
    while (--n >= 0) res[n] =  vec[n];
  else if (k == -1)             /* if to negate the vector */
    while (--n >= 0) res[n] = -vec[n];
  else                          /* otherwise (arbitrary factor) */
    while (--n >= 0) res[n] =  vec[n] *k;
  return res;                   /* multiply the vector elements */
}  /* vec_muls() */             /* and return the result vector */

/*--------------------------------------------------------------------*/

double vec_sclmul (const double *a, const double *b, DIMID n)
{                               /* --- compute scalar product */
  double sum = 0;               /* sum of element products */

  assert(a && b && (n > 0));    /* check the function arguments */
  while (--n >= 0) sum += a[n] *b[n];
  return sum;                   /* sum the products of the elements */
}  /* vec_sclmul() */           /* and return the result */

/*--------------------------------------------------------------------*/

double* vec_vecmul (double *res, const double *a, const double *b)
{                               /* --- compute vector product */
  double x, y;                  /* temporary buffers */

  assert(a && b && res);        /* check the function arguments */
  x      = a[1]*b[2] - a[2]*b[1];
  y      = a[2]*b[0] - a[0]*b[2];
  res[2] = a[0]*b[1] - a[1]*b[0];
  res[1] = y;                   /* assign results to buffers first */
  res[0] = x;                   /* to allow res = a or res = b, */
  return res;                   /* then copy buffers to result */
}  /* vec_vecmul() */

/*--------------------------------------------------------------------*/

MATRIX* vec_matmul (MATRIX *res, const double *a, const double *b)
{                               /* --- compute matrix product */
  DIMID  row, col;              /* loop variables */
  double *d;                    /* to traverse matrix rows */
  double t;                     /* temporary buffer */

  assert(a && b && res);        /* check the function arguments */
  for (row = res->rowcnt; --row >= 0; ) {
    d = res->els[row];          /* traverse the matrix rows */
    t = a[row];                 /* and the vector a */
    for (col = res->colcnt; --col >= 0; )
      d[col] = t *b[col];       /* multiply each element of the */
  }                             /* vector a with the vector b and */
  return res;                   /* store the result in the rows */
}  /* vec_matmul() */

/*--------------------------------------------------------------------*/
#ifdef MAT_RDWR

int vec_write (const double *vec, DIMID n, FILE *file,
               int digs, const char *sep)
{                               /* --- write a vector to a file */
  assert(vec                    /* check the function arguments */
  &&     file && sep && (n > 0));
  while (--n >= 0) {            /* traverse the vector elements */
    fprintf(file, "%-.*g", digs, *vec++);
    if (n > 0) fputc(sep[0], file);  /* print a vector element */
  }                                  /* and an element separator */
  fputs(sep+1, file);           /* print a vector separator */
  return ferror(file) ? -1 : 0; /* check for an error */
}  /* vec_write() */

/*--------------------------------------------------------------------*/

int vec_read (double *vec, DIMID n, TABREAD *tread)
{                               /* --- read a vector from a file */
  DIMID i = 0;                  /* field index */
  int   d;                      /* delimiter type */
  char  *s, *e;                 /* field read, end pointer */

  assert(vec && tread && (n > 0)); /* check the function arguments */
  s = trd_field(tread);         /* get the read buffer */
  do {                          /* vector element read loop */
    d = trd_read(tread);        /* read the next vector element */
    if  (d <= TRD_ERR)  return E_FREAD;
    if ((d <= TRD_EOF) && (i <= 0) && !*s) return 1;
    vec[i++] = strtod(s, &e);   /* convert and store the value read */
    if (*e || (e == s)) return E_VALUE;
  } while (d == TRD_FLD);       /* while not at end of record */
  if (i != n) return E_FLDCNT;  /* check for right number of fields */
  return 0;                     /* return 'ok' */
}  /* vec_read() */

/*--------------------------------------------------------------------*/

int vec_readx (double **vec, DIMID *n, TABREAD *tread)
{                               /* --- read a vector from a file */
  double *p;                    /* reallocation buffer */
  DIMID  i, k;                  /* field index and vector size */
  int    d;                     /* delimiter type */
  char   *s, *e;                /* field read, end pointer */

  assert(vec && n && tread);    /* check the function arguments */
  k = i = 0; *vec = NULL;       /* initialize the index variables */
  s = trd_field(tread);         /* and get the read buffer */
  do {                          /* vector element read loop */
    d = trd_read(tread);        /* read the next vector element */
    if (d <= TRD_ERR) VECERR(E_FREAD, vec);
    if ((d <= TRD_EOF) && (i <= 0) && !*s) return 1;
    if (i >= k) {               /* if the current vector is full */
      if      (*n >  k) k = *n; /* get the new vector size */
      else if (*n <= 0) k += (k > BLKSIZE) ? k >> 1 : BLKSIZE;
      else VECERR(E_FLDCNT, vec);
      p = (double*)realloc(*vec, (size_t)k *sizeof(double));
      if (!p) VECERR(E_NOMEM, vec);
      *vec = p;                 /* allocate/enlarge the vector */
    }                           /* and set the new vector */
    (*vec)[i++] = strtod(s,&e); /* convert and store the value read */
    if (*e || (e == s)) VECERR(E_VALUE, vec);
  } while (d == TRD_FLD);       /* while not at end of record */
  if (i < k) {                  /* check the number of fields */
    if (*n > 0) VECERR(E_FLDCNT, vec);
    *vec = (double*)realloc(*vec, (size_t)i *sizeof(double));
  }                             /* try to shrink the vector */
  *n = i;                       /* note the number of elements */
  return 0;                     /* return the created vector */
}  /* vec_readx() */

#endif
/*----------------------------------------------------------------------
  Basic Matrix Functions
----------------------------------------------------------------------*/

MATRIX* mat_create (DIMID rowcnt, DIMID colcnt)
{                               /* --- create a matrix */
  MATRIX *mat;                  /* created matrix */
  double *p;                    /* to traverse the matrix elements */
  DIMID  m, n;                  /* number of matrix/buffer elements */

  assert((rowcnt > 0)           /* check the function arguments */
  &&     (colcnt > 0));         /* (at least one row and column) */
  m   = (rowcnt > colcnt) ? rowcnt : colcnt;
  mat = (MATRIX*)malloc(sizeof(MATRIX)
                      +(size_t)(rowcnt-1) *sizeof(double*)
                      +(size_t)(m+m)      *sizeof(DIMID));
  if (!mat) return NULL;        /* allocate the matrix body */
  mat->rowcnt = rowcnt;         /* note the number of rows */
  mat->colcnt = colcnt;         /* and  the number of columns */
  mat->flags  = 0;              /* clear the matrix flags */
  mat->weight = 0;              /* and the vector weight sum */
  mat->map    = (DIMID*)(mat->els +rowcnt);
  n = rowcnt *colcnt;           /* get the number of matrix elements */
  mat->vec = p = (double*)malloc((size_t)(n+m+m) *sizeof(double));
  if (!p) { free(mat); return NULL; }
  mat->buf = p += m;            /* allocate the matrix elements */
  for (p += n+m; --rowcnt >= 0; )    /* organize the memory */
    mat->els[rowcnt] = p -= colcnt;  /* into matrix rows */
  return mat;                   /* return the created matrix */
}  /* mat_create() */

/*--------------------------------------------------------------------*/

void mat_delete (MATRIX *mat)
{                               /* --- delete a matrix */
  DIMID row;                    /* loop variable */

  assert(mat);                  /* check the function argument */
  if (mat->flags & ROWBLKS) {   /* if one memory block per row */
    for (row = mat->rowcnt; --row >= 0; )
      if (mat->els[row]) free(mat->els[row]);
  }                             /* delete the matrix rows */
  if (mat->vec) free(mat->vec); /* delete the associated vector */
  free(mat);                    /* and the matrix body */
}  /* mat_delete() */

/*--------------------------------------------------------------------*/

MATRIX* mat_clonex (const MATRIX *mat, int loc)
{                               /* --- clone a matrix */
  MATRIX *clone;                /* created clone */

  assert(mat);                  /* check the function argument */
  clone = mat_create(mat->rowcnt, mat->colcnt);
  if (!clone) return NULL;      /* create a new matrix and copy */
  return mat_copyx(clone, mat, loc);  /* the old matrix into it */
}  /* mat_clonex() */

/*--------------------------------------------------------------------*/

MATRIX* mat_copyx (MATRIX *dst, const MATRIX *src, int loc)
{                               /* --- copy a matrix */
  DIMID  row, col;              /* loop variables */
  double *d; const double *s;   /* matrix rows of dest. and source */

  assert(src && dst             /* check the function arguments */
  &&    (src->colcnt == dst->colcnt)
  &&    (src->rowcnt == dst->rowcnt));
  assert(!(loc & (MAT_LOWER|MAT_UPPER))
  ||    (dst->rowcnt == dst->colcnt));
  if (loc & MAT_VECTOR) {       /* if to copy the associated vector */
    loc &= ~MAT_VECTOR;         /* remove the vector flag */
    d = dst->vec; s = src->vec; /* get pointers to the vectors */
    row = (dst->rowcnt > dst->colcnt) ? dst->rowcnt : dst->colcnt;
    while (--row >= 0) d[row] = s[row];
  }                             /* copy the associated vector */
  if (loc & MAT_WEIGHT) {       /* if to copy the matrix weight */
    loc &= ~MAT_WEIGHT;         /* remove the weight flag */
    dst->weight = src->weight;  /* copy the matrix weight */
  }
  row = dst->rowcnt;            /* get the number of rows */
  if      (loc == MAT_FULL) {   /* if to copy the full matrix */
    if (src->map) {             /* copy the permutation map */
      while (--row >= 0) dst->map[row] = src->map[row];
      dst->flags = (dst->flags & ~ODDPERM) | (src->flags & ODDPERM);
    }                           /* copy the odd permutation flag */
    for (row = dst->rowcnt; --row >= 0; ) {
      d = dst->els[row]; s = src->els[row];
      for (col = dst->colcnt; --col >= 0;   ) d[col] = s[col];
    } }                         /* copy rows column by column */
  else if (loc == MAT_UPPER) {  /* if to copy the upper triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      d = dst->els[row]; s = src->els[row];
      for (col = dst->colcnt; --col >= row; ) d[col] = s[col];
    } }                         /* copy rows column by column */
  else if (loc == MAT_LOWER) {  /* if to copy the lower triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      d = dst->els[row]; s = src->els[row];
      for (col = row+1;       --col >= 0;   ) d[col] = s[col];
    } }                         /* copy rows column by column */
  else if (loc == MAT_DIAG) {   /* if to copy the diagonal */
    while (--row >= 0)          /* copy diagonal elements */
      dst->els[row][row] = src->els[row][row]; }
  else if (loc == MAT_CORNER)   /* if to copy the top left corner */
    dst->els[0][0] = src->els[0][0];
  return dst;                   /* return the destination matrix */
}  /* mat_copyx() */

/*--------------------------------------------------------------------*/

double mat_diffx (const MATRIX *A, const MATRIX *B, int loc)
{                               /* --- compute max. elem. difference */
  DIMID        row, col;        /* loop variables */
  const double *sa, *sb;        /* to traverse the matrix rows */
  double       max = 0, t;      /* maximal difference, buffer */

  assert(A && B                 /* check the function arguments */
  &&    (A->rowcnt == B->rowcnt)
  &&    (A->colcnt == B->colcnt));
  assert(!(loc & (MAT_LOWER|MAT_UPPER))
  ||    (A->rowcnt == A->colcnt));
  if (loc & MAT_VECTOR) {       /* if to copy the associated vector */
    loc &= ~MAT_VECTOR;         /* and remove the vector flag */
    sa  = A->vec; sb = B->vec;  /* get pointers to the vectors */
    row = (A->rowcnt > A->colcnt) ? A->rowcnt : A->colcnt;
    while (--row >= 0) {        /* traverse the vector */
      t = fabs(sa[row] - sb[row]); if (t > max) max = t; }
  }                             /* compare the vector elements */
  if (loc & MAT_WEIGHT) {       /* if to copy the matrix weight */
    loc &= ~MAT_WEIGHT;         /* remove the weight flag */
    t = fabs(A->weight -B->weight); if (t > max) max = t;
  }                             /* compute the weight difference */
  row = A->rowcnt;              /* get the number of rows */
  if      (loc == MAT_FULL) {   /* if to compare the full matrix */
    while (--row >= 0) {        /* traverse the matrix rows */
      sa = A->els[row]; sb = B->els[row];
      for (col = A->colcnt; --col >= 0; ) {
        t = fabs(sa[col] - sb[col]); if (t > max) max = t; }
    } }                         /* compute maximum element difference */
  else if (loc == MAT_UPPER) {  /* if to compare the upper triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      sa = A->els[row]; sb = B->els[row];
      for (col = A->colcnt; --col >= row; ) {
        t = fabs(sa[col] - sb[col]); if (t > max) max = t; }
    } }                         /* compute maximum element difference */
  else if (loc == MAT_LOWER) {  /* if to compare the lower triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      sa = A->els[row]; sb = B->els[row];
      for (col = row+1; --col >= 0; ) {
        t = fabs(sa[col] - sb[col]); if (t > max) max = t; }
    } }                         /* compute maximum element difference */
  else if (loc == MAT_DIAG) {   /* if to compare the diagonal */
    while (--row >= 0) {        /* traverse the matrix rows */
      t = fabs(A->els[row][row] - B->els[row][row]);
      if (t > max) max = t;     /* compute the element differences */
    } }                         /* and determine their maximum */
  else if (loc == MAT_CORNER)   /* if to compare the top left corner */
    return fabs(A->els[0][0] - B->els[0][0]);
  return max;                   /* return the maximal difference */
}  /* mat_diffx() */

/*----------------------------------------------------------------------
  Matrix Initialization Functions
----------------------------------------------------------------------*/

void mat_init (MATRIX *mat, int mode, const double *vals)
{                               /* --- initialize a matrix */
  DIMID  row, col, n;           /* loop variables, buffer */
  double *d;                    /* to traverse the matrix rows */
  double t = 0;                 /* buffer for initialization value */

  assert(mat);                  /* check the function argument */
  mat->flags &= ROWBLKS;        /* clear all special flags */
  if (mode & MAT_VALUE) {       /* if to initialize with a value */
    mode &= ~MAT_VALUE;         /* remove the value flag */
    if (vals) { t = *vals; vals = NULL; }
  }                             /* get the initialization value */
  if (mode & MAT_VECTOR) {      /* if to init. the associated vector */
    mode &= ~MAT_VECTOR;        /* remove the vector flag */
    row = (mat->rowcnt > mat->colcnt) ? mat->rowcnt : mat->colcnt;
    for (d = mat->vec; --row >= 0; ) *d++ = t;
  }                             /* init. the associated vector */
  if (mode & MAT_WEIGHT) {      /* if to initialize the weight, */
    mode &= ~MAT_WEIGHT;        /* remove the weight flag */
    mat->weight = t;            /* init. the matrix/vector weight */
  }
  switch (mode) {               /* evaluate the initialization mode */
    case MAT_ZERO:              /* set matrix to zero */
    case MAT_UNIT:              /* set a unit matrix */
      for (row = mat->rowcnt; --row >= 0; )
        memset(mat->els[row], 0, (size_t)mat->colcnt *sizeof(double));
      if (mode == MAT_UNIT) {   /* if to set a unit matrix */
        assert(mat->rowcnt == mat->colcnt);
        for (row = 0; row < mat->rowcnt; row++)
          mat->els[row][row] = 1;
      } break;                  /* set the diagonal elements */
    case MAT_FULL:              /* set full matrix to given values */
      for (row = 0; row < mat->rowcnt; row++) {
        d = mat->els[row];      /* traverse the matrix rows */
        n = mat->colcnt; col = -1;
        if (vals) { while (++col < n) *d++ = *vals++; }
        else      { while (++col < n) *d++ = t; }
      } break;                  /* copy the given values */
    case MAT_UPPER:             /* set upper triangular matrix */
      assert(mat->rowcnt == mat->colcnt);
      for (row = 0; row < mat->rowcnt; row++) {
        d = mat->els[row] +row; /* traverse the matrix rows */
        n = mat->colcnt; col = row-1;
        if (vals) { while (++col < n) *d++ = *vals++; }
        else      { while (++col < n) *d++ = t; }
      } break;                  /* copy the given values */
    case MAT_LOWER:             /* set lower triangular matrix */
      assert(mat->rowcnt == mat->colcnt);
      for (row = 0; row < mat->rowcnt; row++) {
        d = mat->els[row];      /* traverse the matrix rows */
        n = row+1; col = -1;    /* traverse the left columns */
        if (vals) { while (++col < n) *d++ = *vals++; }
        else      { while (++col < n) *d++ = t; }
      } break;                  /* copy the given values */
    case MAT_DIAG:              /* set matrix diagonal */
      assert(mat->rowcnt == mat->colcnt);
      n = mat->colcnt; row = -1;
      if (vals) { while (++row < n) mat->els[row][row] = *vals++; }
      else      { while (++row < n) mat->els[row][row] = t; }
      break;                    /* copy the values to the diagonal */
    case MAT_CORNER:            /* set only matrix corner */
      mat->els[0][0] = (vals) ? *vals : t;
      break;                    /* copy value to top left corner */
  }  /* switch (mode) ... */
}  /* mat_init() */

/*--------------------------------------------------------------------*/

void mat_crop (MATRIX *mat, int loc)
{                               /* --- crop to triangular/diagonal */
  DIMID  row, col;              /* loop variables */
  double *d;                    /* to traverse the matrix rows */

  assert(mat                    /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  if (loc & MAT_UPPER) {        /* if to crop to upper triangle */
    for (row = mat->rowcnt; --row > 0; )
      for (d = mat->els[row], col = row;         --col >= 0; )
        d[col] = 0;             /* clear the lower triangle */
  }                             /* (below the diagonal) */
  if (loc & MAT_LOWER) {        /* if to crop to lower triangle */
    for (row = mat->rowcnt-1; --row >= 0; )
      for (d = mat->els[row], col = mat->colcnt; --col > row; )
        d[col] = 0;             /* clear the upper triangle */
  }                             /* (above the diagonal) */
}  /* mat_crop() */

/*----------------------------------------------------------------------
  Matrix Row Operations
----------------------------------------------------------------------*/

void mat_rowinit (MATRIX *mat, DIMID row, DIMID unit)
{                               /* --- initialize a row vector */
  DIMID  col;                   /* loop variable */
  double *d;                    /* matrix row to initialize */

  assert(mat                    /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt) && (unit < mat->colcnt));
  for (d = mat->els[row], col = mat->colcnt; --col >= 0; )
    d[col] = 0;                 /* clear the row vector and */
  if (unit >= 0) d[unit] = 1;   /* set a null vector or a unit vector */
}  /* mat_rowinit() */

/*--------------------------------------------------------------------*/

double* mat_rowget (const MATRIX *mat, DIMID row, double *vec)
{                               /* --- get a row vector */
  DIMID        col;             /* loop variable */
  const double *s;              /* matrix row to get */

  assert(mat && vec             /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt));
  s = mat->els[row];            /* copy from the matrix row */
  for (col = mat->colcnt; --col >= 0; ) vec[col] = s[col];
  return vec;                   /* return the filled vector */
}  /* mat_rowget() */

/*--------------------------------------------------------------------*/

void mat_rowset (MATRIX *mat, DIMID row, const double *vec)
{                               /* --- set a row vector */
  DIMID  col;                   /* loop variable */
  double *d;                    /* matrix row to set */

  assert(mat && vec             /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt));
  d = mat->els[row];            /* copy to the matrix row */
  for (col = mat->colcnt; --col >= 0; ) d[col] = vec[col];
}  /* mat_rowset() */

/*--------------------------------------------------------------------*/

double mat_rowsqr (const MATRIX *mat, DIMID row)
{                               /* --- compute squared row length */
  DIMID  col;                   /* loop variable */
  double sum = 0;               /* sum of squared elements */
  double *p;                    /* matrix row to process */

  assert(mat                    /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt));
  p = mat->els[row];            /* sum the row element squares */
  for (col = mat->colcnt; --col >= 0; ) sum += p[col] *p[col];
  return sum;                   /* compute and return the length */
}  /* mat_rowsqr() */

/*--------------------------------------------------------------------*/

void mat_rowaddv (MATRIX *mat, DIMID row, double k, const double *vec)
{                               /* --- add a vector to a row */
  DIMID  col;                   /* loop variable */
  double *d;                    /* matrix row to add to */

  assert(mat && vec             /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt));
  d   = mat->els[row];          /* get the destination row */
  col = mat->colcnt;            /* add the vector to the row */
  if      (k == +1) while (--col >= 0) d[col] +=    vec[col];
  else if (k == -1) while (--col >= 0) d[col] -=    vec[col];
  else              while (--col >= 0) d[col] += k *vec[col];
}  /* mat_rowaddv() */

/*--------------------------------------------------------------------*/

void mat_rowmuls (MATRIX *mat, DIMID row, double k)
{                               /* --- mul. matrix row with scalar */
  DIMID  col;                   /* loop variable */
  double *d;                    /* matrix row to multiply */

  assert(mat                    /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt));
  d = mat->els[row];            /* multiply the row elements */
  for (col = mat->colcnt; --col >= 0; ) d[col] *= k;
}  /* mat_rowmuls() */

/*--------------------------------------------------------------------*/

double mat_rowmulv (const MATRIX *mat, DIMID row, const double *vec)
{                               /* --- multiply a row with a vector */
  DIMID  col;                   /* loop variable */
  double *p;                    /* matrix row to multiply with */
  double sum = 0;               /* sum of element products */

  assert(mat && vec             /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt));
  p = mat->els[row];            /* sum the element products */
  for (col = mat->colcnt; --col >= 0; ) sum += p[col] *vec[col];
  return sum;                   /* return the result */
}  /* mat_rowmulv() */

/*--------------------------------------------------------------------*/

void mat_rowexg (MATRIX *A, DIMID arow, MATRIX *B, DIMID brow)
{                               /* --- exchange two matrix rows */
  DIMID  col;                   /* loop variable */
  double *a, *b, x;             /* matrix rows, exchange buffer */

  assert(A && B                 /* check the function arguments */
  &&    (A->colcnt == B->colcnt)
  &&    (arow >= 0) && (arow < A->rowcnt)
  &&    (brow >= 0) && (brow < B->rowcnt));
  a = A->els[arow];             /* get the destination row */
  b = B->els[brow];             /* and the source row */
  if (A == B) {                 /* if within the same matrix, */
    A->els[arow] = b;           /* exchange the matrix rows */
    B->els[brow] = a; }         /* by assigning them exchanged */
  else {                        /* if between matrices */
    for (col = A->colcnt; --col >= 0; ) {
      x = a[col]; a[col] = b[col]; b[col] = x; }
  }                             /* exchange the row elements */
}  /* mat_rowexg() */

/*--------------------------------------------------------------------*/

void mat_shuffle (MATRIX *mat, double randfn(void))
{                               /* --- shuffle the rows of a matrix */
  DIMID  i, n;                  /* vector index, number of vectors */
  double **p, *vec;             /* to traverse the vectors, buffer */

  assert(mat && randfn);        /* check the function arguments */
  for (p = mat->els, n = mat->rowcnt; --n > 0; ) {
    i = (DIMID)((double)(n+1) *randfn());
    if      (i > n) i = n;      /* compute a random index in the */
    else if (i < 0) i = 0;      /* remaining vector section */
    vec = p[i]; p[i] = *p; *p++ = vec;
  }                             /* exchange first and i-th pattern */
}  /* mat_shuffle() */

/*----------------------------------------------------------------------
  Matrix Column Operations
----------------------------------------------------------------------*/

void mat_colinit (MATRIX *mat, DIMID col, DIMID unit)
{                               /* --- initialize a column vector */
  DIMID row;                    /* loop variable */

  assert(mat                    /* check the function arguments */
  &&    (col >= 0) && (col < mat->colcnt) && (unit < mat->rowcnt));
  for (row = mat->rowcnt; --row >= 0; )
    mat->els[row][col] = 0;     /* clear the column vector */
  if (unit >= 0) mat->els[unit][col] = 1;
}  /* mat_colinit() */          /* set a null vector or a unit vector */

/*--------------------------------------------------------------------*/

double* mat_colget (const MATRIX *mat, DIMID col, double *vec)
{                               /* --- get a column vector */
  DIMID row;                    /* loop variable */

  assert(mat && vec             /* check the function arguments */
  &&    (col >= 0) && (col < mat->colcnt));
  for (row = mat->rowcnt; --row >= 0; )
    vec[row] = mat->els[row][col];
  return vec;                   /* copy from the matrix column */
}  /* mat_colget() */           /* and return the filled vector */

/*--------------------------------------------------------------------*/

void mat_colset (MATRIX *mat, DIMID col, const double *vec)
{                               /* --- set a column vector */
  DIMID row;                    /* loop variable */

  assert(mat && vec             /* check the function arguments */
  &&    (col >= 0) && (col < mat->colcnt));
  for (row = mat->rowcnt; --row >= 0; )
    mat->els[row][col] = vec[row];
}  /* mat_colset() */           /* copy to the matrix column */

/*--------------------------------------------------------------------*/

double mat_colsqr (const MATRIX *mat, DIMID col)
{                               /* --- compute squared column length */
  DIMID  row;                   /* loop variable */
  double sum = 0, t;            /* sum of squared elements, buffer */

  assert(mat                    /* check the function arguments */
  &&    (col >= 0) && (col < mat->colcnt));
  for (row = mat->rowcnt; --row >= 0; ) {
    t = mat->els[row][col]; sum += t*t; }
  return sum;                   /* sum the element squares and */
}  /* mat_colsqr() */           /* return the squared length */

/*--------------------------------------------------------------------*/

void mat_colcopy (MATRIX *dst, DIMID dcol,
            const MATRIX *src, DIMID scol)
{                               /* --- copy a matrix column */
  DIMID row;                    /* loop variable */

  assert(dst && src             /* check the function arguments */
  &&    (dst->rowcnt == src->rowcnt)
  &&    (dcol >= 0) && (dcol < dst->colcnt)
  &&    (scol >= 0) && (scol < src->colcnt));
  for (row = dst->rowcnt; --row >= 0; )
    dst->els[row][dcol] = src->els[row][scol];
}  /* mat_colcopy() */

/*--------------------------------------------------------------------*/

void mat_coladdv (MATRIX *mat, DIMID col, double k, const double *vec)
{                               /* --- add a vector to a column */
  DIMID row;                    /* loop variable */

  assert(mat && vec             /* check the function arguments */
  &&    (col >= 0) && (col < mat->colcnt));
  row = mat->rowcnt;            /* get the number of rows */
  if      (k == +1)             /* if to add to the column */
    while (--row >= 0) mat->els[row][col] +=    vec[row];
  else if (k == -1)             /* if to subtract from the column */
    while (--row >= 0) mat->els[row][col] -=    vec[row];
  else                          /* if to add an arbitrary multiple */
    while (--row >= 0) mat->els[row][col] += k *vec[row];
}  /* mat_coladdv() */

/*--------------------------------------------------------------------*/

void mat_coladd (MATRIX *dst, DIMID dcol,
                 double k, const MATRIX *src, DIMID scol)
{                               /* --- add a column to another */
  DIMID row;                    /* loop variable */

  assert(dst && src             /* check the function arguments */
  &&    (dst->rowcnt == src->rowcnt)
  &&    (dcol >= 0) && (dcol < dst->colcnt)
  &&    (scol >= 0) && (scol < src->colcnt));
  row = dst->rowcnt;            /* get the number of rows */
  if      (k == +1)             /* if to add to the column */
    while (--row >= 0) dst->els[row][dcol] +=    src->els[row][scol];
  else if (k == -1)             /* if to subtract from the column */
    while (--row >= 0) dst->els[row][dcol] -=    src->els[row][scol];
  else                          /* if to add an arbitrary multiple */
    while (--row >= 0) dst->els[row][dcol] += k *src->els[row][scol];
}  /* mat_coladd() */

/*--------------------------------------------------------------------*/

void mat_colmuls (MATRIX *mat, DIMID col, double k)
{                               /* --- mul. matrix column with scalar */
  DIMID row;                    /* loop variable */

  assert(mat                    /* check the function arguments */
  &&    (col >= 0) && (col < mat->rowcnt));
  for (row = mat->rowcnt; --row >= 0; )
    mat->els[row][col] *= k;    /* multiply the column elements */
}  /* mat_colmuls() */

/*--------------------------------------------------------------------*/

double mat_colmulv (const MATRIX *mat, DIMID col, const double *vec)
{                               /* --- mul. a column with a vector */
  DIMID  row;                   /* loop variable */
  double sum = 0;               /* sum of element products */

  assert(mat && vec             /* check the function arguments */
  &&    (col >= 0) && (col < mat->colcnt));
  for (row = mat->colcnt; --row >= 0; )
    sum += mat->els[row][col] *vec[row];
  return sum;                   /* sum the element products */
}  /* mat_colmulv() */          /* and return the result */

/*--------------------------------------------------------------------*/

double mat_colmul (const MATRIX *A, DIMID acol,
                   const MATRIX *B, DIMID bcol)
{                               /* --- multiply two matrix columns */
  DIMID  row;                   /* loop variable */
  double sum = 0;               /* sum of element products */

  assert(A && B                 /* check the function arguments */
  &&    (A->rowcnt == B->rowcnt)
  &&    (acol >= 0) && (acol < A->colcnt)
  &&    (bcol >= 0) && (bcol < B->colcnt));
  for (row = A->colcnt; --row >= 0; )
    sum += A->els[row][acol] *B->els[row][bcol];
  return sum;                   /* sum the element products */
}  /* mat_colmul() */           /* and return the result */

/*--------------------------------------------------------------------*/

void mat_colexg (MATRIX *A, DIMID acol, MATRIX *B, DIMID bcol)
{                               /* --- exchange two matrix columns */
  DIMID  row;                   /* loop variable */
  double t, *a, *b;             /* exchange buffer, matrix elements */

  assert(A && B                 /* check the function arguments */
  &&    (A->rowcnt == B->rowcnt)
  &&    (acol >= 0) && (acol < A->colcnt)
  &&    (bcol >= 0) && (bcol < B->colcnt));
  for (row = A->rowcnt; --row >= 0; ) {
    a = A->els[row] +acol;      /* get pointers to */
    b = B->els[row] +bcol;      /* the matrix elements and */
    t = *a; *a = *b; *b = t;    /* exchange the elements */
  }
}  /* mat_colexg() */

/*----------------------------------------------------------------------
  Diagonal Operations
----------------------------------------------------------------------*/

void mat_diainit (MATRIX *mat, DIMID unit)
{                               /* --- initialize the diagonal */
  DIMID  i;                     /* loop variable */
  double v = (unit < -1) ? 1:0; /* initialization value */

  assert(mat                    /* check the function argument */
  &&    (mat->rowcnt == mat->colcnt) && (unit < mat->rowcnt));
  for (i = mat->rowcnt; --i >= 0; )
    mat->els[i][i] = v;         /* initialize the diagonal */
  if (unit >= 0) mat->els[unit][unit] = 1;
}  /* mat_diainit() */

/*--------------------------------------------------------------------*/

double* mat_diaget (const MATRIX *mat, double *vec)
{                               /* --- get the diagonal of a matrix */
  DIMID i;                      /* loop variable */

  assert(mat && vec             /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  for (i = mat->rowcnt; --i >= 0; )
    vec[i] = mat->els[i][i];    /* copy from the matrix diagonal */
  return vec;                   /* and return the filled vector */
}  /* mat_diaget() */

/*--------------------------------------------------------------------*/

void mat_diaset (MATRIX *mat, const double *vec)
{                               /* --- set the diagonal of a matrix */
  DIMID i;                      /* loop variable */

  assert(mat && vec             /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  for (i = mat->rowcnt; --i >= 0; )
    mat->els[i][i] = vec[i];    /* copy vector to the matrix diagonal */
}  /* mat_diaset() */

/*--------------------------------------------------------------------*/

void mat_diasetx (MATRIX *mat, double x)
{                               /* --- set the diagonal of a matrix */
  DIMID i;                      /* loop variable */

  assert(mat                    /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  for (i = mat->rowcnt; --i >= 0; )
    mat->els[i][i] = x;         /* set matrix diagonal to given value */
}  /* mat_diasetx() */

/*--------------------------------------------------------------------*/

void mat_diacopy (MATRIX *dst, const MATRIX *src)
{                               /* --- copy the diagonal of a matrix */
  DIMID i;                      /* loop variable */

  assert(dst && src             /* check the function arguments */
  &&    (dst->rowcnt == src->rowcnt)
  &&    (dst->colcnt == src->colcnt)
  &&    (dst->rowcnt == dst->colcnt));
  for (i = dst->rowcnt; --i >= 0; )
    dst->els[i][i] = src->els[i][i];
}  /* mat_diacopy() */          /* copy the matrix diagonal */

/*--------------------------------------------------------------------*/

void mat_diaadds (MATRIX *mat, double k)
{                               /* --- add a value to a diagonal */
  DIMID i;                      /* loop variable */

  assert(mat                    /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  for (i = mat->rowcnt; --i >= 0; )
    mat->els[i][i] += k;        /* add value to all diagonal elements */
}  /* mat_diaadds() */

/*--------------------------------------------------------------------*/

void mat_diaaddv (MATRIX *mat, double k, const double *vec)
{                               /* --- add a vector to a diagonal */
  DIMID i;                      /* loop variable */

  assert(mat && vec             /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  i = mat->rowcnt;              /* initialize the loop variable */
  if      (k == +1)             /* if to add to the diagonal */
    while (--i >= 0) mat->els[i][i] +=    vec[i];
  else if (k == -1)             /* if to subtract from the diagonal */
    while (--i >= 0) mat->els[i][i] -=    vec[i];
  else                          /* if to add an arbitrary multiple */
    while (--i >= 0) mat->els[i][i] += k *vec[i];
}  /* mat_diaaddv() */

/*--------------------------------------------------------------------*/

void mat_diaadd (MATRIX *dst, double k, const MATRIX *src)
{                               /* --- add matrix diagonals */
  DIMID i;                      /* loop variable */

  assert(dst && src             /* check the function arguments */
  &&    (dst->rowcnt == src->rowcnt)
  &&    (dst->colcnt == src->colcnt)
  &&    (dst->rowcnt == dst->colcnt));
  i = dst->rowcnt;              /* get the number of rows */
  if      (k == +1)             /* if to add to the diagonal */
    while (--i >= 0) dst->els[i][i] +=    src->els[i][i];
  else if (k == -1)             /* if to subtract from the diagonal */
    while (--i >= 0) dst->els[i][i] -=    src->els[i][i];
  else                          /* if to add an arbitrary multiple */
    while (--i >= 0) dst->els[i][i] += k *src->els[i][i];
}  /* mat_diaadd() */

/*--------------------------------------------------------------------*/

void mat_diamuls (MATRIX *mat, double k)
{                               /* --- multiply diag. with a scalar */
  DIMID i;                      /* loop variable */

  assert(mat                    /* check the function argument */
  &&    (mat->rowcnt == mat->colcnt));
  for (i = mat->rowcnt; --i >= 0; )
    mat->els[i][i] *= k;        /* multiply the diagonal with k */
}  /* mat_diamuls() */

/*--------------------------------------------------------------------*/

double mat_diasum (MATRIX *mat)
{                               /* --- sum of diagonal elements */
  DIMID  i;                     /* loop variable */
  double sum;                   /* sum of diagonal elements (trace) */

  assert(mat                    /* check the function argument */
  &&    (mat->rowcnt == mat->colcnt));
  for (sum = mat->els[0][0], i = mat->rowcnt; --i > 0; )
    sum += mat->els[i][i];      /* sum the diagonal elements */
  return sum;                   /* and return the computed sum */
}  /* mat_diasum() */

/*--------------------------------------------------------------------*/

double mat_diarec (MATRIX *mat)
{                               /* --- sum of reciprocal diag. elems. */
  DIMID  i;                     /* loop variable */
  double sum, t;                /* sum of reciprocal values, buffer */

  assert(mat                    /* check the function argument */
  &&    (mat->rowcnt == mat->colcnt));
  for (sum = 0, i = mat->rowcnt; --i >= 0; ) {
    t = mat->els[i][i];         /* get the diagonal element and */
    if (!(t != 0)) return INFINITY; /* check for non-zero value */
    sum += 1/t;                 /* sum the reciprocal values */
  }                             /* of the diagonal elements */
  return sum;                   /* return the computed sum */
}  /* mat_diarec() */

/*--------------------------------------------------------------------*/

double mat_dialog (MATRIX *mat)
{                               /* --- sum of logarithms of diag.els. */
  DIMID  i;                     /* loop variable */
  double sum, t;                /* sum of logarithms, buffer */

  assert(mat                    /* check the function argument */
  &&    (mat->rowcnt == mat->colcnt));
  for (sum = 0, i = mat->rowcnt; --i >= 0; ) {
    t = mat->els[i][i];         /* get the diagonal element and */
    if (!(t > 0)) return -INFINITY; /* check for positive value */
    sum += log(t);              /* sum the logarithms of */
  }                             /* the diagonal elements */
  return sum;                   /* return the computed sum */
}  /* mat_dialog() */

/*--------------------------------------------------------------------*/

double mat_diaprod (MATRIX *mat)
{                               /* --- product of diagonal elements */
  DIMID  i;                     /* loop variable */
  double prod;                  /* product of diagonal elements */

  assert(mat                    /* check the function argument */
  &&    (mat->rowcnt == mat->colcnt));
  prod = mat->els[0][0];        /* multiply the diagonal elements */
  for (i = mat->rowcnt; --i > 0; ) prod *= mat->els[i][i];
  return prod;                  /* return the computed product */
}  /* mat_diaprod() */

/*----------------------------------------------------------------------
  Matrix/Vector Operations
----------------------------------------------------------------------*/

double* mat_mulmv (double *res, MATRIX *mat, const double *vec)
{                               /* --- multiply matrix and vector */
  DIMID  row, col;              /* loop variables */
  double *src, *dst;            /* to traverse the matrix rows */

  assert(res && mat && vec);    /* check the function arguments */
  dst = (res != vec) ? res : mat->buf;
  for (row = mat->rowcnt; --row >= 0; ) {
    src = mat->els[row];        /* traverse the matrix rows */
    dst[row] = 0;               /* initialize the result */
    for (col = mat->colcnt; --col >= 0; )
      dst[row] += src[col] *vec[col];
  }                             /* multiply matrix and vector elems. */
  if (res == vec)               /* if result is in buffer, copy it */
    memcpy(res, dst, (size_t)mat->rowcnt *sizeof(double));
  return res;                   /* return the resulting vector */
}  /* mat_mulmv() */

/*--------------------------------------------------------------------*/

double* mat_mulvm (double *res, const double *vec, MATRIX *mat)
{                               /* --- multiply vector and matrix */
  DIMID  row, col;              /* loop variables */
  double *src, *dst;            /* to traverse the matrix rows */

  assert(res && mat && vec);    /* check the function arguments */
  dst = (res != vec) ? res : mat->buf;
  for (col = mat->colcnt; --col >= 0; )
    dst[col] = 0;               /* initialize the result */
  for (row = mat->rowcnt; --row >= 0; ) {
    src = mat->els[row];         /* traverse the matrix rows */
    for (col = mat->colcnt; --col >= 0; )
      dst[col] += vec[row] *src[col];
  }                             /* multiply matrix and vector elems. */
  if (res == vec)               /* if result is in buffer, copy it */
    memcpy(res, dst, (size_t)mat->rowcnt *sizeof(double));
  return res;                   /* return the resulting vector */
}  /* mat_mulvm() */

/*--------------------------------------------------------------------*/

double mat_mulvmv (const MATRIX *mat, const double *vec)
{                               /* --- compute vec^T * mat * vec */
  DIMID  row, col;              /* loop variables */
  double *p;                    /* to traverse the matrix rows */
  double t, res = 0;            /* result of multiplication */

  assert(mat && vec             /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  for (row = mat->rowcnt; --row >= 0; ) {
    p = mat->els[row];          /* traverse the matrix rows */
    for (t = 0, col = mat->colcnt; --col >= 0; )
      t += vec[col] *p[col];    /* mult. matrix from left to vector, */
    res += vec[row] *t;         /* then compute the scalar product */
  }                             /* of the result and the vector */
  return res;                   /* return vec * mat * vec */
}  /* mat_mulvmv() */

/*--------------------------------------------------------------------*/

double* mat_muldv (double *res, MATRIX *mat, const double *vec)
{                               /* --- multiply diagonal and vector */
  DIMID i;                      /* loop variable */

  assert(mat && vec             /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  for (i = mat->rowcnt; --i >= 0; )
    res[i] = vec[i] *mat->els[i][i];
  return res;                   /* return vec * diag */
}  /* mat_muldv() */

/*--------------------------------------------------------------------*/

double mat_mulvdv (const MATRIX *mat, const double *vec)
{                               /* --- compute vec * diag * vec */
  DIMID  i;                     /* loop variable */
  double res = 0;               /* result of multiplication */

  assert(mat && vec             /* check the function arguments */
  &&    (mat->rowcnt == mat->colcnt));
  for (i = mat->rowcnt; --i >= 0; )
    res += vec[i] *vec[i] *mat->els[i][i];
  return res;                   /* return vec * diag * vec */
}  /* mat_mulvdv() */

/*----------------------------------------------------------------------
  General Matrix Operations
----------------------------------------------------------------------*/

MATRIX* mat_transp (MATRIX *res, const MATRIX *mat)
{                               /* --- transpose a matrix */
  DIMID  row, col;              /* loop variables */
  double *s, *d;                /* to traverse the matrix rows */
  double t;                     /* exchange buffer */

  assert(res && mat             /* check the function arguments */
  &&    (res->rowcnt == mat->colcnt)
  &&    (res->colcnt == mat->rowcnt));
  if (res == mat) {             /* if the result is id. to the matrix */
    for (row = mat->rowcnt -1; --row >= 0; ) {
      for (s = res->els[row] +(col = mat->colcnt); --col > row; ) {
        d = res->els[col] +row; /* traverse the columns of the row */
        t = *--s; *s = *d; *d = t;
      }                         /* exchange elements so that they */
    } }                         /* are mirrored at the diagonal */
  else {                        /* if source and destination differ */
    for (col = mat->colcnt; --col >= 0; )
      for (s = res->els[col] +(row = mat->rowcnt); --row >= 0; )
        *--s = mat->els[row][col];
  }                             /* transpose the source columns */
  return res;                   /* return the transposed matrix */
}  /* mat_transp() */

/*--------------------------------------------------------------------*/

MATRIX* mat_mulsx (MATRIX *res, const MATRIX *mat, double k, int loc)
{                               /* --- multiply matrix with scalar */
  DIMID  row, col;              /* loop variables */
  double *d; const double *s;   /* to traverse the matrix rows */

  assert(res && mat             /* check the function arguments */
  &&    (res->rowcnt == mat->rowcnt)
  &&    (res->colcnt == mat->colcnt));
  assert(!(loc & (MAT_LOWER|MAT_UPPER))
  ||    (mat->rowcnt == mat->colcnt));
  if (loc & MAT_VECTOR) {       /* if to multiply the assoc. vector */
    loc &= ~MAT_VECTOR;         /* remove the vector flag */
    row = (res->rowcnt > res->colcnt) ? res->rowcnt : res->colcnt;
    s = mat->vec; d = res->vec; /* traverse the vector */
    if      (k == +1) while (--row >= 0) d[row] =    s[row];
    else if (k == -1) while (--row >= 0) d[row] =   -s[row];
    else              while (--row >= 0) d[row] = k *s[row];
  }                             /* multiply column by column */
  if (loc & MAT_WEIGHT) {       /* if to multiply the weight */
    loc &= ~MAT_WEIGHT;         /* remove the weight flag */
    res->weight = k *mat->weight;
  }                             /* multiply the matrix/vector weight */
  row = mat->rowcnt;            /* get the number of rows */
  if      (loc == MAT_FULL) {   /* if to multiply the full matrix */
    while (--row >= 0) {        /* traverse the matrix rows */
      d = res->els[row]; s = mat->els[row];
      col = mat->colcnt;        /* traverse the matrix rows */
      if      (k == +1) while (--col >= 0) d[col] =    s[col];
      else if (k == -1) while (--col >= 0) d[col] =   -s[col];
      else              while (--col >= 0) d[col] = k *s[col];
    } }                         /* multiply column by column */
  else if (loc == MAT_UPPER) {  /* if to multiply the upper triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      s = mat->els[row]; d = res->els[row];
      col = mat->colcnt;        /* end at the diagonal */
      if      (k == +1) while (--col >= row) d[col] =    s[col];
      else if (k == -1) while (--col >= row) d[col] =   -s[col];
      else              while (--col >= row) d[col] = k *s[col];
    } }                         /* multiply column by column */
  else if (loc == MAT_LOWER) {  /* if to multiply the lower triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      s = mat->els[row]; d = res->els[row];
      col = row+1;              /* start at the diagonal */
      if      (k == +1) while (--col >= 0) d[col] =    s[col];
      else if (k == -1) while (--col >= 0) d[col] =   -s[col];
      else              while (--col >= 0) d[col] = k *s[col];
    } }                         /* multiply column by column */
  else if (loc == MAT_DIAG) {   /* if to multiply the diagonal */
    if      (k == +1)           /* if to copy the matrix */
      while (--row >= 0) res->els[row][row] =    mat->els[row][row];
    else if (k == -1)           /* if to negate the matrix */
      while (--row >= 0) res->els[row][row] =   -mat->els[row][row];
    else                        /* if to multiply with arb. factor */
      while (--row >= 0) res->els[row][row] = k *mat->els[row][row]; }
  else if (loc == MAT_CORNER)   /* if to multiply the top left corner */
    res->els[0][0] = k *mat->els[0][0];
  return res;                   /* return the result matrix */
}  /* mat_mulsx() */

/*--------------------------------------------------------------------*/

MATRIX* mat_addx (MATRIX *res, const MATRIX *A,
                  double k,    const MATRIX *B, int loc)
{                               /* --- add a multiple of a matrix */
  DIMID        row, col;        /* loop variables */
  const double *sa, *sb;        /* to traverse the source matrix rows */
  double       *d;              /* to traverse the dest.  matrix rows */

  assert(res && A && B          /* check the function arguments */
  &&    (A->rowcnt == B->rowcnt)   && (A->colcnt == B->colcnt)
  &&    (A->rowcnt == res->rowcnt) && (A->colcnt == res->colcnt));
  if (loc & MAT_VECTOR) {       /* if to add the associated vector */
    loc &= ~MAT_VECTOR;         /* remove the vector flag */
    row = (res->rowcnt > res->colcnt) ? res->rowcnt : res->colcnt;
    if      (k == +1)           /* if to add the vectors */
      while (--row >= 0) res->vec[row] = A->vec[row] +   B->vec[row];
    else if (k == -1)           /* if to subtract the vectors */
      while (--row >= 0) res->vec[row] = A->vec[row] -   B->vec[row];
    else                        /* if to add an arbitrary multiple */
      while (--row >= 0) res->vec[row] = A->vec[row] +k *B->vec[row];
  }                             /* add the associated vector */
  if (loc & MAT_WEIGHT) {       /* if to copy the weight */
    loc &= ~MAT_WEIGHT;         /* remove the weight flag */
    res->weight = A->weight +k *B->weight;
  }                             /* copy the vector weight sum */
  row = A->rowcnt;              /* get the number of rows */
  if      (loc == MAT_FULL) {   /* if to add the full matrix */
    while (--row >= 0) {        /* traverse the matrix rows */
      d   = res->els[row]; sa = A->els[row]; sb = B->els[row];
      col = res->colcnt;        /* get the number of columns */
      if      (k == +1)         /* if to add the matrices */
        while (--col >= 0) d[col] = sa[col] +   sb[col];
      else if (k == -1)         /* if to subtract the matrices */
        while (--col >= 0) d[col] = sa[col] -   sb[col];
      else                      /* if to add an arbitrary multiple */
        while (--col >= 0) d[col] = sa[col] +k *sb[col];
    } }                         /* add rows column by column */
  else if (loc == MAT_UPPER) {  /* if to add only the upper triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      d   = res->els[row]; sa = A->els[row]; sb = B->els[row];
      col = res->colcnt;        /* end at the diagonal */
      if      (k == +1)         /* if to add the matrices */
        while (--col >= row) d[col] = sa[col] +   sb[col];
      else if (k == -1)         /* if to subtract the matrices */
        while (--col >= row) d[col] = sa[col] -   sb[col];
      else                      /* if to add an arbitrary multiple */
        while (--col >= row) d[col] = sa[col] +k *sb[col];
    } }                         /* add rows column by column */
  else if (loc == MAT_LOWER) {  /* if to add only the lower triangle */
    while (--row >= 0) {        /* traverse the matrix rows */
      d   = res->els[row]; sa = A->els[row]; sb = B->els[row];
      col = row+1;              /* start at the diagonal */
      if      (k == +1)         /* if to add the matrices */
        while (--col >= 0) d[col] = sa[col] +   sb[col];
      else if (k == -1)         /* if to subtract the matrices */
        while (--col >= 0) d[col] = sa[col] -   sb[col];
      else                      /* if to add an arbitrary multiple */
        while (--col >= 0) d[col] = sa[col] +k *sb[col];
    } }                         /* add rows column by column */
  else if (loc == MAT_DIAG) {   /* if to add only the diagonals */
    if      (k == +1) {         /* if to add the matrices */
      while (--row >= 0)        /* traverse the diagonal elements */
        res->els[row][row] = A->els[row][row] +   B->els[row][row]; }
    else if (k == -1) {         /* if to subtract the matrices */
      while (--row >= 0)        /* traverse the diagonal elements */
        res->els[row][row] = A->els[row][row] -   B->els[row][row]; }
    else {                      /* if to add an arbitrary multiple */
      while (--row >= 0)        /* traverse the diagonal elements */
        res->els[row][row] = A->els[row][row] +k *B->els[row][row];
    } }                         /* add diagonal row by row */
  else if (loc == MAT_CORNER)   /* if to add only the top left corner */
    res->els[0][0] = A->els[0][0] +k *B->els[0][0];
  return res;                   /* return the result matrix */
}  /* mat_addx() */

/*--------------------------------------------------------------------*/

MATRIX* mat_mul (MATRIX *res, const MATRIX *A, const MATRIX *B)
{                               /* --- multiply two matrices */
  DIMID        row, col, i;     /* loop variables */
  const double *sa, *sb;        /* to traverse the source      rows */
  double       *d;              /* to traverse the destination rows */
  double       *b;              /* to traverse the buffer */
  double       t;               /* temporary buffer */

  assert(res && A && B          /* check the function arguments */
  &&    (A->colcnt   == B->rowcnt)
  &&    (res->rowcnt == A->rowcnt)
  &&    (res->colcnt == B->colcnt));
  if      (res == A) {          /* if matrix A is id. to the result, */
    b = res->buf;               /* get the (row) buffer of matrix A */
    for (row = res->rowcnt; --row >= 0; ) {
      d = res->els[row];        /* traverse the rows of matrix A */
      for (col = A->colcnt; --col >= 0; ) {
        b[col] = d[col];        /* buffer the current row of matrix A */
        d[col] = 0;             /* (because it will be overwritten) */
      }                         /* and initialize a row of the result */
      for (i = B->rowcnt; --i >= 0; ) {
        t = b[i]; sb = B->els[i];
        for (col = B->colcnt; --col >= 0; )
          d[col] += t *sb[col]; /* multiply the row of matrix A */
      }                         /* with the matrix B and store the */
    } }                         /* result in the row of matrix A */
  else if (res == B) {          /* if matrix B is id. to the result, */
    b = res->buf;               /* get the (row) buffer of matrix B */
    for (col = B->colcnt; --col >= 0; ) {
      for (i = B->rowcnt; --i >= 0; )
        b[i] = B->els[i][col];  /* buffer the column of matrix B */
      for (row = A->rowcnt; --row >= 0; ) {
        sa = A->els[row];       /* traverse the rows of matrix A */
        d  = res->els[row]+col; /* get the element to compute */
        *d = 0;                 /* and initialize the result */
        for (i = A->colcnt; --i >= 0; )
          *d += sa[i] *b[i];    /* multiply the matrix A with the */
      }                         /* column of matrix B and store the */
    } }                         /* result in the column of matrix B */
  else {                        /* if the result differs from both */
    for (row = A->rowcnt; --row >= 0; ) {
      sa = A->els[row];         /* traverse the rows    of matrix A */
      d  = res->els[row];       /* traverse the columns of matrix B */
      for (col = B->colcnt; --col >= 0; )
        d[col] = 0;             /* initialize the result */
      for (i = B->rowcnt; --i >= 0; ) {
        t = sa[i]; sb = B->els[i];
        for (col = B->colcnt; --col >= 0; )
          d[col] += t *sb[col]; /* multiply the row of matrix A with */
      }                         /* the matrix B and store the result */
    }                           /* in the corr. row of the matrix res */
  }
  return res;                   /* return the resulting matrix */
}  /* mat_mul() */

/*--------------------------------------------------------------------*/

MATRIX* mat_mulmdm (MATRIX *res, const MATRIX *mat, const double *diag)
{                               /* --- compute mat * diag * mat^T */
  DIMID  row, col, k;           /* loop variables */
  double *src, *tps, *dst;      /* to traverse the matrix rows */

  assert(res && mat             /* check the function arguments */
  &&    (res->rowcnt == mat->rowcnt)
  &&    (res->colcnt == mat->rowcnt));
  for (row = res->rowcnt; --row >= 0; ) {
    src = mat->els[row];        /* traverse the matrix rows */
    dst = (res != mat) ? res->els[row] : res->buf;
    for (col = row+1; --col >= 0; ) {
      tps = mat->els[col];      /* traverse the matrix columns */
      dst[col] = 0; k = mat->colcnt;
      if (diag) while (--k >= 0) dst[col] += src[k] *diag[k] *tps[k];
      else      while (--k >= 0) dst[col] += src[k]          *tps[k];
    }                           /* compute the result elements */
    if (res == mat)             /* if to work in place */
      for (col = row+1; --col >= 0; )
        src[col] = dst[col];    /* copy the computed row */
  }                             /* into the (result) matrix */
  for (row = res->rowcnt; --row >= 0; ) {
    dst = res->els[row];        /* traverse the matrix rows */
    for (col = res->colcnt; --col > row; )
      dst[col] = res->els[col][row];
  }                             /* make the result matrix symmetric */
  return res;                   /* return mat * diag * mat^T */
}  /* mat_mulmdm() */

/*--------------------------------------------------------------------*/

MATRIX* mat_sub (MATRIX *res, const MATRIX *mat, DIMID row, DIMID col)
{                               /* --- cut out a submatrix */
  DIMID  i, k;                  /* loop variables */
  double *d; const double *s;   /* to traverse the matrix rows */

  assert(res && mat             /* check the function arguments */
  &&    (row >= 0) && (row < mat->rowcnt -res->rowcnt)
  &&    (col >= 0) && (row < mat->colcnt -res->colcnt));
  for (i = res->rowcnt; --i >= 0; ) {
    s = mat->els[i +row] +col; d = res->els[i];
    for (k = res->colcnt; --k >= 0; ) d[k] = s[k];
  }                             /* copy a rect. part of the matrix */
  return res;                   /* return the created submatrix */
}  /* mat_sub() */

/*--------------------------------------------------------------------*/

MATRIX* mat_subx (MATRIX *res, const MATRIX *mat,
                  DIMID *rowids, DIMID *colids)
{                               /* --- cut out a submatrix (extended) */
  DIMID  i, k;                  /* loop variables */
  double *d; const double *s;   /* to traverse the matrix rows */

  assert(rowids && colids       /* check the function arguments */
  &&     res && mat && (res != mat)
  &&    (mat->rowcnt >= res->rowcnt)
  &&    (mat->colcnt >= res->colcnt));
  for (i = res->rowcnt; --i >= 0; ) {
    s = mat->els[rowids[i]]; d = res->els[i];
    for (k = res->colcnt; --k >= 0; ) d[k] = s[colids[k]];
  }                             /* copy a part of the matrix */
  return res;                   /* return the created submatrix */
}  /* mat_subx() */

/*--------------------------------------------------------------------*/

double mat_sqrnorm (const MATRIX *mat)
{                               /* --- compute Frobenius norm */
  DIMID  row, col;              /* loop variables */
  double n = 0;                 /* value of the norm */
  const double *p;              /* to traverse the matrix rows */

  assert(mat);                  /* check the function argument */
  for (row = mat->rowcnt; --row >= 0; ) {
    p = mat->els[row];          /* traverse the matrix rows */
    for (col = mat->colcnt; --col >= 0; ) n += p[col] *p[col];
  }                             /* sum the squared matrix elements */
  return n;                     /* return the computed sum */
}  /* mat_sqrnorm() */

/*--------------------------------------------------------------------*/

double mat_max (const MATRIX *mat)
{                               /* --- compute maximum norm */
  DIMID  row, col;              /* loop variables */
  double max = 0;               /* maximal matrix element */
  const double *p;              /* to traverse the matrix rows */

  assert(mat);                  /* check the function argument */
  for (row = mat->rowcnt; --row >= 0; ) {
    p = mat->els[row];          /* traverse the matrix rows */
    for (col = mat->colcnt; --col >= 0; )
      if (p[col] > max) max = p[col];
  }                             /* sum the squared matrix elements */
  return max;                   /* return the maximum element */
}  /* mat_max() */

/*----------------------------------------------------------------------
  Matrix Input/Output Functions
----------------------------------------------------------------------*/
#ifdef MAT_RDWR

int mat_write (const MATRIX *mat, FILE *file, int digs, const char *sep)
{                               /* --- write a matrix to a file */
  DIMID        row, col;        /* loop variables */
  const double *p;              /* to traverse the matrix rows */

  assert(mat && file && sep);   /* check the function arguments */
  for (row = 0; row < mat->rowcnt; row++) {
    p = mat->els[row];          /* traverse the matrix rows */
    for (col = 0; col < mat->colcnt; col++) {
      if (col > 0) fputc(sep[0], file);
      fprintf(file, "%-.*g", digs, p[col]);
    }                           /* print an element separator */
    fputs(sep +1, file);        /* and a matrix element; after */
  }                             /* each row print a row separator */
  return ferror(file) ? -1 : 0; /* check for an error */
}  /* mat_write() */

/*--------------------------------------------------------------------*/

int mat_read (MATRIX *mat, TABREAD *tread)
{                               /* --- read a matrix from a file */
  int   r;                      /* result of vec_read() */
  DIMID i;                      /* loop variable */

  assert(mat && tread);         /* check the function arguments */
  for (i = 0, r = 0; (r == 0) && (i < mat->rowcnt); i++)
    r = vec_read(mat->els[i], mat->colcnt, tread);
  if (r < 0) return r;          /* read the matrix rows and */
  return (r) ? 0 : E_RECCNT;    /* return error code or 'ok' */
}  /* mat_read() */

/*--------------------------------------------------------------------*/

int mat_readx (MATRIX **mat, TABREAD *tread, DIMID rowcnt, DIMID colcnt)
{                               /* --- read a matrix from a file */
  void   *p;                    /* buffer for reallocation */
  double *vec;                  /* for reading a row vector */
  DIMID  n;                     /* size of the row array */
  int    r;                     /* result of vec_readx() */

  assert(tread);                /* check the function arguments */
  if (colcnt < 0) colcnt = 0;   /* eliminate negative values */
  n    = (rowcnt > 0) ? rowcnt : BLKSIZE;
  *mat = (MATRIX*)malloc(sizeof(MATRIX) +(size_t)(n-1)*sizeof(double*));
  if (!*mat) return E_NOMEM;    /* create a matrix and */
  (*mat)->flags  = ROWBLKS;     /* initialize some fields */
  (*mat)->rowcnt = 0;           /* (for a proper cleanup on error) */
  (*mat)->vec    = NULL;
  while (1) {                   /* row vector read loop */
    r = vec_readx(&vec, &colcnt, tread);
    if (r < 0) MATERR(r, mat);  /* read the next vector and check */
    if (r > 0) break;           /* for an error and end of file */
    if ((*mat)->rowcnt >= n) {  /* if the row vector is full */
      if (rowcnt > 0) MATERR(E_RECCNT, mat);
      n += (n > BLKSIZE) ? n >> 1 : BLKSIZE;
      p  = realloc(*mat, sizeof(MATRIX) +(size_t)(n-1)*sizeof(double*));
      if (!p) MATERR(E_NOMEM, mat);
      *mat = (MATRIX*)p;        /* enlarge the matrix structure */
    }                           /* and set the new matrix */
    (*mat)->els[(*mat)->rowcnt++] = vec;
  }                             /* store the pattern read */
  (*mat)->colcnt = colcnt;      /* set the number of columns */
  if (rowcnt > (*mat)->rowcnt)  /* check the number of rows */
    MATERR(E_RECCNT, mat);      /* (no more than requested) */
  n = (colcnt > (*mat)->rowcnt) ? colcnt : (*mat)->rowcnt;
  p = realloc(*mat, sizeof(MATRIX)
              +(size_t)((*mat)->rowcnt-1) *sizeof(double*)
              +(size_t)(n+n)              *sizeof(DIMID));
  if (!p) MATERR(E_NOMEM, mat); /* enlarge the matrix and set and */
  *mat = (MATRIX*)p;            /* organize the new memory block */
  (*mat)->map = (DIMID*)((*mat)->els +(*mat)->rowcnt);
  (*mat)->vec = (double*)malloc((size_t)(n+n) *sizeof(double));
  if (!(*mat)->vec) MATERR(E_NOMEM, mat);
  (*mat)->buf = (*mat)->vec +n; /* allocate a vector buffer */
  return 0;                     /* return 'ok' */
}  /* mat_readx() */

#endif
