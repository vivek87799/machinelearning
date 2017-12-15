/*----------------------------------------------------------------------
  File    : mlp.c
  Contents: multilayer perceptron management
  Author  : Christian Borgelt
  History : 2001.02.26 file created
            2001.03.10 first version completed
            2001.03.18 other weight update methods added
            2001.03.19 tabulated logistic function added
            2001.04.15 tangens hyperbolicus added as a #define option
            2001.04.16 output scaling added
            2001.05.20 extended functions added (attribute set/table)
            2001.07.17 adapted to modified module scan
            2001.09.11 some assertions added
            2002.06.04 input scaling/normalization added
            2002.06.18 sensitivity analysis functions added
            2002.06.19 bug in function mlp_result() fixed
            2002.09.12 scaling bug concerning numeric targets fixed
            2003.01.30 functionality of mlp_reg() extended
            2003.02.01 bug in mapping binary attributes fixed
            2003.02.07 reinitialization added to function mlp_range()
            2003.03.11 weight parameter added to function mlp_reg()
            2003.04.01 bug in function mlp_createx() fixed
            2003.08.06 another bug in mapping binary attributes fixed
            2003.10.24 bug in output range determination fixed
            2004.02.23 check of parabola orientation added to quick()
            2004.08.10 function mlp_deletex() added
            2004.08.11 adapted to new module attmap
            2004.08.12 adapted to new modules parse and nstats
            2007.01.10 usage of attribute maps changed
            2007.02.14 adapted to modified module attset
            2008.04.14 bug in function quick() fixed (parabola orient.)
            2013.06.12 bug in function mlp_desc() fixed (ranges)
            2013.08.28 bug in function mlp_parse() fixed (duplicate nst)
            2014.10.07 bug in function mlp_parse() fixed (missing nst)
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include "mlp.h"
#ifdef STORAGE
#include "storage.h"
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
/* --- activation function and its derivative --- */
#ifdef MLP_TABFN                /* if to use a tabulated function */
#define TABSIZE     1024        /* size of interpolation table */
#define TABMAX      16.0        /* maximum (absolute) argument value */
#define TABSCALE    (TABSIZE/TABMAX)
#endif                          /* scale factor for table indices */

#ifndef MLP_TANH                /* default: logistic function */
#define MACTFN(x)   (1/(1 +exp(-(x))))
#ifdef MLP_TABFN                /* tabulated function */
#define ACTFN(x)    logistic(x)
#else                           /* direct computation */
#define ACTFN(x)    MACTFN(x)
#endif
#define DERIV(x)    ((x)*(1-(x)))
#define ACTMIN       0.0        /* minimal and maximal value */
#define ACTMAX       1.0        /* of the activation function */
#define ACTMID       0.5        /* middle value */
#define MLP_LRATE    0.2

#else                           /* alternative: tangens hyperbolicus */
#define MACTFN(x)   (2/(1 +exp(-2*(x))) -1)
#ifdef MLP_TABFN                /* tabulated function */
#define ACTFN(x)    tanh(x)
#else                           /* direct computation */
#define ACTFN(x)    MACTFN(x)
#endif
#define DERIV(x)    ((1+(x))*(1-(x)))
#define ACTMIN      -1.0        /* minimal and maximal value */
#define ACTMAX       1.0        /* of the activation function */
#define ACTMID       0.0        /* middle value */
#define MLP_LRATE    0.05
#endif

#define NAMELEN     255         /* maximum target name length */

/* --- error codes --- */
#define E_ATTEXP    (-16)       /* attribute expected */
#define E_UNKATT    (-17)       /* unknown attribute */
#define E_LYRCNT    (-18)       /* invalid number of layers */
#define E_UNITCNT   (-19)       /* invalid number of units */

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef void UPDATEFN (MLP* mlp); /* a weight update function */

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
#ifdef MLP_PARSE
static const char *msgs[] = {   /* error messages */
  /*      0 to  -7 */  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /*     -8 to -15 */  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  /* E_ATTEXP  -16 */  "#attribute expected instead of '%s'",
  /* E_UNKATT  -17 */  "#unknown attribute '%s'",
  /* E_LYRCNT  -18 */  "#invalid number of layers",
  /* E_UNITCNT -19 */  "#invalid number of units",
};
#endif

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
#ifdef MLP_TABFN
static double acttab[TABSIZE+1];/* tabulated activation function */
static int    init = 0;         /* whether table is initialized */
#endif

/*----------------------------------------------------------------------
  Activation Function
----------------------------------------------------------------------*/
#ifdef MLP_TABFN
#ifndef MLP_TANH

static double logistic (double x)
{                               /* --- table-based logistic function */
  int    i;                     /* table index */
  double y = fabs(x);           /* argument of tabulated function */

  if (y >= TABMAX)              /* if beyond the table range, */
    return (x > 0) ? 1 : 0;     /* return asymptotic values */
  i = (int)(y *= TABSCALE);     /* otherwise compute the table index */
  y = acttab[i] + (y-i) * (acttab[i+1] -acttab[i]);
  return (x < 0) ? 1-y : y;     /* interpolate between table values */
}  /* logistic() */

#else /*--------------------------------------------------------------*/

static double tanh (double x)
{                               /* --- table-based tanh function */
  int    i;                     /* table index */
  double y = fabs(x);           /* argument of tabulated function */

  if (y >= TABMAX)              /* if beyond the table range, */
    return (x > 0) ? 1 : -1;    /* return asymptotic values */
  i = (int)(y *= TABSCALE);     /* otherwise compute the table index */
  y = acttab[i] + (y-i) * (acttab[i+1] -acttab[i]);
  return (x < 0) ? -y : y;      /* interpolate between table values */
}  /* tanh() */

#endif  /* #ifndef MLP_TANH */
#endif  /* #ifdef MLP_TABFN */
/*----------------------------------------------------------------------
  Weight Update Functions
----------------------------------------------------------------------*/

static void standard (MLP *mlp)
{                               /* --- standard backpropagation */
  DIMID  i;                     /* loop variable */
  double *w, *c, *g;            /* to traverse the vectors */
  double lrate  = mlp->lrate;   /* learning rate */
  double moment = mlp->moment;  /* momentum coefficient */

  w = mlp->wgts;                /* get the necessary vectors and */
  g = mlp->grds;                /* traverse the connection weights */
  if (moment <= 0)              /* if standard backpropagation */
    for (i = 0; i < mlp->wgtcnt; i++) {
      w[i] -= lrate *g[i]; g[i] = 0; }
  else {                        /* if backpropagation with momentum */
    c = mlp->chgs;              /* get the vector of old changes */
    for (i = 0; i < mlp->wgtcnt; i++) {
      w[i] += c[i] = moment *c[i] - lrate *g[i]; g[i] = 0; }
  }                             /* update the connection weights */
}  /* standard() */

/*--------------------------------------------------------------------*/

static void adaptive (MLP *mlp)
{                               /* --- super self-adaptive backprop. */
  DIMID  i;                     /* loop variable */
  double *w, *c, *g, *p;        /* to traverse the vectors */
  double t;                     /* temporary buffer */

  w = mlp->wgts; c = mlp->chgs; /* get the necessary vectors and */
  g = mlp->grds; p = mlp->bufs; /* traverse the connection weights */
  for (i = 0; i < mlp->wgtcnt; i++) {
    if      (g[i] > 0) t =  p[i];
    else if (g[i] < 0) t = -p[i];
    else               t =  0;  /* check directions of the changes */
    if      (t > 0) {           /* if gradients have the same sign, */
      c[i] *= mlp->growth;      /* increase the step width */
      if (c[i] > mlp->maxchg) c[i] = mlp->maxchg;
      p[i] = g[i]; }            /* note the current gradient */
    else if (t < 0) {           /* if gradients have opposite signs, */
      c[i] *= mlp->shrink;      /* decrease the step width */
      if (c[i] < mlp->minchg) c[i] = mlp->minchg;
      p[i] = 0; }               /* suppress a change in the next step */
    else {                      /* if one gradient is zero, */
      p[i] = g[i]; }            /* only note the current gradient */
    w[i] -= c[i] *g[i];         /* update the connection weight */
    g[i] = 0;                   /* and clear the gradient */
  }                             /* for the next step */
}  /* adaptive() */

/*--------------------------------------------------------------------*/

static void resilient (MLP *mlp)
{                               /* --- resilient backpropagation */
  DIMID  i;                     /* loop variable */
  double *w, *c, *g, *p;        /* to traverse the vectors */
  double t;                     /* temporary buffer */

  w = mlp->wgts; c = mlp->chgs; /* get the necessary vectors and */
  g = mlp->grds; p = mlp->bufs; /* traverse the connection weights */
  for (i = 0; i < mlp->wgtcnt; i++) {
    if      (g[i] > 0) t =  p[i];
    else if (g[i] < 0) t = -p[i];
    else               t =  0;  /* check directions of the changes */
    if      (t > 0) {           /* if gradients have the same sign, */
      c[i] *= mlp->growth;      /* increase the update value */
      if (c[i] > mlp->maxchg) c[i] = mlp->maxchg;
      p[i] = g[i]; }            /* note the current gradient */
    else if (t < 0) {           /* if gradients have opposite signs, */
      c[i] *= mlp->shrink;      /* decrease the update value */
      if (c[i] < mlp->minchg) c[i] = mlp->minchg;
      p[i] = 0; }               /* suppress a change in the next step */
    else {                      /* if one gradient is zero, */
      p[i] = g[i]; }            /* only note the current gradient */
    if      (g[i] > 0) w[i] -= c[i];  /* update the weight */
    else if (g[i] < 0) w[i] += c[i];  /* (but use only the sign) */
    g[i] = 0;                   /* clear the gradient */
  }                             /* for the next step */
}  /* resilient() */

/*--------------------------------------------------------------------*/

static void quick (MLP *mlp)
{                               /* --- quick backpropagation */
  DIMID  i;                     /* loop variable */
  double *w, *c, *g, *p;        /* to traverse the vectors */
  double lrate = mlp->lrate;    /* learning rate */
  double m;                     /* maximal fraction of new derivative */
  double t;                     /* temporary buffer */

  m = mlp->growth /(mlp->growth +1);
  w = mlp->wgts; c = mlp->chgs; /* get the necessary vectors and */
  g = mlp->grds; p = mlp->bufs; /* traverse the connection weights */
  for (i = 0; i < mlp->wgtcnt; i++) {
    t = p[i] -g[i];             /* compute the change of the gradient */
    if      (t * c[i] >= 0)     /* if the parabola opens downwards, */
      c[i] = -lrate *g[i];      /* do a normal backpropagation step */
    else if (p[i] > 0) {        /* if previous gradient was positive */
      if (g[i] < m *p[i])       /* compute the factor for a jump */
        c[i] *= g[i] /t;        /* to the minimum (apex of parabola) */
      else                      /* if the growth factor would become */
        c[i] *= mlp->growth;    /* too large, use the maximal factor */
      if (g[i] > 0)             /* if steps are in the same dir., */
        c[i] -= lrate *g[i]; }  /* add a normal backpropagation step */
    else if (p[i] < 0) {        /* if previous gradient was negative */
      if (g[i] > m *p[i])       /* compute the factor for a jump */
        c[i] *= g[i] /t;        /* to the minimum (apex of parabola) */
      else                      /* if the growth factor would become */
        c[i] *= mlp->growth;    /* too large, use the maximal factor */
      if (g[i] < 0)             /* if steps are in the same dir., */
        c[i] -= lrate *g[i]; }  /* add a normal backpropagation step */
    else                        /* if this is the first update, */
      c[i] = -lrate *g[i];      /* do a normal backpropagation step */
    if      (c[i] >  mlp->maxchg) c[i] =  mlp->maxchg;  /* clamp the */
    else if (c[i] < -mlp->maxchg) c[i] = -mlp->maxchg;  /* change */
    w[i] += c[i];               /* adapt the connection weight, */
    p[i] = g[i]; g[i] = 0;      /* note the gradient and clear it */
  }                             /* for the next step */
}  /* quick() */

/*--------------------------------------------------------------------*/

static void manhattan (MLP *mlp)
{                               /* --- Manhattan training */
  DIMID  i;                     /* loop variable */
  double *w, *g;                /* to traverse the vectors */
  double lrate  = mlp->lrate;   /* learning rate */

  w = mlp->wgts;                /* get the necessary vectors and */
  g = mlp->grds;                /* traverse the connection weights */
  for (i = 0; i < mlp->wgtcnt; i++) {
    if      (g[i] > 0) w[i] -= lrate;
    else if (g[i] < 0) w[i] += lrate;
    g[i] = 0;                   /* update the connection weights */
  }                             /* and clear the gradient */
}  /* manhattan() */            /* for the next step */

/*--------------------------------------------------------------------*/

static UPDATEFN *updatefn[] = {
  /* MLP_STANDARD   0 */  standard,
  /* MLP_ADPATIVE   1 */  adaptive,
  /* MLP_RESILIENT  2 */  resilient,
  /* MLP_QUICK      3 */  quick,
  /* MLP_MANHATTAN  4 */  manhattan,
};                              /* list of weight update functions */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/

static MLP* create (int lyrcnt, DIMID *ucnts)
{                               /* --- create a multilayer perceptron */
  int    l;                     /* loop variable for layers */
  DIMID  k, n;                  /* loop variable for weights, buffer */
  MLP    *mlp;                  /* created multilayer perceptron */
  double **pp, *p, *o;          /* to traverse the allocated vectors */

  #ifdef MLP_TABFN              /* if to use tabulated logistic fn. */
  if (!_init) {                 /* if table has not been initialized */
    for (k = TABSIZE; --k >= 0; )
      acttab[k] = MACTFN(k *(1/TABSCALE));
    acttab[TABSIZE] = 1;        /* precompute a table for */
    init = 1;                   /* the activation function and */
  }                             /* set the initialization flag */
  #endif  /* #ifdef MLP_TABFN */

  assert((lyrcnt >= 2)          /* check the function arguments */
  &&     (lyrcnt <= MLP_MAXLAYER) && ucnts);
  mlp = (MLP*)malloc(sizeof(MLP) +(size_t)(lyrcnt-2) *sizeof(MLPLAYER));
  if (!mlp) return NULL;        /* allocate the base structure */
  mlp->lyrcnt = lyrcnt;         /* note the number of layers and */
  mlp->incnt  = ucnts[0];       /* the number of inputs/outputs */
  mlp->outcnt = ucnts[lyrcnt-1];
  assert(mlp->outcnt > 0);      /* check the number of outputs */
  for (k = 0, l = lyrcnt; --l > 0; )
    k += ucnts[l];              /* determine the number of units */
  mlp->unitcnt = k +ucnts[0];   /* and allocate matrix vectors */
  pp = (double**)malloc(4 *(size_t)k *sizeof(double*));
  if (!pp) { free(mlp); return NULL; }
  for (n = 0, l = lyrcnt; --l > 0; )
    n += ucnts[l] *(ucnts[l-1] +1); /* det. the number of weights */
  mlp->wgtcnt = n;                  /* and num. of inputs/outputs */
  k += mlp->incnt +mlp->outcnt;     /* and allocate number vectors */
  p = (double*)malloc((2*(size_t)k +4*(size_t)n +6*(size_t)mlp->outcnt)
                      *sizeof(double));
  if (!p) { free(pp); free(mlp); return NULL; }
  mlp->mins = p; p += mlp->outcnt;  /* set the vectors for the */
  mlp->maxs = p; p += mlp->outcnt;  /* minimal/maximal outputs, */
  mlp->offs = p; p += mlp->outcnt;  /* the offsets, */
  mlp->scls = p; p += mlp->outcnt;  /* the scaling factors */
  mlp->recs = p; p += mlp->outcnt;  /* and their reciprocals, */
  mlp->scos = p; p += mlp->outcnt;  /* the scaled outputs, */
  mlp->trgs = p; p += mlp->outcnt;  /* the target outputs, */
  mlp->ins = o = p; p += mlp->incnt;/* and the input vector */
  lyrcnt -= 1;                  /* traverse the network layers */
  for (l = 0; l < lyrcnt; l++){ /* and init. inputs and outputs */
    mlp->layers[l].incnt  = ucnts[l];
    mlp->layers[l].outcnt = k = ucnts[l+1];
    mlp->layers[l].ins    = o; o = p;
    mlp->layers[l].outs   = p; p += k;
    mlp->layers[l].errs   = p; p += k;
  }                             /* set the layer specific vectors */
  mlp->outs = mlp->layers[l-1].outs; /* set output and error vector */
  mlp->errs = mlp->layers[l-1].errs; /* of the network as a whole */
  mlp->wgts = p;                /* note the weight vector */
  for (l = 0; l < lyrcnt; l++){ /* traverse the layers */
    mlp->layers[l].wgts = pp; n = ucnts[l] +1;
    for (k = ucnts[l+1]; --k >= 0; ) { *pp++ = p; p += n; }
  }                             /* set the weight matrix lines */
  mlp->chgs = p;                /* note the weight change vector */
  for (l = 0; l < lyrcnt; l++){ /* traverse the layers */
    mlp->layers[l].chgs = pp; n = ucnts[l] +1;
    for (k = ucnts[l+1]; --k >= 0; ) { *pp++ = p; p += n; }
  }                             /* set the weight change matrix lines */
  mlp->grds = p;                /* note the gradient vector */
  for (l = 0; l < lyrcnt; l++){ /* traverse the layers */
    mlp->layers[l].grds = pp; n = ucnts[l] +1;
    for (k = ucnts[l+1]; --k >= 0; ) { *pp++ = p; p += n; }
  }                             /* set the gradient matrix lines */
  mlp->bufs = p;                /* note the buffer vectors */
  for (l = 0; l < lyrcnt; l++){ /* traverse the layers */
    mlp->layers[l].bufs = pp; n = ucnts[l] +1;
    for (k = ucnts[l+1]; --k >= 0; ) { *pp++ = p; p += n; }
  }                             /* set the buffer vectors */
  for (k = 0; k < mlp->outcnt; k++) {
    mlp->mins[k] =  INFINITY;   /* traverse the outputs and */
    mlp->maxs[k] = -INFINITY;   /* initialize the output ranges */
  }
  mlp->nst    = NULL;           /* clear the norm. statistics */
  mlp->method = MLP_STANDARD;   /* set default update method */
  mlp->raise  = 0.0;            /* and set default values */
  mlp->lrate  = MLP_LRATE;      /* for the learning rate */
  mlp->growth = 1.2;            /* and all other parameters */
  mlp->shrink = 0.5;
  mlp->minchg = 1e-6;
  mlp->maxchg = 16.0;
  mlp->moment = 0.0;
  mlp->decay  = 1.0;
  #ifdef MLP_EXTFN              /* if to compile extended functions, */
  mlp->attmap = NULL;           /* clear the attribute set */
  #endif
  return mlp;                   /* return the created network */
}  /* create() */

/*--------------------------------------------------------------------*/

MLP* mlp_create (int lyrcnt, DIMID *ucnts)
{                               /* --- create a multilayer perceptron */
  MLP *mlp;                     /* created multilayer perceptron */

  mlp = create(lyrcnt, ucnts);  /* create a multilayer perceptron */
  if (!mlp) return NULL;        /* and the normalization statistics */
  mlp->nst = nst_create(ucnts[0]);
  if (!mlp->nst) { mlp_delete(mlp); return NULL; }
  return mlp;                   /* return the created network */
}  /* mlp_create() */

/*--------------------------------------------------------------------*/

void mlp_delete (MLP *mlp)
{                               /* --- delete a multilayer perceptron */
  assert(mlp);                  /* check the function arguments */
  free(mlp->mins);              /* delete the weight vectors etc., */
  free(mlp->layers[0].wgts);    /* the weight matrix vectors and */
  nst_delete(mlp->nst);         /* the normalization statistics */
  free(mlp);                    /* delete the base structure */
}  /* mlp_delete() */

/*--------------------------------------------------------------------*/
#ifdef MLP_EXTFN                /* if to compile extended functions */

MLP* mlp_createx (ATTMAP *attmap, int lyrcnt, DIMID *ucnts)
{                               /* --- create a multilayer perceptron */
  MLP *mlp;                     /* created multilayer perceptron */

  assert(attmap && ucnts);      /* check the function arguments */
  ucnts[0]        = am_incnt (attmap);
  ucnts[lyrcnt-1] = am_outcnt(attmap);
  mlp = mlp_create(lyrcnt, ucnts);
  if (!mlp) return NULL;        /* create a multilayer perceptron */
  mlp->attset = am_attset(attmap);
  mlp->attmap = attmap;         /* note the attribute set and map */
  mlp->trgatt = am_att(attmap, -1);
  return mlp;                   /* return the created MLP */
}  /* mlp_createx() */

/*--------------------------------------------------------------------*/

void mlp_deletex (MLP *mlp, int delas)
{                               /* --- delete a multilayer perceptron */
  if (delas) {                  /* delete the attribute set and map */
    am_delete(mlp->attmap, 0); as_delete(mlp->attset); }
  mlp_delete(mlp);              /* delete the multilayer perceptron */
}  /* mlp_deletex() */

#endif  /* #ifdef MLP_EXTFN */
/*--------------------------------------------------------------------*/

void mlp_range (MLP *mlp, DIMID unit, double min, double max)
{                               /* --- set range of output values */
  assert(mlp && (unit < mlp->outcnt));     /* check the arguments */
  if (unit < 0) {               /* if to reinitialize the ranges */
    for (unit = mlp->outcnt; --unit >= 0; ) {
      mlp->mins[unit] =  INFINITY; /* traverse all output units and */
      mlp->maxs[unit] = -INFINITY; /* set the maximal range w.r.t. */
    } }                         /* the used activation function */
  else {                        /* if to set a specific range */
    mlp->mins[unit] = min;      /* set minimal */
    mlp->maxs[unit] = max;      /* and maximal value */
  }                             /* for the specified output */
}  /* mlp_range() */

/*--------------------------------------------------------------------*/

void mlp_expand (MLP *mlp, DIMID unit, double factor)
{                               /* --- expand range of output values */
  double t;                     /* change of minimal/maximal value */

  assert(mlp                    /* check the function arguments */
  &&    (unit >= 0) && (unit < mlp->outcnt) && (factor > 0));
  t = (mlp->maxs[unit] -mlp->mins[unit]) *(factor -1) *0.5;
  mlp->mins[unit] -= t;         /* adapt minimal and */
  mlp->maxs[unit] += t;         /* maximal output value */
}  /* mlp_expand() */

/*--------------------------------------------------------------------*/

void mlp_reg (MLP *mlp, const double *ins,
                        const double *trgs, double weight)
{                               /* --- register input/target pairs */
  DIMID i;                      /* loop variable */

  assert(mlp);                  /* check the function arguments */
  nst_reg(mlp->nst, ins, weight);
  if (!trgs) return;            /* check if target values are given */
  for (i = 0; i < mlp->outcnt; i++) {
    if (trgs[i] < mlp->mins[i]) mlp->mins[i] = trgs[i];
    if (trgs[i] > mlp->maxs[i]) mlp->maxs[i] = trgs[i];
  }                             /* adapt the ranges of output values */
}  /* mlp_reg() */

/*--------------------------------------------------------------------*/
#ifdef MLP_EXTFN

void mlp_regx (MLP *mlp, const TUPLE *tpl, int ninp)
{                               /* --- register a training tuple */
  assert(mlp);                  /* check the function arguments */
  if (!tpl) {                   /* if to terminate registration */
    mlp_reg(mlp, NULL, NULL, 0); return; }
  am_exec(mlp->attmap, tpl, AM_INPUTS, mlp->ins);
  am_exec(mlp->attmap, tpl, AM_TARGET, mlp->trgs);
  mlp_reg(mlp, (ninp) ? mlp->ins : NULL, mlp->trgs, tpl_getwgt(tpl));
}  /* mlp_regx() */             /* register the mapped tuple */

/*--------------------------------------------------------------------*/

void mlp_inputx (MLP *mlp, const TUPLE *tpl)
{                               /* --- set inputs from a tuple */
  assert(mlp);                  /* check the function arguments */
  am_exec(mlp->attmap, tpl, AM_INPUTS, mlp->ins);
  nst_norm(mlp->nst, mlp->ins, mlp->ins);
}  /* mlp_inputx() */           /* normalize the mapped input values */

/*--------------------------------------------------------------------*/

void mlp_outputx (MLP *mlp, TUPLE *tpl)
{                               /* --- store output in a tuple */
  assert(mlp);                  /* check the function arguments */
  mlp_result(mlp, (tpl) ? tpl_colval(tpl, att_id(mlp->trgatt))
                        : att_inst(mlp->trgatt), NULL);
}  /* mlp_outputx() */          /* store the result */

/*--------------------------------------------------------------------*/

void mlp_result (MLP *mlp, INST *inst, double *conf)
{                               /* --- store output in an instance */
  int    t;                     /* type of attribute attribute */
  VALID  i, k;                  /* loop variables for values */
  double max, o;                /* (maximal) output value */
  double sum;                   /* sum of output values */

  assert(mlp && inst);          /* check the function arguments */
  t = am_type(mlp->attmap, AM_TARGET);
  if      (t != AT_NOM) {       /* if the target is numeric */
    o = mlp->scos[0];           /* get the scaled output */
    if (t == AT_FLT) inst->f = (DTFLT) o;
    else             inst->i = (DTINT)(o +0.5);
    if (conf) *conf = 0; }      /* no confidence measure available */
  else if (mlp->outcnt <= 1) {  /* if the target is binary */
    o = (att_valcnt(mlp->trgatt) < 2)
      ? 0 : mlp->outs[0];       /* compare the output to 0.5 */
    if (o > ACTMID) { inst->n = 1;
      if (conf) *conf = (o -ACTMIN) * (1/(ACTMAX -ACTMIN)); }
    else            { inst->n = 0;
      if (conf) *conf = (ACTMAX -o) * (1/(ACTMAX -ACTMIN)); } }
  else {                        /* if the target is symbolic */
    sum = 0; max = -INFINITY; k = NV_NOM;
    for (i = 0; i < mlp->outcnt; i++) {
      sum += o = mlp->outs[i];  /* get and sum the activations */
      if (o > max) { max = o; k = i; }
    }                           /* find the output with the highest */
    inst->n = k;                /* value and set the instantiation */
    if (conf) *conf = (sum > 0) ? ((max > 1) ? 1 : max) /sum : 0;
  }                             /* compute and set the confidence */
}  /* mlp_result() */

#endif
/*--------------------------------------------------------------------*/

void mlp_init (MLP *mlp, double rand(void), double range)
{                               /* --- init. multilayer perceptron */
  DIMID i;                      /* loop variable */

  assert(mlp && rand && (range > 0));   /* check the function args. */
  range *= 2;                   /* compute the full range of values */
  for (i = 0; i < mlp->wgtcnt; i++)     /* init. the weights */
    mlp->wgts[i] = range *(rand()-0.5); /* to random values */
}  /* mlp_init() */

/*--------------------------------------------------------------------*/

void mlp_jog (MLP *mlp, double rand(void), double range)
{                               /* --- jog weights of m.l. perceptron */
  DIMID i;                      /* loop variable */

  assert(mlp && rand && (range > 0));   /* check the function args. */
  range *= 2;                   /* compute the full range of values */
  for (i = 0; i < mlp->wgtcnt; i++)      /* jog the weights */
    mlp->wgts[i] += range *(rand()-0.5); /* with random values */
}  /* mlp_jog() */

/*--------------------------------------------------------------------*/

void mlp_setup (MLP *mlp)
{                               /* --- set up multilayer perceptron */
  DIMID  i;                     /* loop variable */
  double t;                     /* temporary buffer */
  double *m, *n, *o, *r, *s;    /* to access the arrays */

  assert(mlp);                  /* check the function argument */
  if ((mlp->method == MLP_RESILIENT)
  ||  (mlp->method == MLP_ADAPTIVE)) {
    for (i = 0; i < mlp->wgtcnt; i++)
      mlp->chgs[i] = mlp->lrate; }    /* initialize weight changes */
  else                          /* and clear gradients and buffers */
    memset(mlp->chgs, 0, (size_t)mlp->wgtcnt *sizeof(double));
  memset(mlp->grds, 0, (size_t)mlp->wgtcnt *sizeof(double));
  memset(mlp->bufs, 0, (size_t)mlp->wgtcnt *sizeof(double));
  o = mlp->offs;                /* get the vectors of offsets */
  s = mlp->scls; r = mlp->recs; /* and (inverse) scaling factors */
  m = mlp->mins; n = mlp->maxs; /* as well as minimum and maximum */
  for (i = 0; i < mlp->outcnt; i++) {
    if (m[i] > n[i]) { m[i] = ACTMIN; n[i] = ACTMAX; }
    t = n[i] -m[i];             /* compute the range and */
    s[i] = t /(ACTMAX -ACTMIN); /* the scaling factor */
    t = (t > 0) ? 1/t : 1.0;    /* avoid infinity */
    r[i] = t *(ACTMAX -ACTMIN); /* comput reciprocal scaling factor */
    o[i] = m[i] -ACTMIN *s[i];  /* and the offset */
  }                             /* (linear transformation parameters) */
}  /* mlp_setup() */

/*--------------------------------------------------------------------*/

void mlp_exec (MLP *mlp, const double *ins, double *outs)
{                               /* --- execute multilayer perceptron */
  int      l;                   /* loop variable  for layers */
  DIMID    k, n;                /* loop variables for weights */
  MLPLAYER *layer;              /* to traverse the network layers */
  double   *wgt;                /* to traverse the weight vectors */
  double   net;                 /* sum of weighted inputs */

  assert(mlp);                  /* check the function arguments */
  if (ins)                      /* normalize the input vector */
    nst_norm(mlp->nst, ins, mlp->ins);
  layer = mlp->layers;          /* traverse the network layers */
  for (l = mlp->lyrcnt-1; --l >= 0; ++layer) {
    for (k = layer->outcnt; --k >= 0; ) {
      wgt = layer->wgts[k];     /* traverse the units of the layer */
      net = wgt[n = layer->incnt];
      while (--n >= 0) net += layer->ins[n] *wgt[n];
      layer->outs[k] = ACTFN(net);
    }                           /* sum the weighted inputs and */
  }                             /* compute the activation (output) */
  for (k = 0; k < mlp->outcnt; k++) /* apply output transformation */
    mlp->scos[k] = mlp->outs[k] *mlp->scls[k] +mlp->offs[k];
  if (outs)                     /* copy outputs to result vector */
    memcpy(outs, mlp->scos, (size_t)mlp->outcnt *sizeof(double));
}  /* mlp_exec() */

/*--------------------------------------------------------------------*/

double mlp_error (MLP *mlp, const double *trgs)
{                               /* --- compute sum of squared errors */
  DIMID  k;                     /* loop variable */
  double sse = 0, d;            /* sum of squared errors, buffer */

  assert(mlp);                  /* check the function arguments */
  if (!trgs) trgs = mlp->trgs;  /* if not targets, use intern. vector */
  for (k = 0; k < mlp->outcnt; k++) {
    mlp->errs[k] = mlp->recs[k] *(d = trgs[k] -mlp->scos[k]);
    sse += d*d;                 /* compute the output errors */
  }                             /* (target - scaled computed output) */
  return sse;                   /* return sum of squared errors */
}  /* mlp_error() */

/*--------------------------------------------------------------------*/

double mlp_bkprop (MLP *mlp, const double *trgs)
{                               /* --- backpropagate errors */
  int      l;                   /* loop variable  for layers */
  DIMID    k, n;                /* loop variables for weights */
  MLPLAYER *layer;              /* to traverse the network layers */
  double   *w, *g;              /* to traverse the weights/gradients */
  double   *e;                  /* to traverse the errors */
  double   delta;               /* temporary buffer */
  double   sse;                 /* sum of squared errors */
  double   raise = mlp->raise;  /* raise value for derivative */

  assert(mlp);                  /* check the function arguments */
  sse = mlp_error(mlp, trgs);   /* compute sum of squared errors */
  for (l = mlp->lyrcnt-2; --l >= 0; ) {
    memset(mlp->layers[l].errs, 0,
          (size_t)mlp->layers[l].outcnt *sizeof(double));
  }                             /* clear the backpropagated errors */
  for (l = mlp->lyrcnt-1; --l >  0; ) {
    layer = mlp->layers +l;     /* traverse the units of each layer */
    for (k = 0; k < layer->outcnt; k++) {
      delta = layer->errs[k] * (DERIV(layer->outs[k]) +raise);
      g     = layer->grds[k];   /* first process the offset gradient */
      g[n = layer->incnt] -= delta;
      while (--n >= 0)          /* then process the weight gradients */
        g[n] -= layer->ins[n] * delta;
      e = (layer-1)->errs;      /* traverse the errors */
      w = layer->wgts[k];       /* and the connection weights */
      for (n = 0; n < layer->incnt; n++)
        e[n] += w[n] * delta;   /* propagate the errors back */
    }                           /* to the preceding layer */
  }
  layer = mlp->layers;          /* traverse the first hidden layer */
  for (k = 0; k < layer->outcnt; k++) {
    delta = layer->errs[k] * (DERIV(layer->outs[k]) +raise);
    g     = layer->grds[k];     /* first process the offset gradient */
    g[n = layer->incnt] -= delta;
    while (--n >= 0)            /* then process the weight gradients */
      g[n] -= layer->ins[n] * delta;
  }                             /* (but do no backpropagation) */
  return sse;                   /* return sum of squared errors */
}  /* mlp_bkprop() */

/*--------------------------------------------------------------------*/

void mlp_update (MLP *mlp)
{                               /* --- update connection weights */
  DIMID k;                      /* loop variable */

  assert(mlp);                  /* check the function argument */
  if (mlp->decay != 1.0)        /* if weight decay is requested */
    for (k = 0; k < mlp->wgtcnt; k++)
      mlp->wgts[k] *= mlp->decay; /* reduce all connection weights */
  updatefn[mlp->method](mlp);   /* call the weight update function */
}  /* mlp_update() */

/*--------------------------------------------------------------------*/

double mlp_sens (MLP *mlp, DIMID unit, int mode)
{                               /* --- analyze sensitivity on input */
  int      l;                   /* loop variable  for layers */
  DIMID    k, n;                /* loop variables for weights */
  MLPLAYER *layer;              /* to traverse the network layers */
  double   **w;                 /* to traverse the weight vectors */
  double   *e, *p;              /* to traverse the sensitivity values */
  double   s;                   /* resulting sensitivity */

  assert(mlp                    /* check the function arguments */
  &&    (unit >= 0) && (unit < mlp->incnt));
  layer = mlp->layers;          /* get the first hidden layer */
  w     = layer->wgts;          /* traverse the first hidden layer */
  e     = layer->errs;          /* and compute sensitivity values */
  for (k = layer->outcnt; --k >= 0; )
    e[k] = w[k][unit] *DERIV(layer->outs[k]);
  for (l = mlp->lyrcnt-2; --l >= 0; ) {
    p = e; ++layer;             /* traverse the remaining layers */
    e = memset(layer->errs, 0, (size_t)layer->outcnt *sizeof(double));
    w = layer->wgts;            /* clear the sensitivity values */
    for (k = layer->outcnt; --k >= 0; ) {
      for (n = layer->incnt; --n >= 0; )
        e[k] += w[k][n] *p[n];  /* aggregate the sensitivity values */
      e[k] *= DERIV(layer->outs[k]);  /* of the preceding layer and */
    }                           /* compute the sensitivity values */
  }                             /* of the next layer */
  p = mlp->scls;                /* traverse the output units */
  for (k = mlp->outcnt; --k >= 0; )
    e[k] = fabs(e[k] *p[k]);    /* compute absolute sensitivity value */
  k = mlp->outcnt; s = 0;       /* compute sum or maximum */
  if (mode & MLP_SUM) while (--k >= 0) { s += e[k]; }
  else                while (--k >= 0) { if (e[k] > s) s = e[k]; }
  return s;                     /* return the sensitivity */
}  /* mlp_sens() */

/*--------------------------------------------------------------------*/
#ifdef MLP_EXTFN

double mlp_sensx (MLP *mlp, DIMID col, int mode)
{                               /* --- analyze sensitivity on input */
  DIMID  cnt, off;              /* loop variable, offset */
  double s, t;                  /* sensitivity value, buffer */

  assert(mlp                    /* check the function arguments */
  &&    (col >= 0) && (col < am_attcnt(mlp->attmap)));
  cnt = am_cnt(mlp->attmap, col);
  off = am_off(mlp->attmap, col);
  if (cnt <= 2)                 /* if there is only one unit */
    return mlp_sens(mlp, off, mode);
  for (s = 0; --cnt >= 0; ) {   /* traverse the inputs */
    t = mlp_sens(mlp, off +cnt, mode);
    if (mode & MLP_SUMIN) s += t;
    else if (t > s)       s  = t;
  }                             /* sum/take maximum of sensitivity */
  return s;                     /* for the inputs and return it */
}  /* mlp_sensx() */

#endif
/*--------------------------------------------------------------------*/

int mlp_desc (MLP *mlp, FILE *file, int mode, int maxlen)
{                               /* --- describe multilayer perceptron */
  int      l;                   /* loop variable  for layers */
  DIMID    k, n;                /* loop variables for weights */
  int      len, i;              /* loop variables for comments */
  double   **wgts;              /* to traverse the weight vectors */
  char     *indent = "";        /* indentation string */
  #ifdef MLP_EXTFN
  char     buf[AS_MAXLEN+1];    /* output buffer for target name */
  #endif

  assert(mlp && file);          /* check the function arguments */
  len = (maxlen > 0) ? maxlen -2 : 70;

  /* --- print header (as a comment) --- */
  if (mode & MLP_TITLE) {       /* if the title flag is set */
    fputs("/*", file); for (i = len; --i >= 0; ) fputc('-', file);
    fprintf(file, "\n  multilayer perceptron\n");
    for (i = len; --i >= 0; ) fputc('-', file);
    fputs("*/\n", file);        /* print a title header */
  }                             /* (as a comment) */

  #ifdef MLP_EXTFN              /* if to compile extended functions */
  if (mlp->attmap) {            /* if based on an attribute set, */
    fputs("mlp(", file);        /* start the network description */
    scn_format(buf, att_name(mlp->trgatt), 0);
    fputs(buf, file);           /* print target attribute name */
    fputs(") = {\n", file);     /* and start the description */
    indent = "  ";              /* indent the network description */
  }                             /* by two characters */
  #endif

  /* --- print list of numbers of units --- */
  fprintf(file, "%sunits    = %"DIMID_FMT, indent, mlp->incnt);
  for (l = 0; l < mlp->lyrcnt-1; l++)
    fprintf(file, ", %"DIMID_FMT, mlp->layers[l].outcnt);
  fprintf(file, ";\n");         /* print number of units per layer */

  /* --- print list of input scalings --- */
  if (mlp->incnt > 0)           /* check whether there are inputs */
    nst_desc(mlp->nst, file, indent, maxlen);

  /* --- print list of weight vectors --- */
  fprintf(file, "%sweights  = ", indent);
  for (l = 0; l < mlp->lyrcnt-1; l++) {   /* traverse the layers */
    if (l > 0) fprintf(file, ",\n%s           ", indent);
    fputc('{', file);           /* start a layer description */
    wgts = mlp->layers[l].wgts; /* traverse the units in a layer */
    for (k = 0; k < mlp->layers[l].outcnt; k++) {
      if (k > 0) fprintf(file, ",\n%s            ", indent);
      fputs("{ ", file);        /* print the connection weights */
      for (n = 0; n < mlp->layers[l].incnt; n++)
        fprintf(file, "%+.16g, ", wgts[k][n]);
      fprintf(file, "%+.16g }", wgts[k][n]);
    }                           /* print the bias value */
    fputc('}', file);           /* terminate the layer description */
  }                             /* if not last layer, start new line */
  fprintf(file, ";\n");         /* terminate the list */

  /* --- print list of output ranges --- */
  fprintf(file, "%sranges   = ", indent);
  for (k = 0; k < mlp->outcnt; k++) {
    if (k > 0) fputs(", ", file);
    if (mlp->maxs[k] < mlp->mins[k]) {
      mlp->mins[k] = ACTMIN; mlp->maxs[k] = ACTMAX; }
    fprintf(file, "[%.16g, %.16g]", mlp->mins[k], mlp->maxs[k]);
  }                             /* print the output ranges */
  fputs(";\n", file);           /* terminate the list */

  #ifdef MLP_EXTFN              /* if to compile extended functions */
  if (mlp->attmap) fputs("};\n", file);
  #endif                        /* terminate the description */

  /* --- print additional information (as a comment) --- */
  if (mode & MLP_INFO) {        /* if the add. info. flag is set */
    fputs("\n/*", file); for (i = len; --i >= 0; ) fputc('-', file);
    fprintf(file, "\n  number of inputs : %"DIMID_FMT, mlp->incnt);
    fprintf(file, "\n  number of outputs: %"DIMID_FMT, mlp->outcnt);
    fprintf(file, "\n  number of units  : %"DIMID_FMT, mlp->unitcnt);
    fprintf(file, "\n  number of weights: %"DIMID_FMT, mlp->wgtcnt);
    fputc('\n', file);          /* print additional network info. */
    for (i = len; --i >= 0; ) fputc('-', file);
    fputs("*/\n", file);        /* (as a comment) */
  }
  return ferror(file);          /* return write error status */
}  /* mlp_desc() */

/*--------------------------------------------------------------------*/
#ifdef MLP_PARSE

static int getucnts (SCANNER *scan, DIMID ucnts[],
                     DIMID incnt, DIMID outcnt)
{                               /* --- get number of units per layer */
  int   lyrcnt = 0;             /* number of layers */
  char  *s;                     /* end pointer for conversion */
  DIMID n;                      /* temporary buffer */

  assert(scan && ucnts);        /* check the function arguments */
  if ((scn_token(scan) != T_ID) /* check for 'units' */
  ||  (strcmp(scn_value(scan), "units") != 0))
    SCN_ERROR(scan, E_STREXP, "units");
  SCN_NEXT(scan);               /* consume 'units' */
  SCN_CHAR(scan, '=');          /* consume '=' */
  while (1) {                   /* unit counter read loop */
    if (scn_token(scan) != T_NUM)  SCN_ERROR(scan, E_NUMEXP);
    if (lyrcnt >= MLP_MAXLAYER)    SCN_ERROR(scan, E_LYRCNT);
    n = strtodim(scn_value(scan), &s);
    if (*s || (n < 0))             SCN_ERROR(scan, E_NUMBER);
    if ((lyrcnt >  0) && (n <= 0)) SCN_ERROR(scan, E_NUMBER);
    if ((lyrcnt == 0) && (incnt > 0) && (n != incnt))
      SCN_ERROR(scan,E_UNITCNT);/* get and check number of inputs */
    SCN_NEXT(scan);             /* consume the number of units */
    ucnts[lyrcnt++] = n;        /* and store it */
    if (scn_token(scan) != ',') break;
    SCN_NEXT(scan);             /* if at end of list, abort the loop, */
  }                             /* otherwise consume ',' */
  if (lyrcnt < 2)                    SCN_ERROR(scan, E_LYRCNT);
  if ((outcnt > 0) && (n != outcnt)) SCN_ERROR(scan, E_UNITCNT);
  SCN_CHAR(scan, ';');          /* consume ';' */
  return lyrcnt;                /* return the number of layers */
}  /* getucnts() */

/*--------------------------------------------------------------------*/

static int getscls (MLP *mlp, SCANNER *scan)
{                               /* --- get the input scalings */
  assert(mlp && scan);          /* check the function arguments */
  if (mlp->incnt <= 0) return 0;/* check whether there are inputs */
  if ((scn_token(scan) != T_ID) /* check whether 'scales' follows */
  ||  (strcmp(scn_value(scan), "scales") != 0))
    mlp->nst = nst_create(mlp->incnt);
  else                          /* create or parse scaling parameters */
    mlp->nst = nst_parse(scan, mlp->incnt);
  return (mlp->nst) ? 0 : 1;    /* return an error indicator */
}  /* getscls() */

/*--------------------------------------------------------------------*/

static int getwgts (MLP *mlp, SCANNER *scan)
{                               /* --- get the connection weights */
  int      l;                   /* loop variable  for layers */
  DIMID    k, n;                /* loop variables for weights */
  MLPLAYER *layer;              /* to traverse the network layers */
  double   **wp, *w;            /* to traverse the weights */

  assert(mlp && scan);          /* check the function arguments */
  if ((scn_token(scan) != T_ID) /* check for 'weights' */
  ||  (strcmp(scn_value(scan), "weights") != 0))
    SCN_ERROR(scan, E_STREXP, "weights");
  SCN_NEXT(scan);               /* consume 'weights' */
  SCN_CHAR(scan, '=');          /* consume '=' */
  layer = mlp->layers;          /* traverse the network layers */
  for (l = mlp->lyrcnt-1; --l >= 0; layer++) {
    SCN_CHAR(scan, '{');        /* consume '{' */
    wp = layer->wgts;           /* traverse the units of a layer */
    for (k = layer->outcnt; --k >= 0; ) {
      SCN_CHAR(scan, '{');      /* consume '{' */
      w = *wp++;                /* traverse the inputs of a unit */
      for (n = layer->incnt +1; --n >= 0; ) {
        if (scn_token(scan) != T_NUM) SCN_ERROR(scan, E_NUMEXP);
        *w++ = strtod(scn_value(scan), NULL);
        SCN_NEXT(scan);         /* get the connection weight */
        if (n > 0) { SCN_CHAR(scan, ','); }
      }                         /* if not bis value, consume ',' */
      SCN_CHAR(scan, '}');      /* consume '}' */
      if (k > 0) { SCN_CHAR(scan, ','); }
    }                           /* if not last unit, consume ',' */
    SCN_CHAR(scan, '}');        /* consume '}' */
    if (l > 0) { SCN_CHAR(scan, ','); }
  }                             /* if not output layer, consume ',' */
  SCN_CHAR(scan, ';');          /* consume ';' */
  return 0;                     /* return 'ok' */
}  /* getwgts() */

/*--------------------------------------------------------------------*/

static int getrngs (MLP *mlp, SCANNER *scan)
{                               /* --- get the output ranges */
  DIMID  k;                     /* loop variable */
  double *min, *max;            /* to traverse the minima/maxima */

  assert(mlp && scan);          /* check the function arguments */
  if ((scn_token(scan) != T_ID) /* check whether 'ranges' follows */
  ||  (strcmp(scn_value(scan), "ranges") != 0))
    return 0;                   /* if not, abort the function */
  SCN_NEXT(scan);               /* consume 'ranges' */
  SCN_CHAR(scan, '=');          /* consume '=' */
  min = mlp->mins;              /* traverse the vectors of */
  max = mlp->maxs;              /* minimal and maximal output values */
  for (k = mlp->outcnt; --k >= 0; ) {
    SCN_CHAR(scan, '[');        /* consume '[' */
    if (scn_token(scan) != T_NUM) SCN_ERROR(scan, E_NUMEXP);
    *min++ = strtod(scn_value(scan), NULL);
    SCN_NEXT(scan);             /* consume the minimal output value */
    SCN_CHAR(scan, ',');        /* consume '[' */
    if (scn_token(scan) != T_NUM) SCN_ERROR(scan, E_NUMEXP);
    *max++ = strtod(scn_value(scan), NULL);
    SCN_NEXT(scan);             /* consume the maximal output value */
    SCN_CHAR(scan, ']');        /* consume '[' */
    if (k > 0) { SCN_CHAR(scan, ','); }
  }                             /* if not last unit, consume ',' */
  SCN_CHAR(scan, ';');          /* consume ';' */
  return 0;                     /* return 'ok' */
}  /* getrngs() */

/*--------------------------------------------------------------------*/

MLP* mlp_parse (SCANNER *scan)
{                               /* --- parse a multilayer perceptron */
  int   lyrcnt;                 /* number of layers */
  DIMID ucnts[MLP_MAXLAYER];    /* number of units per layer */
  MLP   *mlp;                   /* parsed multilayer perceptron */

  assert(scan);                 /* check the function argument */
  scn_setmsgs(scan, msgs, (int)(sizeof(msgs)/sizeof(*msgs)));
  scn_first(scan);              /* set messages, get first token */
  lyrcnt = getucnts(scan, ucnts, 0, 0);
  if (lyrcnt < 0) return NULL;  /* get the number of units per layer */
  mlp = create(lyrcnt, ucnts);  /* create a multilayer perceptron */
  if (!mlp) return NULL;
  if ((getscls(mlp, scan) != 0)    /* read input scalings, */
  ||  (getwgts(mlp, scan) != 0)    /* the connection weights, */
  ||  (getrngs(mlp, scan) != 0)) { /* and the output ranges */
    mlp_delete(mlp); return NULL; }
  return mlp;                   /* return the parsed network */
}  /* mlp_parse() */

/*--------------------------------------------------------------------*/
#ifdef MLP_EXTFN

static ATTID header (SCANNER *scan, ATTSET *attset)
{                               /* --- parse header */
  ATTID i;                      /* target identifier */
  int   t;                      /* buffer for a token */

  assert(attset && scan);       /* check the function arguments */
  if ((scn_token(scan) != T_ID) /* check for 'mlp' */
  ||  (strcmp(scn_value(scan), "mlp") != 0))
    SCN_ERROR(scan, E_STREXP, "mlp");
  SCN_NEXT(scan);               /* consume 'mlp' */
  SCN_CHAR(scan, '(');          /* consume '(' */
  t = scn_token(scan);          /* check for an attribute name */
  if ((t != T_ID) && (t != T_NUM)) SCN_ERROR(scan, E_ATTEXP);
  i = as_attid(attset, scn_value(scan));
  if (i < 0)                       SCN_ERROR(scan, E_UNKATT);
  SCN_NEXT(scan);               /* consume the attribute name */
  SCN_CHAR(scan, ')');          /* consume ')' */
  SCN_CHAR(scan, '=');          /* consume '=' */
  SCN_CHAR(scan, '{');          /* consume '{' */
  return i;                     /* return target identifier */
}  /* header() */

/*--------------------------------------------------------------------*/

static int trailer (SCANNER *scan)
{                               /* --- parse trailer */
  assert(scan);                 /* check the function argument */
  SCN_CHAR(scan, '}');          /* consume '}' */
  SCN_CHAR(scan, ';');          /* consume ';' */
  return 0;                     /* return 'ok' */
}  /* trailer() */

/*--------------------------------------------------------------------*/

MLP* mlp_parsex (SCANNER *scan, ATTMAP *attmap)
{                               /* --- parse a multilayer perceptron */
  ATTID trgid;                  /* identifier of target attribute */
  int   lyrcnt;                 /* number of layers */
  DIMID ucnts[MLP_MAXLAYER];    /* number of units per layer */
  MLP   *mlp;                   /* parsed multilayer perceptron */

  assert(scan && attmap);       /* check the function arguments */
  scn_setmsgs(scan, msgs, (int)(sizeof(msgs)/sizeof(*msgs)));
  scn_first(scan);              /* set messages, get first token */
  trgid = header(scan, am_attset(attmap));
  if (trgid < 0) return NULL;   /* parse the header and */
  am_target(attmap, trgid);     /* set the target attribute */
  lyrcnt = getucnts(scan, ucnts, am_incnt(attmap), am_outcnt(attmap));
  if (lyrcnt < 0) return NULL;  /* get the number of units per layer*/
  ucnts[0]        = am_incnt (attmap);
  ucnts[lyrcnt-1] = am_outcnt(attmap);
  mlp = create(lyrcnt, ucnts);  /* create a multilayer perceptron */
  if (!mlp) { scn_error(scan, E_NOMEM); return NULL; }
  mlp->attset = am_attset(attmap);
  mlp->attmap = attmap;         /* note the attribute set and map */
  mlp->trgatt = am_att(attmap, -1);
  if ((getscls(mlp, scan) != 0) /* read the input scalings, */
  ||  (getwgts(mlp, scan) != 0) /* the connection weights, */
  ||  (getrngs(mlp, scan) != 0) /* the output ranges, */
  ||  (trailer(scan) != 0)) {   /* and the trailer */
    mlp_delete(mlp); return NULL; }
  return mlp;                   /* return the parsed network */
}  /* mlp_parsex() */

#endif  /* #ifdef MLP_EXTFN */
#endif  /* #ifdef MLP_PARSE */
