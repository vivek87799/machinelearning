/*----------------------------------------------------------------------
  File    : mlpt.c
  Contents: multilayer perceptron training
  Author  : Christian Borgelt
  History : 2001.05.13 file created from file dti.c
            2001.05.20 first version completed
            2001.07.17 adapted to modified module scanner
            2002.06.04 input normalization/scaling added
            2002.08.21 flag AS_NOXVAL set if a network is read
            2003.05.15 options -g and -z changed
            2003.08.16 slight changes in error message output
            2004.02.13 bug in verbose error output fixed
            2004.02.26 source files mmlpt.c and mlpt.c combined
            2007.01.10 use of attribute maps changed
            2007.02.14 adapted to modufied module tabread
            2007.03.16 table and matrix version combined (option -M)
            2007.10.10 evaluation of attribute directions added
            2011.12.09 adapted to modified attset and utility modules
            2011.12.14 adapted to utility module to get parameters
            2013.08.12 adapted to definitions ATTID, VALID, TPLID etc.
            2013.08.29 adapted to new function as_target()
            2014.10.07 bug in handling option -q fixed (input norm.)
            2014.10.24 changed from LGPL license to MIT license
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#ifndef MAT_RDWR
#define MAT_RDWR
#endif
#include "matrix.h"
#ifndef AS_READ
#define AS_READ
#endif
#ifndef AS_PARSE
#define AS_PARSE
#endif
#ifndef AS_DESC
#define AS_DESC
#endif
#include "attset.h"
#ifndef TAB_READ
#define TAB_READ
#endif
#include "table.h"
#ifndef MLP_EXTFN
#define MLP_EXTFN
#endif
#ifndef MLP_PARSE
#define MLP_PARSE
#endif
#include "mlp.h"
#include "random.h"
#include "params.h"
#include "error.h"
#ifdef STORAGE
#include "storage.h"
#endif

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define PRGNAME     "mlpt"
#define DESCRIPTION "multilayer perceptron training"
#define VERSION     "version 2.2 (2014.10.24)         " \
                    "(c) 2001-2014   Christian Borgelt"

/* --- error codes --- */
/* error codes 0 to -5 defined in attset.h */
#define E_OPTION     (-6)       /* unknown option */
#define E_OPTARG     (-7)       /* missing option argument */
#define E_ARGCNT     (-8)       /* wrong number of arguments */
#define E_PARSE      (-9)       /* parse errors on input file */
#define E_TPLCNT    (-10)       /* data file is empty */
#define E_ATTCNT    (-11)       /* no usable attributes found */
#define E_UNKTRG    (-12)       /* missing target attribute */
#define E_MULTRG    (-13)       /* multiple target attributes */
#define E_LAYERS    (-14)       /* invalid number of layers */
#define E_UNITS     (-15)       /* invalid number of units */
#define E_METHOD    (-16)       /* invalid weight update method */
#define E_LRATE     (-17)       /* invalid learning rate */
#define E_LPARAM    (-18)       /* invalid learning parameter */
#define E_MOMENT    (-19)       /* invalid momentum coefficient */
#define E_EPOCHS    (-20)       /* invalid number of epochs */

#define INPUT       "input"
#define HIDDEN      "hidden"
#define OUTPUT      "output"

#define SEC_SINCE(t)  ((double)(clock()-(t)) /(double)CLOCKS_PER_SEC)

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- mode information --- */
  int  code;                    /* code        of update mode */
  char *name;                   /* name        of update mode */
  char *desc;                   /* description of update mode */
} MODEINFO;                     /* (mode information) */

/*----------------------------------------------------------------------
  Constants
----------------------------------------------------------------------*/
static const char *errmsgs[] = {   /* error messages */
  /* E_NONE      0 */  "no error",
  /* E_NOMEM    -1 */  "not enough memory",
  /* E_FOPEN    -2 */  "cannot open file %s",
  /* E_FREAD    -3 */  "read error on file %s",
  /* E_FWRITE   -4 */  "write error on file %s",
  /* E_STDIN    -5 */  "double assignment of standard input",
  /* E_OPTION   -6 */  "unknown option -%c",
  /* E_OPTARG   -7 */  "missing option argument",
  /* E_ARGCNT   -8 */  "wrong number of arguments",
  /* E_PARSE    -9 */  "parse error(s) on file %s",
  /* E_TPLCNT  -10 */  "data file is empty",
  /* E_ATTCNT  -11 */  "no (usable) attributes (need at least 1)",
  /* E_UNKTRG  -12 */  "missing target '%s' in file %s",
  /* E_MULTRG  -13 */  "multiple target attributes",
  /* E_LAYERS  -14 */  "invalid number of layers",
  /* E_UNITS   -15 */  "invalid number of %s units",
  /* E_METHOD  -16 */  "invalid weight update method %s",
  /* E_LRATE   -17 */  "invalid learning rate %g",
  /* E_LPARAM  -18 */  "invalid learning parameter %g",
  /* E_MOMENT  -19 */  "invalid momentum coefficient %g",
  /* E_EPOCHS  -20 */  "invalid number of epochs %"DIMID_FMT,
  /*           -21 */  "unknown error",
};

static const MODEINFO updtab[] = {    /* table of update methods */
  { MLP_STANDARD,  "bkprop",    "standard backpropagation"            },
  { MLP_ADAPTIVE,  "supersab",  "super self-adaptive backpropagation" },
  { MLP_RESILIENT, "rprop",     "resilient backpropagation"           },
  { MLP_QUICK,     "quick",     "quick backpropagation"               },
  { MLP_MANHATTAN, "manhattan", "manhattan training"                  },
  { -1,            NULL,        NULL  /* sentinel */                  },
};

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
static CCHAR   *prgname;        /* program name for error messages */
static SCANNER *scan   = NULL;  /* scanner (for domain descriptions) */
static ATTSET  *attset = NULL;  /* attribute set */
static ATTMAP  *attmap = NULL;  /* attribute map */
static TABREAD *tread  = NULL;  /* table reader */
static TABLE   *table  = NULL;  /* table  of training patterns */
static MATRIX  *matrix = NULL;  /* matrix of training patterns */
static MLP     *mlp    = NULL;  /* multilayer perceptron */
static FILE    *out    = NULL;  /* network output file */

/*----------------------------------------------------------------------
  Functions
----------------------------------------------------------------------*/

#ifndef NDEBUG
  #undef  CLEANUP               /* clean up memory and close files */
  #define CLEANUP \
  if (mlp)    mlp_deletex(mlp, 0);  \
  if (matrix) mat_delete(matrix);   \
  if (table)  tab_delete(table, 0); \
  if (attmap) am_delete(attmap, 0); \
  if (attset) as_delete(attset);    \
  if (tread)  trd_delete(tread, 1); \
  if (scan)   scn_delete(scan,  1); \
  if (out && (out != stdout)) fclose(out);
#endif

GENERROR(error, exit)           /* generic error reporting function */

/*--------------------------------------------------------------------*/

static void help (void)
{                               /* --- print help on methods */
  int i;                        /* loop variable */

  fprintf(stderr, "\n");        /* terminate startup message */
  printf("list of parameter update methods (option -a#)\n");
  printf("  name        parameter update method\n");
  for (i = 0; updtab[i].name; i++) /* list of update method */
    printf("  %-10s  %s\n", updtab[i].name, updtab[i].desc);
  printf("For method bkprop a momentum term and for methods supersab "
         "and rprop\na growth and a shrink factor and a minimum and "
         "a maximum value for\nthe change/learning rate may be "
         "specified (options -g# and -z#).\n");
  exit(0);                      /* describe the weight update methods */
}  /* help() */                 /* and abort the program */

/*--------------------------------------------------------------------*/

static int getucnts (DIMID *ucnts, char *s, char **end)
{                               /* --- get numbers of units per layer */
  int   lyrcnt = 0;             /* number of (hidden) layers */
  DIMID n;                      /* number of units */

  do {                          /* number of units read loop */
    if (lyrcnt >= MLP_MAXLAYER)  return E_LAYERS;
    n = strtodim(s, end);       /* get next number of units */
    if ((*end == s) || (n <= 0)) return E_UNITS;
    ucnts[++lyrcnt] = n;        /* store the number of units */
    s = *end;                   /* and consume the string */
  } while (*s++ == ':');        /* while another number follows */
  return lyrcnt +2;             /* return the number of layers */
}  /* getucnts() */             /* (including input and output layer) */

/*--------------------------------------------------------------------*/

static double geterr (MLP *mlp, TABLE *table, double *err)
{                               /* --- determine network error */
  TPLID  i;                     /* loop variable for tuples */
  ATTID  trgid;                 /* target identifier */
  VALID  k;                     /* buffer for target value */
  int    type;                  /* type of the target attribute */
  TUPLE  *tpl;                  /* to traverse the tuples */
  WEIGHT wgt;                   /* tuple weight */
  double sse;                   /* sum of (squared) errors */
  INST   res;                   /* target value and network result */

  assert(mlp && table && err);  /* check the function arguments */
  trgid = mlp_trgid(mlp);       /* get the target att. and its type */
  type  = att_type(mlp_trgatt(mlp));
  for (*err = sse = 0.0, i = tab_tplcnt(table); --i >= 0; ) {
    tpl = tab_tpl(table, i);    /* traverse the training patterns */
    mlp_inputx(mlp, tpl);       /* present the pattern to the network */
    mlp_exec(mlp, NULL, NULL);  /* execute the neural network */
    mlp_targetx(mlp, tpl);      /* set the target output value */
    wgt  = tpl_getwgt(tpl);     /* and sum the squared errors */
    sse += wgt *mlp_error(mlp, NULL);
    if (type != AT_NOM) continue;  /* for nominal target attributes: */
    mlp_result(mlp, &res, NULL);   /* get the classification result */
    k = tpl_colval(tpl, trgid)->n;
    if (!isnone(k) && (k != res.n)) *err += wgt;
  }                             /* sum the misclassifications */
  return sse;                   /* return the sum of squared errors */
}  /* geterr() */

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k = 0;             /* loop variables, counter */
  char    *s;                   /* to traverse options */
  CCHAR   **optarg = NULL;      /* option argument */
  CCHAR   *fn_dom  = NULL;      /* name of domain/input network file */
  CCHAR   *fn_hdr  = NULL;      /* name of table header file */
  CCHAR   *fn_tab  = NULL;      /* name of table file */
  CCHAR   *fn_mlp  = NULL;      /* name of output network file */
  CCHAR   *fn_inp  = NULL;      /* name of input  network file */
  CCHAR   *recseps = NULL;      /* record  separators */
  CCHAR   *fldseps = NULL;      /* field   separators */
  CCHAR   *blanks  = NULL;      /* blank   characters */
  CCHAR   *comment = NULL;      /* comment characters */
  CCHAR   *trgname = NULL;      /* target attribute name */
  CCHAR   *upname  = "bkprop";  /* name of update/learning method */
  int     method   = 0;         /* code of update/learning method */
  int     matinp   = 0;         /* flag for numerical matrix input */
  int     mode     = AS_ATT|AS_NOXATT|AS_NONULL;/* table read mode */
  int     lyrcnt   = 2;         /* number of layers */
  DIMID   incnt    = 0;         /* number of input  units */
  DIMID   outcnt   = 1;         /* number of output units */
  DIMID   ucnts[MLP_MAXLAYER];  /* number of units per layer */
  int     norm     = 1;         /* whether to normalize inputs */
  double  expand   = 1.0;       /* expansion factor for output ranges */
  #ifdef MLP_TANH
  double  range    = 0.5;       /* initial weight range */
  double  lrate    = 0.05;      /* learning rate */
  #else
  double  range    = 1.0;       /* initial weight range */
  double  lrate    = 0.2;       /* learning rate */
  #endif
  DIMID   epochs   = 1000, e;   /* number of epochs to train */
  DIMID   update   = 1,    u;   /* number of patterns between updates */
  DIMID   verbose  = 0,    v;   /* flag for verbose output */
  int     shuffle  = 1;         /* shuffle pattern set */
  double  term     = 0.0;       /* maximum sse for termination */
  double  raise    = 0.0;       /* raise value for derivative */
  double  moment   = 0.0;       /* momentum coefficient */
  double  growth   = 1.2;       /* growth factor for learning rate */
  double  shrink   = 0.5;       /* shrink factor for learning rate */
  double  minchg   = 0.0;       /* minimal change/learning rate */
  double  maxchg   = 2.0;       /* maximal change/learning rate */
  double  decay    = 0.0;       /* weight decay */
  double  jog      = 0.0;       /* range for weight jogging */
  int     maxlen   = 0;         /* maximal output line length */
  int     sse4nom  = 1;         /* use sse for nominal target */
  long    seed;                 /* seed for random numbers */
  double  *pat;                 /* to traverse the training patterns */
  TUPLE   *tpl;                 /* to traverse the training patterns */
  double  err;                  /* number of misclassifications */
  double  sse;                  /* sum of (squared) errors */
  ATTID   trgid;                /* id of the target column */
  ATTID   m, c;                 /* number of attributes */
  TPLID   n, r;                 /* number of data tuples */
  DIMID   p;                    /* number of patterns */
  double  w;                    /* weight of data tuples */
  clock_t t;                    /* for time measurements */

  prgname = argv[0];            /* get program name for error msgs. */

  /* --- print startup/usage message --- */
  if (argc > 1) {               /* if arguments are given */
    fprintf(stderr, "%s - %s\n", argv[0], DESCRIPTION);
    fprintf(stderr, VERSION); } /* print a startup message */
  else {                        /* if no argument is given */
    printf("usage: %s [options] [-M|domfile [-d|-h hdrfile]] "
                    "tabfile mlpfile [infile]\n", argv[0]);
    printf("%s\n", DESCRIPTION);
    printf("%s\n", VERSION);
    printf("-!       print a list of available update methods\n");
    printf("-M       input is a numerical matrix            "
                    "(default: input is a table)\n");
    printf("-U#      number of output units (with -M)       "
                    "(default: %"DIMID_FMT")\n", outcnt);
    printf("-o#      output/target attribute name           "
                    "(default: last attribute)\n");
    printf("-c#:#..  number of units per hidden layer       "
                    "(default: no hidden layer)\n");
    printf("-q       do not normalize input ranges          "
                    "(default: mean=0, var=1)\n");
    printf("-x#      expansion for output ranges            "
                    "(default: %g)\n", expand);
    printf("-S#      seed for random numbers                "
                    "(default: time)\n");
    printf("-w#      initial weight range                   "
                    "(default: %g)\n", range);
    printf("-a#      parameter update method                "
                    "(default: %s)\n", upname);
    printf("-t#      learning rate                          "
                    "(default: %g)\n", lrate);
    printf("-m#      momentum coefficient                   "
                    "(default: %g)\n", moment);
    printf("-z#:#    minimal and maximal change/lrate       "
                    "(default: %g:%g)\n", minchg, maxchg);
    printf("-g#:#    growth and shrink factor               "
                    "(default: %g:%g)\n", growth, shrink);
    printf("-i#      flat spot elimination                  "
                    "(default: %g)\n", raise);
    printf("-y#      weight decay factor                    "
                    "(default: %g)\n", decay);
    printf("-j#      range for weight jogging               "
                    "(default: %g)\n", jog);
    printf("-s       do not shuffle patterns                "
                    "(default: once per epoch)\n");
    printf("-e#      maximum number of update epochs        "
                    "(default: %"DIMID_FMT")\n", epochs);
    printf("-k#      patterns between two updates           "
                    "(default: %"DIMID_FMT")\n", update);
    printf("-T#      error for termination                  "
                    "(default: %g)\n", term);
    printf("-E       use misclassification error            "
                    "(default: sse)\n");
    printf("-l#      output line length                     "
                    "(default: no limit)\n");
    printf("-P#      verbose output (print sse every # epochs)\n");
    printf("-r#      record  separators                     "
                    "(default: \"\\n\")\n");
    printf("-f#      field   separators                     "
                    "(default: \" \\t,\")\n");
    printf("-b#      blank   characters                     "
                    "(default: \" \\t\\r\")\n");
    printf("-C#      comment characters                     "
                    "(default: \"#\")\n");
    printf("domfile  file containing domain descriptions\n"
           "         (and maybe a pretrained network)\n");
    printf("-d       use default header "
                    "(field names = field numbers)\n");
    printf("-h       read table header (field names) from hdrfile\n");
    printf("hdrfile  file containing table header (field names)\n");
    printf("tabfile  table file to read "
                    "(field names in first record)\n");
    printf("mlpfile  file to write multilayer perceptron to\n");
    printf("infile   file to read  multilayer perceptron from "
                    "(only with -M)\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */

  /* remaining option characters: n p u v A B D F-L N O Q R V W X Y Z */

  /* --- evaluate arguments --- */
  seed = (long)time(NULL);      /* and get a default seed value */
  for (i = 1; i < argc; i++) {  /* traverse arguments */
    s = argv[i];                /* get option argument */
    if (optarg) { *optarg = s; optarg = NULL; continue; }
    if ((*s == '-') && *++s) {  /* -- if argument is an option */
      while (1) {               /* traverse characters */
        switch (*s++) {         /* evaluate option */
          case '!': help();                              break;
          case 'M': matinp  = 1;                         break;
          case 'U': outcnt  = (DIMID)strtol(s, &s, 0);   break;
          case 'o': optarg  = &trgname;                  break;
          case 'c': lyrcnt  = getucnts(ucnts, s, &s);    break;
          case 'q': norm    = 0;                         break;
          case 'x': expand  =        strtod(s, &s);      break;
          case 'S': seed    =        strtol(s, &s, 0);   break;
          case 'w': range   =   fabs(strtod(s, &s));     break;
          case 'a': optarg  = &upname;                   break;
          case 't': lrate   =        strtod(s, &s);      break;
          case 'm': moment  =        strtod(s, &s);      break;
          case 'z': getdbls(s, &s, 2, &minchg, &maxchg); break;
          case 'g': getdbls(s, &s, 2, &growth, &shrink); break;
          case 'i': raise   =        strtod(s, &s);      break;
          case 'y': decay   =        strtod(s, &s);      break;
          case 'j': jog     =        strtod(s, &s);      break;
          case 's': shuffle = 0;                         break;
          case 'e': epochs  = (DIMID)strtol(s, &s, 0);   break;
          case 'k': update  = (DIMID)strtol(s, &s, 0);   break;
          case 'T': term    =        strtod(s, &s);      break;
          case 'E': sse4nom = 0;                         break;
          case 'l': maxlen  = (int)  strtol(s, &s, 0);   break;
          case 'P': verbose = (int)  strtol(s, &s, 0);   break;
          case 'r': optarg  = &recseps;                  break;
          case 'f': optarg  = &fldseps;                  break;
          case 'b': optarg  = &blanks;                   break;
          case 'C': optarg  = &comment;                  break;
          case 'd': mode   |= AS_DFLT;                   break;
          case 'h': optarg  = &fn_hdr;                   break;
          default : error(E_OPTION, *--s);               break;
        }                       /* set option variables */
        if (!*s) break;         /* if at end of string, abort loop */
        if (optarg) { *optarg = s; optarg = NULL; break; }
      } }                       /* get option argument */
    else {                      /* -- if argument is no option */
      switch (k++) {            /* evaluate non-option */
        case  0: fn_dom = s;      break;
        case  1: fn_tab = s;      break;
        case  2: fn_mlp = s;      break;
        default: error(E_ARGCNT); break;
      }                         /* note file names */
    }
  }
  if (optarg) error(E_OPTARG);  /* check option argument */
  if (matinp) {                 /* if matrix version */
    if ((k != 2) && (k != 3))   /* check the number */
      error(E_ARGCNT);          /* of arguments */
    fn_inp = fn_mlp;            /* shift the file names */
    fn_mlp = fn_tab;            /* (as there is no domain file) */
    fn_tab = fn_dom; fn_dom = NULL;
    if ((!fn_tab || !*fn_tab) && (fn_inp && !*fn_inp))
      error(E_STDIN); }         /* stdin must not be used twice */
  else {                        /* if table version */
    if (k != 3) error(E_ARGCNT);/* check number of arguments */
    if (fn_hdr && (strcmp(fn_hdr, "-") == 0))
      fn_hdr = "";              /* convert "-" to "" */
    i = ( fn_hdr && !*fn_hdr) ? 1 : 0;
    if  (!fn_dom || !*fn_dom) i++;
    if  (!fn_tab || !*fn_tab) i++;
    if (i > 1) error(E_STDIN);  /* check assignments of stdin: */
  }                             /* stdin must not be used twice */
  for (i = 0; updtab[i].name; i++)
    if (strcmp(updtab[i].name, upname) == 0) break;
  if (!updtab[i].name) error(E_METHOD, upname);
  method = i;                   /* code the update method */
  if (outcnt <  0) error(E_UNITS,  OUTPUT);
  if (lyrcnt <  0) error(lyrcnt,   HIDDEN);
  if (expand <  1) error(E_LPARAM, expand);
  if (lrate  <= 0) error(E_LRATE,  lrate);
  if (raise  <  0) error(E_LPARAM, raise);
  if (growth <  1) error(E_LPARAM, growth);
  if (shrink >  1) error(E_LPARAM, shrink);
  if (minchg <  0) error(E_LPARAM, minchg);
  if (maxchg <= 0) error(E_LPARAM, maxchg);
  if ((moment < 0) || (moment >= 1)) error(E_MOMENT, moment);
  if ((decay  < 0) || (decay  >= 1)) error(E_LPARAM, decay);
  if (epochs  < 0) error(E_EPOCHS, epochs);
  rseed((unsigned)seed);        /* init. the random number generator */
  fputc('\n', stderr);          /* terminate the startup message */

  if (matinp) {                 /* if matrix version */
    /* --- parse multilayer perceptron --- */
    if (k <= 2) m = -1;         /* if no input, clear att. counter */
    else {                      /* if an input file is given */
      t    = clock();           /* start the timer */
      scan = scn_create();      /* create a scanner */
      if (!scan) error(E_NOMEM);/* for the input network */
      if (scn_open(scan, NULL, fn_inp) != 0)
        error(E_FOPEN, scn_name(scan));
      fprintf(stderr, "reading %s ... ", scn_name(scan));
      mlp = mlp_parse(scan);    /* parse the input network */
      if (!mlp || !scn_eof(scan, 1)) error(E_PARSE, scn_name(scan));
      scn_delete(scan, 1);      /* delete the scanner */
      scan   = NULL;            /* and clear the variable */
      incnt  = mlp_incnt(mlp);  /* get number of input */
      outcnt = mlp_outcnt(mlp); /* and output units and */
      m      = incnt +outcnt;   /* compute the number of variables */
      fprintf(stderr, "[%"DIMID_FMT" unit(s),",   mlp_unitcnt(mlp));
      fprintf(stderr, " %"DIMID_FMT" weight(s)]", mlp_wgtcnt(mlp));
      fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
    }                           /* print a success message */

    /* --- read training patterns --- */
    tread = trd_create();       /* create a table reader and */
    if (!tread) error(E_NOMEM); /* set the separator characters */
    trd_allchs(tread, recseps, fldseps, blanks, "", comment);
    t = clock();                /* start timer, open input file */
    if (trd_open(tread, NULL, fn_tab) != 0)
      error(E_FOPEN, trd_name(tread));
    fprintf(stderr, "reading %s ... ", trd_name(tread));
    k = mat_readx(&matrix, tread, 0, m);
    if (k) error(k, TRD_INFO(tread));
    trd_delete(tread, 1);       /* read the training patterns, */
    tread = NULL;               /* then close the input file */
    m = mat_colcnt(matrix);     /* get the number of variables */
    p = mat_rowcnt(matrix);     /* and the number of patterns */
    fprintf(stderr, "[%"DIMID_FMT" variable(s),", m);
    fprintf(stderr, " %"DIMID_FMT" pattern(s)]",  p);
    fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
    if (m <= 0) error(E_ATTCNT);/* check for at least one attribute */
    if (p <= 0) error(E_TPLCNT);/* check for at least one pattern */

    /* --- create multilayer perceptron --- */
    if (!mlp) {                 /* if no input network is given */
      t = clock();              /* start the timer, print message */
      fprintf(stderr, "creating network ... ");
      incnt = m -outcnt;        /* get and check the number of inputs */
      if (incnt <= 0) error(E_UNITS, INPUT);
      ucnts[0]        = incnt;  /* store the number of input  units */
      ucnts[lyrcnt-1] = outcnt; /* and   the number of output units */
      mlp = mlp_create(lyrcnt, ucnts);
      if (!mlp) error(E_NOMEM); /* create a multilayer perceptron */
      mlp_init(mlp,drand,range);/* initialize the connection weights */
      for (p = mat_rowcnt(matrix); --p >= 0; ) {
        pat = mat_row(matrix, p);
        mlp_reg(mlp, (norm) ? pat : NULL, pat +incnt, 1);
      }                         /* register the input/target pairs */
      mlp_reg(mlp,NULL,NULL,0); /* compute normalization */
      if (expand != 1)          /* expand the output value ranges */
        for (c = outcnt; --c >= 0; )
          mlp_expand(mlp, c, expand);
      fprintf(stderr, "[%"DIMID_FMT" units,",   mlp_unitcnt(mlp));
      fprintf(stderr, " %"DIMID_FMT" weights]", mlp_wgtcnt(mlp));
      fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
    } }                         /* print a success message */

  else {                        /* if table version */
    /* --- parse domain descriptions --- */
    attset = as_create("domains", att_delete);
    if (!attset) error(E_NOMEM);/* create an attribute set */
    scan = scn_create();        /* create a scanner */
    if (!scan)   error(E_NOMEM);/* for the domain file */
    t = clock();                /* start timer, open input file */
    if (scn_open(scan, NULL, fn_dom) != 0)
      error(E_FOPEN, scn_name(scan));
    fprintf(stderr, "reading %s ... ", scn_name(scan));
    if (as_parse(attset, scan, AT_ALL, 1) != 0)
      error(E_PARSE, scn_name(scan));
    m = as_attcnt(attset);      /* parse domain descriptions */

    /* --- get target attribute --- */
    if (!scn_eof(scan, 0)) trgname = NULL;
    trgid = as_target(attset, trgname, +1);
    if (trgid < 0) error(trgname ? E_UNKTRG : E_MULTRG, trgname);

    /* --- parse multilayer perceptron --- */
    if (scn_eof(scan, 0))       /* if there is no input network */
      fprintf(stderr, "[%"ATTID_FMT" attribute(s)]", m);
    else {                      /* if there is an input network */
      mode  |= AS_NOXVAL;       /* prevent nominal domain extensions */
      attmap = am_create(attset, 0, 1.0);
      if (!attmap) error(E_NOMEM);    /* create an attribute map */
      am_target(attmap, trgid);       /* and set the target att. */
      mlp = mlp_parsex(scan, attmap); /* parse the neural network */
      if (!mlp || !scn_eof(scan, 1)) error(E_PARSE, scn_name(scan));
      fprintf(stderr, "[%"DIMID_FMT" units,",   mlp_unitcnt(mlp));
      fprintf(stderr, " %"DIMID_FMT" weights]", mlp_wgtcnt(mlp));
    }                           /* print a success message */
    scn_delete(scan, 1);        /* delete the scanner and */
    scan = NULL;                /* clear the scanner variable */
    fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
    if (m <= 0) error(E_ATTCNT);/* check for at least one attribute */

    /* --- read table header --- */
    tread = trd_create();       /* create a table reader and */
    if (!tread) error(E_NOMEM); /* set the separator characters */
    trd_allchs(tread, recseps, fldseps, blanks, "", comment);
    if (fn_hdr) {               /* if a header file is given */
      t = clock();              /* start timer, open input file */
      if (trd_open(tread, NULL, fn_hdr) != 0)
        error(E_FOPEN, trd_name(tread));
      fprintf(stderr, "reading %s ... ", trd_name(tread));
      k = as_read(attset, tread, (mode & ~AS_DFLT) | AS_ATT);
      if (k < 0) error(-k, as_errmsg(attset, NULL, 0));
      trd_close(tread);         /* read table header, close file */
      m = as_attcnt(attset);    /* get the number of attributes */
      fprintf(stderr, "[%"ATTID_FMT" attribute(s)]", m);
      fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
      mode &= ~(AS_ATT|AS_DFLT);/* print a success message and */
    }                           /* remove the attribute flag */

    /* --- read table --- */
    t = clock();                /* start timer, open input file */
    if (trd_open(tread, NULL, fn_tab) != 0)
      error(E_FOPEN, trd_name(tread));
    fprintf(stderr, "reading %s ... ", trd_name(tread));
    table = tab_create("table", attset, tpl_delete);
    if (!table) error(E_NOMEM); /* create a data table */
    k = tab_read(table, tread, mode);
    if (k < 0) error(-k, tab_errmsg(table, NULL, 0));
    trd_delete(tread, 1);       /* read the table body and */
    tread = NULL;               /* delete the table reader */
    m = tab_colcnt(table);      /* get the number of attributes */
    n = tab_tplcnt(table);      /* and the number of data tuples */
    w = tab_tplwgt(table);      /* and print a success message */
    fprintf(stderr, "[%"ATTID_FMT" attribute(s),", m);
    fprintf(stderr, " %"TPLID_FMT, n);
    if (w != (double)n) fprintf(stderr, "/%g", w);
    fprintf(stderr, " tuple(s)] done [%.2fs].\n", SEC_SINCE(t));
    if (n <= 0) error(E_TPLCNT);

    /* --- create multilayer perceptron --- */
    if (!mlp) {                 /* if no input network is given */
      t = clock();              /* start the timer, print message */
      fprintf(stderr, "creating network ... ");
      attmap = am_create(attset, 0, 1.0);
      if (!attmap) error(E_NOMEM);   /* create an attribute map */
      am_target(attmap, trgid); /* and set the target attribute */
      mlp = mlp_createx(attmap, lyrcnt, ucnts);
      if (!mlp) error(E_NOMEM); /* create a multilayer perceptron */
      mlp_init(mlp,drand,range);/* initialize the connection weights */
      for (r = 0; r < n; r++)   /* determine the ranges of values */
        mlp_regx(mlp, tab_tpl(table, r), norm);
      mlp_regx(mlp, NULL, norm);/* compute the scaling parameters */
      if (expand != 1)          /* expand the output value ranges */
        for (c = 0; c < outcnt; c++) mlp_expand(mlp, c, expand);
      fprintf(stderr, "[%"DIMID_FMT" units,",   mlp_unitcnt(mlp));
      fprintf(stderr, " %"DIMID_FMT" weights]", mlp_wgtcnt(mlp));
      fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
    }                           /* print a success message */
  }                             /* if (matinp) .. else .. */

  /* --- train multilayer perceptron --- */
  t = clock();                  /* start the timer */
  fprintf(stderr, "training network ... ");
  mlp_method (mlp, method);     /* set the weight update method, */
  mlp_raise  (mlp, raise);      /* the derivative raise value, */
  mlp_lrate  (mlp, lrate);      /* the learning rate, */
  mlp_factors(mlp, growth, shrink); /* the growth and shrink factors, */
  mlp_limits (mlp, minchg, maxchg); /* the min. and max. change, */
  mlp_moment (mlp, moment);     /* the momentum coefficient, */
  mlp_decay  (mlp, decay);      /* and the weight decay factor */
  mlp_setup  (mlp);             /* set up network for training */
  u = update; v = 0;            /* and initialize the counters */
  for (e = 0; e < epochs; e++){ /* do "epochs" epochs of training */
    if (matinp) {               /* if matrix version */
      if (shuffle)              /* shuffle the training patterns */
        mat_shuffle(matrix, drand);
      for (sse = 0, p = mat_rowcnt(matrix); --p >= 0; ) {
        pat = mat_row(matrix, p);  /* traverse the training patterns */
        mlp_exec(mlp, pat, NULL);  /* execute the neural network and */
        sse += mlp_bkprop(mlp, pat +incnt);    /* do error backprop. */
        if ((update > 0) && (--u <= 0)) {
          u = update; mlp_update(mlp); }
      } }                       /* update after 'update' patterns */
    else {                      /* if table version */
      if (shuffle)              /* shuffle the training patterns */
        tab_shuffle(table, 0, TPLID_MAX, drand);
      for (sse = 0, n = tab_tplcnt(table); --n >= 0; ) {
        tpl = tab_tpl(table,n); /* traverse the training patterns */
        mlp_inputx(mlp, tpl);   /* and enter them into the network */
        mlp_exec(mlp,NULL,NULL);/* execute the neural network */
        mlp_targetx(mlp, tpl);  /* set the target output value and */
        sse += mlp_bkprop(mlp, NULL);  /* do error backpropagation */
        if ((update > 0) && (--u <= 0)) {
          u = update; mlp_update(mlp); }
      }                         /* update after 'update' patterns */
    }                           /* if (matinp) .. else .. */
    if ((term >= 0)             /* if termination error set or */
    || (verbose && (--v <= 0))){/* if a verbose output is requested */
      if (matinp && !sse4nom)   /* compute the misclassifications */
        geterr(mlp, table, &sse);
      if (sse <= term) break;   /* if error is small enough, abort */
      if (verbose)              /* if verbose output requested */
        fprintf(stderr, "%15g\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b", sse);
      v = verbose;              /* print sum of (squared) errors */
    }                           /* every 'verbose' epochs */
    if (update <= 0)            /* if no number of patterns is given, */
      mlp_update(mlp);          /* update once in each epoch */
    if (jog > 0)                /* if a range for weight jogging */
      mlp_jog(mlp, drand, jog); /* is given, jog the weights */
  }
  if (verbose)                  /* clear verbose error output */
    fprintf(stderr, "               \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");

  /* --- compute sse of trained network --- */
  if (matinp) {                 /* if matrix version */
    for (sse = err = 0, p = mat_rowcnt(matrix); --p >= 0; ) {
      pat = mat_row(matrix, p); /* traverse the training patterns */
      mlp_exec(mlp, pat, NULL); /* and execute the neural network */
      sse += mlp_error(mlp, pat +incnt);
    } }                         /* sum the squared errors */
  else {                        /* if table version */
    sse = geterr(mlp, table, &err);
  }                             /* compute the number of errors */
  fprintf(stderr, "[%"DIMID_FMT" epoch(s)]", e);
  fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- describe multilayer perceptron --- */
  t = clock();                  /* start timer, open output file */
  if (fn_mlp && (strcmp(fn_mlp, "-") == 0)) fn_mlp = "";
  if (fn_mlp && *fn_mlp) { out = fopen(fn_mlp, "w"); }
  else                   { out = stdout; fn_mlp = "<stdout>"; }
  fprintf(stderr, "writing %s ... ", fn_mlp);
  if (!out) error(E_FOPEN, fn_mlp);
  if (!matinp) {                /* if table version */
    if (as_desc(attset, out, AS_TITLE|AS_IVALS, maxlen) != 0)
      error(E_FWRITE, fn_mlp);  /* describe attribute domains */
    fprintf(out, "\n");         /* leave one line empty */
  }
  if (mlp_desc(mlp, out, MLP_TITLE|MLP_INFO, maxlen) != 0)
    error(E_FWRITE, fn_mlp);    /* describe the multilayer perceptron */
  if (out && (((out == stdout) ? fflush(out) : fclose(out)) != 0))
    error(E_FWRITE, fn_mlp);    /* close the output file and */
  out = NULL;                   /* print a success message */
  fprintf(stderr, "[sse: %g", sse);
  if (!matinp && (att_type(mlp_trgatt(mlp)) == AT_NOM))
    fprintf(stderr, ", %g error(s)", err);
  fprintf(stderr, "] done [%.2fs].\n", SEC_SINCE(t));

  /* --- clean up --- */
  CLEANUP;                      /* clean up memory and close files */
  SHOWMEM;                      /* show (final) memory usage */
  return 0;                     /* return 'ok' */
}  /* main() */
