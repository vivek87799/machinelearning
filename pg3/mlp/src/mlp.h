/*----------------------------------------------------------------------
  File    : mlp.h
  Contents: multilayer perceptron management
  Author  : Christian Borgelt
  History : 2001.02.21 file created
            2001.03.10 first version completed
            2001.03.18 other weight update methods added
            2001.04.16 output scaling added
            2001.05.20 extended functions added (attribute set/table)
            2001.07.17 adapted to modified module scan
            2002.06.04 input scaling/normalization added
            2002.06.18 sensitivity analysis functions added
            2003.03.11 weight parameter added to function mlp_reg
            2003.03.15 functions combined to mlp_factors and mlp_limits
            2004.08.10 function mlp_deletex added
            2004.08.11 adapted to new module attmap
            2004.08.12 adapted to new module parse
            2013.08.13 adapted to definition of type DIMID in matrix.h
----------------------------------------------------------------------*/
#ifndef __MLP__
#define __MLP__
#include <stdio.h>
#include "matrix.h"
#ifdef MLP_PARSE
#ifndef NST_PARSE
#define NST_PARSE
#endif
#ifndef SCN_SCAN
#define SCN_SCAN
#endif
#endif
#ifdef MLP_EXTFN
#include "attmap.h"
#endif
#include "nstats.h"

#ifdef MLP_EXTFN
#if ATTID_MAX != DIMID_MAX
#error "mlp with MLP_EXTFN requires ATTID_MAX == DIMID_MAX"
#endif
#if VALID_MAX != DIMID_MAX
#error "mlp with MLP_EXTFN requires VALID_MAX == DIMID_MAX"
#endif
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define MLP_MAXLAYER   32       /* maximum number of layers */

/* --- training methods --- */
#define MLP_STANDARD    0       /* standard backpropagation */
#define MLP_ADAPTIVE    1       /* self-adaptive learning rate */
#define MLP_RESILIENT   2       /* resilient backpropagation */
#define MLP_QUICK       3       /* quick backpropagation */
#define MLP_MANHATTAN   4       /* Manhattan training */

/* --- sensitivity modes --- */
#define MLP_MAX         0       /* determine maximal output change */
#define MLP_SUM         1       /* determine sum of output changes */
#define MLP_MAXIN       0       /* take max. over input units */
#define MLP_SUMIN       2       /* sum over input units */

/* --- description modes --- */
#define MLP_TITLE     0x0001    /* print a title (as a comment) */
#define MLP_INFO      0x0002    /* print add. info. (as a comment) */

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- an MLP layer --- */
  DIMID    incnt;               /* number of inputs */
  DIMID    outcnt;              /* number of outputs/units */
  double   **wgts;              /* matrix of connection weights */
  double   **chgs;              /* matrix of weight changes */
  double   **grds;              /* matrix of (aggregated) gradients */
  double   **bufs;              /* matrix of buffers */
  double   *ins;                /* vector of inputs */
  double   *outs;               /* vector of outputs */
  double   *errs;               /* (backpropagated) errors */
  void     *rsvd;               /* reserved (alignment) */
} MLPLAYER;                     /* (MLP layer) */

typedef struct {                /* --- a multilayer perceptron --- */
  int      lyrcnt;              /* number of layers */
  DIMID    incnt;               /* number of inputs */
  DIMID    outcnt;              /* number of outputs */
  DIMID    unitcnt;             /* number of units */
  DIMID    wgtcnt;              /* number of weights */
  double   *ins;                /* vector of inputs */
  double   *outs;               /* vector of (computed) outputs */
  double   *mins;               /* vector of minimal output values */
  double   *maxs;               /* vector of maximal output values */
  double   *offs;               /* offsets for output scaling */
  double   *scls;               /* factors for output scaling */
  double   *recs;               /* and their reciprocal values */
  double   *scos;               /* vector of (scaled) outputs */
  double   *trgs;               /* vector of (target) outputs */
  double   *errs;               /* vector of output errors */
  double   *wgts;               /* vector of all connection weights */
  double   *chgs;               /* vector of all weight changes */
  double   *grds;               /* vector of all gradients */
  double   *bufs;               /* vector of all buffers */
  NSTATS   *nst;                /* input normalization statistics */
  #ifdef MLP_EXTFN
  ATTSET   *attset;             /* underlying attribute set */
  ATTMAP   *attmap;             /* attribute map for numeric coding */
  ATT      *trgatt;             /* target attribute */
  #endif
  int      method;              /* weight update method */
  double   raise;               /* raise value for derivative */
  double   lrate;               /* (initial) change/learning rate */
  double   moment;              /* momentum coefficient */
  double   growth;              /* growth    factor */
  double   shrink;              /* shrinkage factor */
  double   minchg;              /* minimal weight change */
  double   maxchg;              /* maximal weight change */
  double   decay;               /* weight decay factor */
  MLPLAYER layers[1];           /* layers of the network */
} MLP;                          /* (multilayer perceptron) */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/
extern MLP*    mlp_create  (int lyrcnt, DIMID *ucnts);
extern void    mlp_delete  (MLP *mlp);
#ifdef MLP_EXTFN
extern MLP*    mlp_createx (ATTMAP *attmap, int lyrcnt, DIMID *ucnts);
extern void    mlp_deletex (MLP *mlp, int delas);
extern ATTSET* mlp_attset  (MLP *mlp);
extern ATT*    mlp_trgatt  (MLP *mlp);
extern ATTID   mlp_trgid   (const MLP *mlp);
#endif

extern int     mlp_lyrcnt  (const MLP *mlp);
extern DIMID   mlp_incnt   (const MLP *mlp);
extern DIMID   mlp_outcnt  (const MLP *mlp);
extern DIMID   mlp_unitcnt (const MLP *mlp);
extern DIMID   mlp_wgtcnt  (const MLP *mlp);

extern void    mlp_method  (MLP *mlp, int method);
extern void    mlp_raise   (MLP *mlp, double raise);
extern void    mlp_lrate   (MLP *mlp, double lrate);
extern void    mlp_moment  (MLP *mlp, double moment);
extern void    mlp_factors (MLP *mlp, double growth, double shrink);
extern void    mlp_limits  (MLP *mlp, double minchg, double maxchg);
extern void    mlp_decay   (MLP *mlp, double decay);
extern void    mlp_scale   (MLP *mlp, DIMID unit,
                            double exp, double nrm);
extern void    mlp_range   (MLP *mlp, DIMID unit,
                            double min, double max);
extern void    mlp_expand  (MLP *mlp, DIMID unit, double factor);

extern void    mlp_reg     (MLP *mlp, const double *ins,
                            const double *trgs, double weight);
extern void    mlp_input   (MLP *mlp, DIMID unit, double value);
extern void    mlp_target  (MLP *mlp, DIMID unit, double value);
extern double  mlp_output  (const MLP *mlp, DIMID unit);
#ifdef MLP_EXTFN
extern void    mlp_regx    (MLP *mlp, const TUPLE *tpl, int ninp);
extern void    mlp_inputx  (MLP *mlp, const TUPLE *tpl);
extern void    mlp_targetx (MLP *mlp, const TUPLE *tpl);
extern void    mlp_outputx (MLP *mlp, TUPLE *tpl);
extern void    mlp_result  (MLP *mlp, INST *inst, double *conf);
#endif

extern void    mlp_init    (MLP *mlp, double rand(void), double range);
extern void    mlp_jog     (MLP *mlp, double rand(void), double range);
extern void    mlp_setup   (MLP *mlp);
extern void    mlp_exec    (MLP *mlp, const double *ins, double *outs);
extern double  mlp_error   (MLP *mlp, const double *trgs);
extern double  mlp_bkprop  (MLP *mlp, const double *trgs);
extern void    mlp_update  (MLP *mlp);
extern double  mlp_sens    (MLP *mlp, DIMID unit, int mode);
#ifdef MLP_EXTFN
extern double  mlp_sensx   (MLP *mlp, DIMID col, int mode);
#endif

extern int     mlp_desc    (MLP *mlp, FILE *file, int mode, int maxlen);
#ifdef MLP_PARSE
extern MLP*    mlp_parse   (SCANNER *scan);
#ifdef MLP_EXTFN
extern MLP*    mlp_parsex  (SCANNER *scan, ATTMAP *attmap);
#endif
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define mlp_lyrcnt(n)      ((n)->lyrcnt)
#define mlp_incnt(n)       ((n)->incnt)
#define mlp_outcnt(n)      ((n)->outcnt)
#define mlp_unitcnt(n)     ((n)->unitcnt)
#define mlp_wgtcnt(n)      ((n)->wgtcnt)
#ifdef MLP_EXTFN
#define mlp_attset(n)      ((n)->attset)
#define mlp_trgatt(n)      ((n)->trgatt)
#define mlp_trgid(n)       (att_id((n)->trgatt))
#endif

#define mlp_method(n,m)    ((n)->method = (m))
#define mlp_raise(n,r)     ((n)->raise  = (r))
#define mlp_lrate(n,r)     ((n)->lrate  = (r))
#define mlp_moment(n,m)    ((n)->moment = (m))
#define mlp_factors(n,g,s) ((n)->growth = (g), (n)->shrink = (s))
#define mlp_limits(n,a,b)  ((n)->minchg = (a), (n)->maxchg = (b))
#define mlp_decay(n,d)     ((n)->decay  = 1.0 -(d))
#define mlp_scale(n,u,o,f) nst_scale((n)->nst, u, o, f);

#define mlp_input(n,i,v)   ((n)->ins[i]  = (n)->nrms[i] \
                                        * ((v) -(n)->exps[i]))
#define mlp_target(n,i,v)  ((n)->trgs[i] = (v))
#define mlp_output(n,i)    ((n)->scos[i])
#ifdef MLP_EXTFN
#define mlp_targetx(n,t)   am_exec((n)->attmap, t, AM_TARGET, (n)->trgs)
#endif

#endif
