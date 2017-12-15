/*----------------------------------------------------------------------
  File    : matrix.h
  Contents: general vector and matrix management
  Author  : Christian Borgelt
  History : 1999.04.30 file created
            1999.05.15 diagonal element functions added
            1999.05.17 parameter 'det' added to function mat_gjinv
            1999.05.18 function 'mat_tri2sym' added
            2001.08.17 VECTOR data type replaced by double*
            2001.10.13 covariance funcs. redesigned, mat_regress added
            2001.10.15 vec_show, mat_show made macros, mat_sub added
            2001.10.23 functions mat_addsv and mat_var added
            2001.10.25 function mat_subx added
            2001.10.26 interface of function mat_regress changed
            2001.11.07 arg. loc added to mat_clone, mat_copy, mat_diff
            2001.11.09 functions mat_diasum, mat_diaprod, mat_trace add.
            2001.11.10 functions vec_sqrlen, vec_sqrdist, vec_dist added
            2001.11.11 function mat_mulvdv added
            2002.09.09 initialization mode MAT_VALUE added
            2004.02.09 return value of mat_trinv changed to int
            2004.02.16 function mat_inc added (increase matrix element)
            2004.02.23 function mat_isovar added (isotropic variance)
            2004.03.01 function mat_diaadds added, MAT_VECTOR added
            2004.03.22 extended (co)variance functions added
            2004.04.23 functions mat_dialog and mat_chlogd added
            2004.04.30 function mat_addx added (with location parameter)
            2004.08.16 function mat_addwgt added
            2004.08.18 functions mat_muldv and mat_mulvd added
            2007.11.22 function mat_evsort added (eigenvalue sorting)
            2007.11.23 functions mat_mulmmt and mat_mulmvmt added
            2007.12.01 function vec_unitlen added
            2007.12.03 several functions added and others extended
            2008.02.28 some vector functions added (e.g. vec_sum)
            2008.03.27 semantics of associated vector simplified
            2008.04.25 use of matrix locations made more consistent
            2010.10.11 adapted to modified module tabread
            2010.11.10 interface of vec_readx() and mat_readx() adapted
            2011.02.04 function mat_negate() added (negate elements)
            2013.08.13 preprocessor definition of type DIMID added
----------------------------------------------------------------------*/
#ifndef __MATRIX__
#define __MATRIX__
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef MAT_RDWR
#include "tabread.h"
#endif

#ifdef _MSC_VER
#include <float.h>
#ifndef INFINITY
#define INFINITY     (DBL_MAX+DBL_MAX)
#endif
#ifndef NAN
#define NAN          (INFINITY-INFINITY)
#endif
#ifndef isnan
#define isnan(x)     _isnan(x)
#endif
#ifndef isinf
#define isinf(x)     (!_isnan(x) && !_finite(x))
#endif
#ifndef strtoll
#define strtoll      _strtoi64  /* string to long long int */
#endif
#endif                          /* MSC still does not support C99 */

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
#ifndef strtodim
#define strtodim(s,p)   (int)strtol(s,p,0)
#endif

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
#ifndef strtodim
#define strtodim(s,p)   strtol(s,p,0)
#endif

#elif DIMID==ptrdiff_t
#ifndef DIMID_MAX
#define DIMID_MAX   PTRDIFF_MAX /* maximum dimension identifier */
#endif
#ifndef DIMID_SGN
#define DIMID_SGN   PTRDIFF_MIN /* sign of dimension identifier */
#endif
#ifndef DIMID_FMT
#  ifdef _MSC_VER
#  define DIMID_FMT "Id"        /* printf format code for ptrdiff_t */
#  else
#  define DIMID_FMT "td"        /* printf format code for ptrdiff_t */
#  endif                        /* MSC still does not support C99 */
#endif
#ifndef strtodim
#define strtodim(s,p)   (ptrdiff_t)strtoll(s,p,0)
#endif

#else
#error "DIMID must be either 'int', 'long' or 'ptrdiff_t'"
#endif

/*--------------------------------------------------------------------*/

#undef int                      /* remove preprocessor definitions */
#undef long                     /* needed for the type checking */
#undef ptrdiff_t

/*--------------------------------------------------------------------*/

/* --- matrix locations --- */
#define MAT_FULL      0x0001    /* full matrix */
#define MAT_LOWER     0x0002    /* lower triangular matrix */
#define MAT_LEFT      0x0002    /* left  triangular matrix */
#define MAT_UPPER     0x0004    /* upper triangular matrix */
#define MAT_RIGHT     0x0004    /* right triangular matrix */
#define MAT_DIAG      0x0006    /* diagonal matrix */
#define MAT_CORNER    0x0008    /* upper left corner */
#define MAT_VECTOR    0x0010    /* vector buffer */
#define MAT_WEIGHT    0x0020    /* vector/matrix weight (sum) */

/* --- initialization modes --- */
#define MAT_VALUE     0x0040    /* set to a single given value */

/* --- special initialization modes --- */
#define MAT_UNIT      0x0100    /* set a unit matrix */
#define MAT_ZERO      0x0200    /* set matrix to zero */

/* --- mode flags --- */
#define MAT_PARTPIV   0x0000    /* do partial pivoting */
#define MAT_FULLPIV   0x0001    /* do full pivoting */
#define MAT_NOCOPY    0x8000    /* do not copy the input matrix */
#define MAT_INVERSE   0x4000    /* compute inverse matrix */

/* --- error codes --- */
#define E_NONE           0      /* no error */
#define E_NOMEM        (-1)     /* not enough memory */
#define E_FOPEN        (-2)     /* file open failed */
#define E_FREAD        (-3)     /* file read failed */
#define E_FWRITE       (-4)     /* file write failed */

#define E_VALUE       (-16)     /* invalid field value */
#define E_FLDCNT      (-17)     /* wrong number of fields */
#define E_RECCNT      (-18)     /* wrong number of records */

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- a matrix --- */
  DIMID  rowcnt;                /* number of rows */
  DIMID  colcnt;                /* number of columns */
  int    flags;                 /* flags, e.g. ROWBLKS, ODDPERM */
  DIMID  *map;                  /* row/column permutation map */
  double weight;                /* sum of vector weights */
  double *vec;                  /* associated vector, mean values */
  double *buf;                  /* buffer for internal computations */
  double *els[1];               /* matrix elements (in rows) */
} MATRIX;                       /* (matrix) */

/*----------------------------------------------------------------------
  Vector Functions
----------------------------------------------------------------------*/
/* --- basic functions --- */
extern double* vec_init    (double *vec, DIMID n, DIMID unit);
extern void    vec_set     (double *vec, DIMID n, double x);
extern double* vec_copy    (double *dst, const double *src,   DIMID n);
extern double  vec_diff    (const double *a, const double *b, DIMID n);
extern int     vec_cmp     (const double *a, const double *b, DIMID n);

/* --- vector element operations --- */
extern double  vec_sum     (const double *vec, DIMID n);
extern double  vec_sumrec  (const double *vec, DIMID n);
extern double  vec_sumlog  (const double *vec, DIMID n);
extern double  vec_prod    (const double *vec, DIMID n);
extern double  vec_max     (const double *vec, DIMID n);
extern double  vec_absmax  (const double *vec, DIMID n);

/* --- vector operations --- */
extern double  vec_sqrlen  (const double *vec, DIMID n);
extern double  vec_len     (const double *vec, DIMID n);
extern double* vec_unitlen (double *res, const double *vec, DIMID n);
extern double  vec_sqrdist (const double *a, const double *b, DIMID n);
extern double  vec_dist    (const double *a, const double *b, DIMID n);
extern double* vec_add     (double *res, DIMID n,
                            const double *a, double k, const double *b);
extern double* vec_muls    (double *res, DIMID n,
                            const double *vec, double k);
extern double  vec_mul     (const double *a, const double *b, DIMID n);
extern double  vec_smul    (const double *a, const double *b, DIMID n);
extern double  vec_sclmul  (const double *a, const double *b, DIMID n);
extern double* vec_vmul    (double *res, const double *a,
                                         const double *b);
extern double* vec_vecmul  (double *res, const double *a,
                                         const double *b);
extern MATRIX* vec_mmul    (MATRIX *res, const double *a,
                                         const double *b);
extern MATRIX* vec_matmul  (MATRIX *res, const double *a,
                                         const double *b);

/* --- input/output functions --- */
extern void    vec_show    (const double *vec, DIMID n);
extern int     vec_write   (const double *vec, DIMID n, FILE *file,
                            int digs, const char *seps);
#ifdef MAT_RDWR
extern int     vec_read    (double  *vec, DIMID  n, TABREAD *tread);
extern int     vec_readx   (double **vec, DIMID *n, TABREAD *tread);
#endif

/*----------------------------------------------------------------------
  Matrix Functions
----------------------------------------------------------------------*/
/* --- basic functions --- */
extern MATRIX* mat_create  (DIMID rowcnt, DIMID colcnt);
extern void    mat_delete  (MATRIX *mat);
extern MATRIX* mat_clone   (const MATRIX *mat);
extern MATRIX* mat_clonex  (const MATRIX *mat, int loc);
extern MATRIX* mat_copy    (MATRIX *dst, const MATRIX *src);
extern MATRIX* mat_copyx   (MATRIX *dst, const MATRIX *src,   int loc);
extern double  mat_diff    (const MATRIX *A, const MATRIX *B);
extern double  mat_diffx   (const MATRIX *A, const MATRIX *B, int loc);
extern int     mat_issqr   (const MATRIX *mat);
extern double* mat_vector  (MATRIX *mat);

/* --- initialization and access functions --- */
extern void    mat_init    (MATRIX *mat, int mode, const double *vals);
extern void    mat_crop    (MATRIX *mat, int loc);
extern double  mat_get     (const MATRIX *mat, DIMID row, DIMID col);
extern double  mat_set     (MATRIX *mat, DIMID row, DIMID col,
                            double val);
extern double  mat_inc     (MATRIX *mat, DIMID row, DIMID col,
                            double val);
extern double  mat_emul    (MATRIX *mat, DIMID row, DIMID col,
                            double val);

/* --- row operations --- */
extern DIMID   mat_rowcnt  (const MATRIX *mat);
extern void    mat_rowinit (MATRIX *mat, DIMID row, DIMID unit);
extern double* mat_row     (MATRIX *mat, DIMID row);
extern double* mat_rowget  (const MATRIX *mat, DIMID row, double *vec);
extern void    mat_rowset  (MATRIX *mat, DIMID row, const double *vec);
extern double  mat_rowsqr  (const MATRIX *mat, DIMID row);
extern double  mat_rowlen  (const MATRIX *mat, DIMID row);
extern void    mat_rowcopy (MATRIX *dst,       DIMID drow,
                            const MATRIX *src, DIMID srow);
extern void    mat_rowaddv (MATRIX *mat, DIMID row,
                            double k, const double *vec);
extern void    mat_rowadd  (MATRIX *dst, DIMID drow,
                            double k, const MATRIX *src, DIMID srow);
extern void    mat_rowmuls (MATRIX *mat, DIMID row,  double k);
extern double  mat_rowmulv (const MATRIX *mat, DIMID row,
                            const double *vec);
extern double  mat_rowmul  (const MATRIX *A, DIMID arow,
                            const MATRIX *B, DIMID brow);
extern void    mat_rowexg  (MATRIX *A, DIMID arow,
                            MATRIX *B, DIMID brow);
extern void    mat_shuffle (MATRIX *mat, double randfn(void));

/* --- column operations --- */
extern DIMID   mat_colcnt  (const MATRIX *mat);
extern void    mat_colinit (MATRIX *mat, DIMID col, DIMID unit);
extern double* mat_colget  (const MATRIX *mat, DIMID col, double *vec);
extern void    mat_colset  (MATRIX *mat, DIMID col, const double *vec);
extern double  mat_colsqr  (const MATRIX *mat, DIMID col);
extern double  mat_collen  (const MATRIX *mat, DIMID col);
extern void    mat_colcopy (MATRIX *dst,       DIMID dcol,
                            const MATRIX *src, DIMID scol);
extern void    mat_coladdv (MATRIX *mat, DIMID col,
                            double k, const double *vec);
extern void    mat_coladd  (MATRIX *mat, DIMID dcol,
                            double k, const MATRIX *src, DIMID scol);
extern void    mat_colmuls (MATRIX *mat, DIMID col,  double k);
extern double  mat_colmulv (const MATRIX *mat, DIMID col,
                            const double *vec);
extern double  mat_colmul  (const MATRIX *A, DIMID acol,
                            const MATRIX *B, DIMID bcol);
extern void    mat_colexg  (MATRIX *A, DIMID acol,
                            MATRIX *B, DIMID bcol);

/* --- diagonal operations --- */
extern void    mat_diainit (MATRIX *mat, DIMID unit);
extern double* mat_diaget  (const MATRIX *mat, double *vec);
extern void    mat_diaset  (MATRIX *mat, const double *vec);
extern void    mat_diasetx (MATRIX *mat, double x);
extern void    mat_diacopy (MATRIX *dst, const MATRIX *src);
extern void    mat_diaadds (MATRIX *mat, double k);
extern void    mat_diaaddv (MATRIX *mat, double k, const double *vec);
extern void    mat_diaadd  (MATRIX *dst, double k, const MATRIX *src);
extern void    mat_diamuls (MATRIX *mat, double k);
extern double  mat_diasum  (MATRIX *mat);
extern double  mat_trace   (MATRIX *mat);
extern double  mat_diarec  (MATRIX *mat);
extern double  mat_dialog  (MATRIX *mat);
extern double  mat_diaprod (MATRIX *mat);

/* --- matrix/vector operations --- */
extern double* mat_mulmv   (double *res, MATRIX *mat,const double *vec);
extern double* mat_mulvm   (double *res, const double *vec,MATRIX *mat);
extern double  mat_mulvmv  (const MATRIX *mat, const double *vec);
extern double  mat_mulvtmv (const MATRIX *mat, const double *vec);
extern double* mat_muldv   (double *res, MATRIX *mat,const double *vec);
extern double* mat_mulvd   (double *res, const double *vec,MATRIX *mat);
extern double  mat_mulvdv  (const MATRIX *mat, const double *vec);
extern double  mat_mulvtdv (const MATRIX *mat, const double *vec);

/* --- general matrix operations --- */
extern MATRIX* mat_transp  (MATRIX *res, const MATRIX *mat);
extern MATRIX* mat_muls    (MATRIX *res, const MATRIX *mat, double k);
extern MATRIX* mat_mulsx   (MATRIX *res, const MATRIX *mat,
                            double k, int loc);
extern MATRIX* mat_add     (MATRIX *res, const MATRIX *A,
                            double k,    const MATRIX *B);
extern MATRIX* mat_addx    (MATRIX *res, const MATRIX *A,
                            double k,    const MATRIX *B, int loc);
extern MATRIX* mat_mul     (MATRIX *res, const MATRIX *A,
                                         const MATRIX *B);
extern MATRIX* mat_mulmmt  (MATRIX *res, const MATRIX *mat);
extern MATRIX* mat_mulmdm  (MATRIX *res, const MATRIX *mat,
                            const double *diag);
extern MATRIX* mat_mulmdmt (MATRIX *res, const MATRIX *mat,
                            const double *diag);
extern MATRIX* mat_sub     (MATRIX *res, const MATRIX *mat,
                            DIMID row, DIMID col);
extern MATRIX* mat_subx    (MATRIX *res, const MATRIX *mat,
                            DIMID *rowids, DIMID *colids);
extern double  mat_sqrnorm (const MATRIX *mat);
extern double  mat_norm    (const MATRIX *mat);
extern double  mat_max     (const MATRIX *mat);

/* --- diagonal matrix operations --- */
extern int     mat_diainv  (MATRIX *res, const MATRIX *mat);

/* --- triangular matrix operations --- */
extern MATRIX* mat_tr2sym  (MATRIX *res, const MATRIX *mat, int src);
extern MATRIX* mat_trtrp   (MATRIX *res, const MATRIX *mat, int src);
extern double* mat_trsubst (double *res, const MATRIX *mat, int loc,
                            const double *vec);
extern int     mat_trinv   (MATRIX *res, const MATRIX *mat, int loc);

/* --- Gauss-Jordan elimination functions --- */
extern double  mat_gjdet   (MATRIX *mat, int mode);
extern int     mat_gjinv   (MATRIX *res, const MATRIX *mat,
                            int mode, double *pdet);
extern int     mat_gjsol   (double **res, MATRIX *mat,
                            double *const*vec, DIMID cnt,
                            int mode, double *pdet);

/* --- LU decomposition functions --- */
extern int     mat_ludecom (MATRIX *res, const MATRIX *mat);
extern double* mat_lusubst (double *res, const MATRIX *mat,
                            const double *vec);
extern double  mat_ludet   (const MATRIX *mat);
extern int     mat_luinv   (MATRIX *res, const MATRIX *mat);

/* --- Cholesky decomposition functions --- */
extern int     mat_chdecom (MATRIX *res, const MATRIX *mat);
extern double* mat_chsubst (double *res, const MATRIX *mat,
                            const double *vec);
extern double  mat_chdet   (const MATRIX *mat);
extern double  mat_chlogd  (const MATRIX *mat);
extern int     mat_chinv   (MATRIX *res, const MATRIX *mat);
extern int     mat_chinvx  (MATRIX *res, const MATRIX *mat);

/* --- eigenvalue and eigenvector functions --- */
extern int     mat_jacobi  (double *evs, MATRIX *res,
                            MATRIX *mat, int max);
extern void    mat_3dred   (double *dia, double *sub, MATRIX *otm,
                            MATRIX *mat, int loc);
extern int     mat_3dqli   (double *evs, MATRIX *mat,
                            const double *dia, double *sub,
                            DIMID n, DIMID max);
extern void    mat_evsort  (double *evs, MATRIX *mat, int dir);
extern void    mat_bal     (MATRIX *res, const MATRIX *mat);
extern void    mat_heselm  (MATRIX *res, const MATRIX *mat);
extern void    mat_hesqr   (double *real, double *img,
                            MATRIX *mat, int mode);

/* --- (co)variance/correlation/regression operations --- */
extern double  mat_addwgt  (MATRIX *mat, double wgt);
extern MATRIX* mat_addvec  (MATRIX *mat, const double *vec, double wgt);
extern MATRIX* mat_addsv   (MATRIX *mat, const double *vec, double wgt);
extern MATRIX* mat_addsvx  (MATRIX *mat, const double *vec, double wgt);
extern MATRIX* mat_addmp   (MATRIX *mat, const double *vec, double wgt);
extern MATRIX* mat_addmpx  (MATRIX *mat, const double *vec, double wgt);

extern double  mat_getwgt  (const MATRIX *mat);
extern double  mat_setwgt  (MATRIX *mat, double wgt);
extern double  mat_mulwgt  (MATRIX *mat, double factor);
extern double* mat_mean    (double *res, const MATRIX *mat);
extern MATRIX* mat_meanx   (MATRIX *res, const MATRIX *mat);
extern double  mat_isovar  (const MATRIX *mat, int mle);
extern double  mat_isovarm (double *res, const MATRIX *mat, int mle);
extern MATRIX* mat_isovarx (MATRIX *res, const MATRIX *mat, int mle);
extern MATRIX* mat_var     (MATRIX *res, const MATRIX *mat, int mle);
extern MATRIX* mat_varx    (MATRIX *res, const MATRIX *mat, int mle);
extern MATRIX* mat_covar   (MATRIX *res, const MATRIX *mat, int mle);
extern MATRIX* mat_covarx  (MATRIX *res, const MATRIX *mat, int mle);

extern MATRIX* mat_correl  (MATRIX *res, const MATRIX *mat);
extern double  mat_regress (MATRIX *res, double *rhs,const MATRIX *mat);

/* --- special purpose functions --- */
extern double  mat_match   (MATRIX *mat, DIMID *map);
extern double  mat_negate  (MATRIX *mat, double zero);

/* --- input/output functions --- */
extern void    mat_show    (const MATRIX *mat);
extern int     mat_write   (const MATRIX *mat, FILE *file,
                            int digs, const char *seps);
#ifdef MAT_RDWR
extern int     mat_read    (MATRIX  *mat, TABREAD *tread);
extern int     mat_readx   (MATRIX **mat, TABREAD *tread,
                            DIMID rowcnt, DIMID colcnt);
#endif
/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define vec_copy(d,s,n)        memcpy(d,s,(size_t)(n)*sizeof(double))
#define vec_len(a,n)           sqrt(vec_sqrlen(a,n))
#define vec_dist(a,b,n)        sqrt(vec_sqrdist(a,b,n))
#define vec_mul(a,b,n)         vec_sclmul(a,b,n)
#define vec_smul(a,b,n)        vec_sclmul(a,b,n)
#define vec_vmul(r,a,b)        vec_vecmul(r,a,b)
#define vec_mmul(r,a,b)        vec_matmul(r,a,b)
#define vec_show(v,n)          vec_write(v, n, stdout, 6, " \n")

/*--------------------------------------------------------------------*/
#define mat_clone(m)           mat_clonex(m,MAT_FULL)
#define mat_copy(d,s)          mat_copyx(d,s,MAT_FULL)
#define mat_diff(a,b)          mat_diffx(a,b,MAT_FULL)
#define mat_issqr(m)           ((m)->rowcnt == (m)->colcnt)
#define mat_vector(m)          ((m)->vec)

#define mat_get(m,r,c)         ((m)->els[r][c])
#define mat_set(m,r,c,x)       ((m)->els[r][c]  = (x))
#define mat_inc(m,r,c,x)       ((m)->els[r][c] += (x))
#define mat_emul(m,r,c,x)      ((m)->els[r][c] *= (x))

#define mat_rowcnt(m)          ((m)->rowcnt)
#define mat_row(m,r)           ((m)->els[r])
#define mat_rowlen(m,r)        sqrt(mat_rowsqr(m,r))
#define mat_rowcopy(d,i,s,k)   mat_rowset(d, i, (s)->els[k])
#define mat_rowadd(d,i,x,s,k)  mat_rowaddv(d, i, x, (s)->els[k])
#define mat_rowmul(d,i,s,k)    mat_rowmulv(d, i, (s)->els[k])

#define mat_colcnt(m)          ((m)->colcnt)
#define mat_collen(m,c)        sqrt(mat_colsqr(m,c))

#define mat_trace(m)           mat_diasum(m)
#define mat_muls(r,m,k)        mat_mulsx(r,m,k,MAT_FULL)
#define mat_add(r,a,k,b)       mat_addx(r,a,k,b,MAT_FULL)
#define mat_mulvtmv(m,v)       mat_mulvmv(m,v)
#define mat_mulvd(r,v,m)       mat_muldv(r,m,v)
#define mat_mulvtdv(m,v)       mat_mulvdv(m,v)
#define mat_mulmmt(r,m)        mat_mulmdm(r,m,NULL)
#define mat_mulmdmt(r,m,d)     mat_mulmdm(r,m,d)
#define mat_norm(m)            sqrt(mat_sqrnorm(m))

#define mat_addwgt(m,w)        ((m)->weight += (w))
#define mat_getwgt(m)          ((m)->weight)
#define mat_setwgt(m,w)        ((m)->weight  = (w))
#define mat_mulwgt(m,f)        ((m)->weight *= (f))

#define mat_show(m)            mat_write(m, stdout, 6, " \n")
#endif
