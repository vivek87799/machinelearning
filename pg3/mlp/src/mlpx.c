/*----------------------------------------------------------------------
  File    : mlpx.c
  Contents: multilayer perceptron execution
  Author  : Christian Borgelt
  History : 2001.05.13 file created from file dtx.c
            2001.05.20 first version completed
            2001.07.17 adapted to modified module scan
            2002.09.18 bug concerning missing target fixed
            2003.02.02 bug in alignment in connection with -d fixed
            2003.04.23 missing AS_MARKED added for second reading
            2003.08.16 slight changes in error message output
            2003.10.24 bug in output of integer targets fixed
            2004.02.26 source files mmlpx.c and mlpx.c combined
            2007.01.10 usage of attribute maps changed
            2007.03.16 table and matrix version combined
            2007.03.21 format string moved to result structure
            2011.12.13 adapted to modified attset and utility modules
            2011.12.15 processing without table reading improved
            2013.06.08 missing flag AS_MARKED added to table read mode
            2013.08.12 adapted to definitions ATTID, VALID, TPLID etc.
            2013.08.20 output format changed to significant digits
            2013.08.30 missing deallocation of pattern buffer added
            2014.10.24 changed from LGPL license to MIT license
----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
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
#ifndef AS_WRITE
#define AS_WRITE
#endif
#ifndef AS_PARSE
#define AS_PARSE
#endif
#include "attset.h"
#ifndef TAB_READ
#define TAB_READ
#endif
#include "table.h"
#ifndef MLP_PARSE
#define MLP_PARSE
#endif
#ifndef MLP_EXTFN
#define MLP_EXTFN
#endif
#include "mlp.h"
#include "error.h"
#ifdef STORAGE
#include "storage.h"
#endif

#ifdef _MSC_VER
#ifndef snprintf
#define snprintf   _snprintf
#endif
#endif                          /* MSC still does not support C99 */

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define PRGNAME     "mlpx"
#define DESCRIPTION "multilayer perceptron execution"
#define VERSION     "version 2.2 (2014.10.24)         " \
                    "(c) 2001-2014   Christian Borgelt"

/* --- error codes --- */
/* error codes 0 to -5 defined in attset.h */
#define E_OPTION     (-6)       /* unknown option */
#define E_OPTARG     (-7)       /* missing option argument */
#define E_ARGCNT     (-8)       /* wrong number of arguments */
#define E_PARSE      (-9)       /* parse errors on input file */
#define E_PATSIZE   (-10)       /* invalid pattern size */
#define E_OUTPUT    (-11)       /* target in input or write output */

#define INPUT       "input"
#define HIDDEN      "hidden"
#define OUTPUT      "output"

#define SEC_SINCE(t)  ((double)(clock()-(t)) /(double)CLOCKS_PER_SEC)

/*----------------------------------------------------------------------
  Type Definitions
----------------------------------------------------------------------*/
typedef struct {                /* --- prediction result --- */
  ATT   *att;                   /* target attribute */
  int    type;                  /* type of the target attribute */
  VALID  cnt;                   /* number of classes (nominal target) */
  INST   pred;                  /* predicted value */
  CCHAR *col_pred;              /* name   of prediction column */
  int    cwd_pred;              /* width  of prediction column */
  int    dig_pred;              /* digits of prediction column */
  double conf;                  /* confidence of prediction */
  CCHAR *col_conf;              /* name   of confidence column */
  int    cwd_conf;              /* width  of confidence column */
  int    dig_conf;              /* digits of confidence column */
  int    all;                   /* whether to show all activations */
  int    cwd_all;               /* width  of activation columns */
  double err;                   /* error value (squared difference) */
} RESULT;                       /* (prediction result) */

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
  /* E_PATSIZE -10 */  "invalid pattern size %"DIMID_FMT,
  /* E_OUTPUT  -11 */  "must have target as input or write output",
  /*           -12 */  "unknown error",
};

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
static CCHAR    *prgname;       /* program name for error messages */
static SCANNER  *scan   = NULL; /* scanner (multilayer perceptron) */
static TABREAD  *tread  = NULL; /* table reader */
static TABWRITE *twrite = NULL; /* table writer */
static ATTSET   *attset = NULL; /* attribute set */
static ATTMAP   *attmap = NULL; /* attribute map */
static TABLE    *table  = NULL; /* data table */
static MLP      *mlp    = NULL; /* multilayer perceptron */
static RESULT   res     = {     /* prediction result */
  NULL, AT_NOM, 0,              /* target attribute data */
  {0}, "mlp", 0, 3,             /* data for prediction column */
  0.0, NULL,  0, 3,             /* data for confidence column */
  0, 0,                         /* flag for all activations */
  0 };                          /* error value (squared difference) */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/

#ifndef NDEBUG                  /* if debug version */
  #undef  CLEANUP               /* clean up memory and close files */
  #define CLEANUP \
  if (mlp)    mlp_deletex(mlp,   0); \
  if (attmap) am_delete(attmap,  0); \
  if (attset) as_delete(attset);     \
  if (table)  tab_delete(table,  0); \
  if (tread)  trd_delete(tread,  1); \
  if (twrite) twr_delete(twrite, 1); \
  if (scan)   scn_delete(scan,   1);
#endif

GENERROR(error, exit)           /* generic error reporting function */

/*--------------------------------------------------------------------*/

static void predict (void)
{                               /* --- classify the current tuple */
  INST *inst;                   /* to access the target instance */

  assert(mlp);                  /* check for a multilayer perceptron */
  mlp_exec  (mlp, NULL, NULL);  /* execute multilayer perceptron */
  mlp_result(mlp, &res.pred, &res.conf);
  inst = att_inst(res.att);     /* execute the multilayer perceptron */
  if      (res.type == AT_NOM)  /* if the target att. is nominal */
    res.err = (!isnone(inst->n) && (res.pred.n != inst->n)) ? 1 : 0;
  else if (res.type == AT_INT){ /* if the target att. is integer */
    res.err    = !isnull(inst->i) ? (double)(res.pred.i -inst->i) : 0;
    res.err   *= res.err; }     /* squared difference to true value */
  else {                        /* if the target att. is real-valued */
    res.err    = !isnan (inst->f) ? (double)(res.pred.f -inst->f) : 0;
    res.err   *= res.err;       /* squared difference to true value */
  }
}  /* predict() */

/*--------------------------------------------------------------------*/

static void infout (ATTSET *set, TABWRITE *twrite, int mode)
{                               /* --- write additional information */
  DIMID c, o;                   /* loop variable for classes */
  int   n, k;                   /* character counters */

  assert(set && twrite);        /* check the function arguments */
  if (mode & AS_ATT) {          /* if to write the header */
    twr_puts(twrite, res.col_pred); /* write prediction column name */
    if ((mode & AS_ALIGN)       /* if to align the column */
    && ((mode & AS_WEIGHT) || res.col_conf || res.all)){
      n = (int)strlen(res.col_pred);
      k = att_valwd(res.att, 0);
      res.cwd_pred = k = ((mode & AS_ALNHDR) && (n > k)) ? n : k;
      if (k > n) twr_pad(twrite, (size_t)(k-n));
    }                           /* compute width of class column */
    if (res.col_conf) {         /* if to write a class confability */
      twr_fldsep(twrite);       /* write a field separator and */
      twr_puts(twrite, res.col_conf);       /* the column name */
      if ((mode & AS_ALIGN)     /* if to align the column */
      && ((mode & AS_WEIGHT) || res.all)) {
        n = (int)strlen(res.col_conf);
        k = res.dig_conf +3;    /* compute width of confidence column */
        res.cwd_conf = k = ((mode & AS_ALNHDR) && (n > k)) ? n : k;
        if (k > n) twr_pad(twrite, (size_t)(k-n));
      }                         /* pad with blanks if requested */
    }
    if (res.type != AT_NOM) res.all = 0;
    if (res.all) {              /* if to write all activations */
      o = mlp_outcnt(mlp);      /* get the number of outputs */
      if (mode & AS_ALIGN) {    /* if to align the column */
        n = (int)(log((double)o+1) +0.99);
        k = res.dig_conf +3;    /* compute width of activ. columns */
        res.cwd_all = ((mode & AS_ALNHDR) && (n > k)) ? n : k;
      }
      for (c = 0; c < o; c++) { /* traverse the network outputs */
        twr_fldsep(twrite);     /* write separator and output index */
        n = twr_printf(twrite, "%"DIMID_FMT, c+1);
        if (res.cwd_all > n) twr_pad(twrite, (size_t)(res.cwd_all-n));
      }                         /* print the value name and */
    } }                         /* pad with blanks if requested */
  else {                        /* if to write a normal record */
    n = (res.type == AT_NOM)    /* get the class value */
      ? twr_printf(twrite, "%s", att_valname(res.att, res.pred.n))
      : (res.type == AT_INT)    /* format numeric types */
      ? twr_printf(twrite, "%"DTINT_FMT,         res.pred.i)
      : twr_printf(twrite, "%.*g", res.dig_pred, res.pred.f);
    if (res.cwd_pred > n) twr_pad(twrite, (size_t)(res.cwd_pred-n));
    if (res.col_conf) {         /* if to write a confidence field */
      twr_fldsep(twrite);       /* write separator and confidence */
      n = twr_printf(twrite, "%.*g", res.dig_conf, res.conf);
      if (res.cwd_conf > n) twr_pad(twrite, (size_t)(res.cwd_conf-n));
    }                           /* if to align, pad with blanks */
    if (res.all) {              /* if to write all activations */
      for (c = 0; c < res.cnt; c++) {
        twr_fldsep(twrite);     /* traverse the target values */
        n = twr_printf(twrite, "%.*g", res.dig_conf, mlp_output(mlp,c));
        if (res.cwd_all > n) twr_pad(twrite, (size_t)(res.cwd_all-n));
      }                         /* print the activation and */
    }                           /* pad with blanks if requested */
  }
}  /* infout() */

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k = 0;             /* loop variables, counters */
  char    *s;                   /* to traverse options */
  CCHAR   **optarg = NULL;      /* option argument */
  CCHAR   *fn_hdr  = NULL;      /* name of table header   file */
  CCHAR   *fn_mlp  = NULL;      /* name of neural network file */
  CCHAR   *fn_tab  = NULL;      /* name of input  pattern file */
  CCHAR   *fn_out  = NULL;      /* name of output pattern file */
  CCHAR   *recseps = NULL;      /* record  separators */
  CCHAR   *fldseps = NULL;      /* field   separators */
  CCHAR   *blanks  = NULL;      /* blank   characters */
  CCHAR   *comment = NULL;      /* comment characters */
  int     matinp   =  0;        /* flag for numerical matrix input */
  DIMID   dim      = -1;        /* data point/pattern dimension */
  int     mode     = AS_ATT|AS_MARKED; /* table file read  mode */
  int     mout     = AS_ATT;           /* table file write mode */
  double  sse      = 0.0;       /* (weighted) sum of squared errors */
  double  *pat;                 /* to traverse the patterns */
  ATTID   m, c;                 /* number of attributes */
  TPLID   n, r;                 /* number of data tuples */
  DIMID   x, o;                 /* number of dimensions/fields */
  DIMID   p;                    /* number of patterns */
  double  w, u;                 /* weight of data tuples, buffer */
  clock_t t;                    /* for time measurements */

  prgname = argv[0];            /* get program name for error msgs. */

  /* --- print startup/usage message --- */
  if (argc > 1) {               /* if arguments are given */
    fprintf(stderr, "%s - %s\n", argv[0], DESCRIPTION);
    fprintf(stderr, VERSION); } /* print a startup message */
  else {                        /* if no argument is given */
    printf("usage: %s [options] mlpfile [-d|-h hdrfile] "
                    "tabfile [outfile]\n", argv[0]);
    printf("%s\n", DESCRIPTION);
    printf("%s\n", VERSION);
    printf("-p#      prediction field name                  "
                    "(default: \"%s\")\n", res.col_pred);
    printf("-o#      significant digits for prediction      "
                    "(default: %d)\n", res.dig_pred);
    printf("-c#      confidence/activation field name       "
                    "(default: no confidence field)\n");
    printf("-z#      significant digits for confidence      "
                    "(default: %d)\n", res.dig_conf);
    printf("-x       print extended confidence information\n");
    printf("-a       align fields in output table           "
                    "(default: single separator)\n");
    printf("-w       do not write field names to the output file\n");
    printf("-r#      record  separators                     "
                    "(default: \"\\n\")\n");
    printf("-f#      field   separators                     "
                    "(default: \" \\t,\")\n");
    printf("-b#      blank   characters                     "
                    "(default: \" \\t\\r\")\n");
    printf("-C#      comment characters                     "
                    "(default: \"#\")\n");
    printf("mlpfile  file to read multilayer perceptron from\n");
    printf("-d       use default header "
                    "(attribute names = field numbers)\n");
    printf("-h       read table header  "
                    "(attribute names) from hdrfile\n");
    printf("hdrfile  file containing table header "
                    "(attribute names)\n");
    printf("tabfile  table file to read "
                    "(attribute names in first record)\n");
    printf("outfile  file to write output table to (optional)\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */

  /* --- evaluate arguments --- */
  for (i = 1; i < argc; i++) {  /* traverse arguments */
    s = argv[i];                /* get option argument */
    if (optarg) { *optarg = s; optarg = NULL; continue; }
    if ((*s == '-') && *++s) {  /* -- if argument is an option */
      while (1) {               /* traverse characters */
        switch (*s++) {         /* evaluate option */
          case 'p': optarg  = &res.col_pred; break;
          case 'c': optarg  = &res.col_conf; break;
          case 'o': res.dig_pred = (int)strtol(s, &s, 0); break;
          case 'z': res.dig_conf = (int)strtol(s, &s, 0); break;
          case 'a': mout   |=  AS_ALIGN;     break;
          case 'w': mout   &= ~AS_ATT;       break;
          case 'x': res.all = -1;            break;
          case 'r': optarg  = &recseps;      break;
          case 'f': optarg  = &fldseps;      break;
          case 'b': optarg  = &blanks;       break;
          case 'C': optarg  = &comment;      break;
          case 'd': mode   |= AS_DFLT;       break;
          case 'h': optarg  = &fn_hdr;       break;
          default : error(E_OPTION, *--s);   break;
        }                       /* set option variables */
        if (!*s) break;         /* if at end of string, abort loop */
        if (optarg) { *optarg = s; optarg = NULL; break; }
      } }                       /* get option argument */
    else {                      /* -- if argument is no option */
      switch (k++) {            /* evaluate non-option */
        case  0: fn_mlp = s;      break;
        case  1: fn_tab = s;      break;
        case  2: fn_out = s;      break;
        default: error(E_ARGCNT); break;
      }                         /* note filenames */
    }
  }
  if (optarg) error(E_OPTARG);  /* check option argument and */
  if ((k != 2) && (k != 3))     /* the number of arguments */
    error(E_ARGCNT);            /* (output file is optional) */
  if (fn_hdr && (strcmp(fn_hdr, "-") == 0))
    fn_hdr = "";                /* convert "-" to "" */
  i = ( fn_hdr && !*fn_hdr) ? 1 : 0;
  if  (!fn_mlp || !*fn_mlp) i++;
  if  (!fn_tab || !*fn_tab) i++;
  if (i > 1) error(E_STDIN);    /* stdin must not be used twice */
  if ((mout & AS_ATT) && (mout & AS_ALIGN))
    mout |= AS_ALNHDR;          /* set align to header flag */
  if (fn_out) mout |= AS_MARKED|AS_INFO1|AS_RDORD;
  else        mout  = 0;        /* set up the table write mode */
  fputc('\n', stderr);          /* terminate the startup message */

  /* --- read multilayer perceptron --- */
  scan = scn_create();          /* create a scanner */
  if (!scan) error(E_NOMEM);    /* for the multilayer perceptron */
  t = clock();                  /* start timer, open input file */
  if (scn_open(scan, NULL, fn_mlp) != 0)
    error(E_FOPEN, scn_name(scan));
  fprintf(stderr, "reading %s ... ", scn_name(scan));
  matinp = (scn_first(scan) == T_ID)
        && (strcmp(scn_value(scan), "dom") != 0);
  if (matinp)                   /* if matrix version */
    mlp = mlp_parse(scan);      /* parse the input network */
  else {                        /* if table version */
    attset = as_create("domains", att_delete);
    if (!attset) error(E_NOMEM);      /* create an attribute set */
    if (as_parse(attset, scan, AT_ALL, 1) != 0)
      error(E_PARSE, scn_name(scan)); /* parse domain descriptions */
    attmap = am_create(attset, 0, 1.0);
    if (!attmap) error(E_NOMEM);/* create an attribute map */
    mlp = mlp_parsex(scan, attmap);
  }                             /* parse the multilayer perceptron */
  if (!mlp || !scn_eof(scan, 1)) error(E_PARSE, scn_name(scan));
  scn_delete(scan, 1);          /* delete the scanner and */
  scan = NULL;                  /* print a log message */
  fprintf(stderr, "[%"DIMID_FMT" unit(s),",   mlp_unitcnt(mlp));
  fprintf(stderr, " %"DIMID_FMT" weight(s)]", mlp_wgtcnt(mlp));
  fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
  mlp_setup(mlp);               /* set network up for execution */

  if (matinp) {                 /* if matrix version */
    /* --- process patterns --- */
    tread = trd_create();       /* create a table reader and */
    if (!tread) error(E_NOMEM); /* set the separator characters */
    trd_allchs(tread, recseps, fldseps, blanks, "", comment);
    t = clock();                /* start timer, open input file */
    if (trd_open(tread, NULL, fn_tab) != 0)
      error(E_FOPEN, trd_name(tread));
    fprintf(stderr, "reading %s ... ", trd_name(tread));
    if (fn_out) {               /* if to write an output file */
      twrite = twr_create();    /* create a table writer and */
      if (!twrite) error(E_NOMEM);   /* configure characters */
      twr_xchars(twrite, recseps, fldseps, blanks, "");
      if (twr_open(twrite, NULL, fn_out) != 0)
        error(E_FOPEN, twr_name(twrite));
    }                           /* open the output file */
    k = vec_readx(&pat, &dim, tread); /* read the first pattern */
    if (k) error(k, TRD_INFO(tread)); /* from the input file */
    x = mlp_incnt(mlp);         /* get the number of inputs */
    o = mlp_outcnt(mlp);        /* and outputs of the network */
    if ((dim != x) && (dim != x+o)){/* check the pattern size */
      free(pat); error(E_PATSIZE, dim); }
    for (p = 0; k == 0; p++) {  /* pattern read loop */
      mlp_exec(mlp, pat, NULL); /* execute multilayer perceptron */
      if (dim > x)              /* and sum the squared errors */
        sse += mlp_error(mlp, pat +x);
      if (twrite) {             /* if to write an output table */
        for (c = 0; c < dim; c++) {
          twr_printf(twrite, "%.*g", res.dig_pred, pat[c]);
          twr_fldsep(twrite);   /* print the pattern elements */
        }                       /* followed by a field separator */
        for (c = 0; c < o;   c++) {
          if (c > 0) twr_fldsep(twrite);
          twr_printf(twrite, "%.*g", res.dig_pred, mlp_output(mlp, c));
        }                       /* print the values computed */
        twr_recsep(twrite);     /* by the multilayer perceptron */
      }                         /* and terminate the record */
      k = vec_read(pat, dim, tread);
    }                           /* read the next pattern */
    free(pat);                  /* delete the pattern buffer */
    if (k < 0) error(k, TRD_INFO(tread));
    trd_delete(tread, 1);       /* close the input file and */
    tread = NULL;               /* delete the table reader */
    if (twrite) {               /* if an output file was written */
      if (twr_close(twrite) != 0) error(E_FWRITE, twr_name(twrite));
      twr_delete(twrite, 1);    /* close the output file and */
      twrite = NULL;            /* delete the table writer */
    }                           /* print a success message */
    fprintf(stderr, "[%"DIMID_FMT" pattern(s)]", p);
    fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));

    /* --- print error statistics --- */
    if (dim > x) {              /* if desired outputs are present */
      fprintf(stderr, "sse : %g", sse);
      if (p > 0) {              /* if there was at least one pattern */
        sse /= (double)p;       /* compute mean squared error */
        fprintf(stderr, ", mse: %g, rmse: %g\n", sse, sqrt(sse));
      }                         /* print some error measures */
    } }                         /* for the test pattern set */

  else {                        /* if table version */
    /* --- get target attribute --- */
    res.att  = as_att(attset, mlp_trgid(mlp));
    res.type = att_type(res.att);    /* get the target attribute and */
    res.cnt  = att_valcnt(res.att);  /* its type and num. of values */
    if (res.type != AT_NOM)     /* no confidence column */
      res.col_conf = NULL;      /* for numeric targets */
    as_setmark(attset, 1);      /* mark all attributes */
    att_setmark(res.att, 0);    /* except the class attribute */

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
      fprintf(stderr, "[%"ATTID_FMT" attribute(s)]", as_attcnt(attset));
      fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));
      mode &= ~(AS_ATT|AS_DFLT);/* print a success message and */
    }                           /* remove the attribute flag */

    /* --- process table body --- */
    t = clock();                /* start timer, open input file */
    if (trd_open(tread, NULL, fn_tab) != 0)
      error(E_FOPEN, trd_name(tread));
    fprintf(stderr, "reading %s ... ", trd_name(tread));
    if (mout & AS_ALIGN) {      /* if to align the output columns */
      table = tab_create("table", attset, tpl_delete);
      if (!table) error(E_NOMEM);  /* read the data table */
      k = tab_read(table, tread, mode);
      if (k < 0) error(-k, tab_errmsg(table, NULL, 0));
      trd_delete(tread, 1);     /* delete the table reader */
      tread = NULL;             /* and clear the variable */
      m = tab_attcnt(table);    /* get the number of attributes */
      n = tab_tplcnt(table);    /* and the number of tuples */
      w = tab_tplwgt(table);    /* and print a success message */
      fprintf(stderr, "[%"ATTID_FMT" attribute(s),", m);
      fprintf(stderr, " %"TPLID_FMT, n);
      if (w != (double)n) fprintf(stderr, "/%g", w);
      fprintf(stderr, " tuple(s)] done [%.2fs].\n", SEC_SINCE(t));
      twrite = twr_create();       /* create a table writer and */
      if (!twrite) error(E_NOMEM); /* configure the characters */
      twr_xchars(twrite, recseps, fldseps, blanks, "");
      t = clock();              /* start timer, open output file */
      if (twr_open(twrite, NULL, fn_out) != 0)
        error(E_FOPEN, twr_name(twrite));
      fprintf(stderr, "writing %s ... ", twr_name(twrite));
      if ((mout & AS_ATT)       /* write a table header */
      &&  (as_write(attset, twrite, mout, infout) != 0))
        error(E_FWRITE, twr_name(twrite));
      mout = AS_INST | (mout & ~AS_ATT);
      m += (res.col_conf ? 2 : 1) +(res.all ? res.cnt : 0);
      for (r = 0; r < n; r++) { /* traverse the tuples */
        tpl_toas(tab_tpl(table, r));
        mlp_inputx(mlp, NULL);  /* set the pattern from a tuple */
        predict();              /* compute prediction for target */
        u = as_getwgt(attset);  /* get the tuple weight and */
        sse += res.err *u;      /* sum the prediction errors */
        if (as_write(attset, twrite, mout, infout) != 0)
          error(E_FWRITE, twr_name(twrite));
      } }                       /* write the current tuple */
    else {                      /* if to process tuples directly */
      k = as_read(attset, tread, mode);     /* read table header */
      if (k < 0) error(-k, as_errmsg(attset, NULL, 0));
      if (!fn_out && (att_getmark(res.att) < 0))
        error(E_OUTPUT);        /* check for outp4ut to produce */
      if (fn_out) {             /* if to write an output file */
        twrite = twr_create();       /* create a table writer and */
        if (!twrite) error(E_NOMEM); /* configure the characters */
        twr_xchars(twrite, recseps, fldseps, blanks, "");
        t = clock();            /* start timer, open output file */
        if (twr_open(twrite, NULL, fn_out) != 0)
          error(E_FOPEN, twr_name(twrite));
        if ((mout & AS_ATT)     /* write a table header */
        &&  (as_write(attset, twrite, mout, infout) != 0))
          error(E_FWRITE, twr_name(twrite));
        mout = AS_INST | (mout & ~AS_ATT);
      }                         /* remove the attribute flag */
      i = mode; mode = (mode & ~(AS_DFLT|AS_ATT)) | AS_INST;
      if (i & AS_ATT)           /* if not done yet, read first tuple */
        k = as_read(attset, tread, mode);
      for (w = 0, n = 0; k == 0; n++) {
        mlp_inputx(mlp, NULL);  /* set the pattern from a tuple */
        predict();              /* predict target for current tuple */
        w   += u = as_getwgt(attset); /* sum the tuple weights and */
        sse += res.err *u;      /* count the classification errors */
        if (twrite              /* write the current tuple */
        && (as_write(attset, twrite, mout, infout) != 0))
          error(E_FWRITE, twr_name(twrite));
        k = as_read(attset, tread, mode);
      }                         /* try to read the next tuple */
      if (k < 0) error(-k, as_errmsg(attset, NULL, 0));
      trd_delete(tread, 1);     /* delete the table reader */
      tread = NULL;             /* and clear the variable */
      m = as_attcnt(attset);    /* get the number of attributes */
    }
    if (twrite) {               /* if an output file was written */
      if (twr_close(twrite) != 0) error(E_FWRITE, twr_name(twrite));
      twr_delete(twrite, 1);    /* close the output file and */
      twrite = NULL;            /* delete the table writer */
    }                           /* print a success message */
    fprintf(stderr, "[%"ATTID_FMT" attribute(s),", m);
    fprintf(stderr, " %"TPLID_FMT, n);
    if (w != (double)n) fprintf(stderr, "/%g", w);
    fprintf(stderr, " tuple(s)] done [%.2fs].\n", SEC_SINCE(t));

    /* --- print error statistics --- */
    if (att_getmark(res.att) >= 0) {
      if (res.type != AT_NOM) { /* if the target att. is metric */
        fprintf(stderr, "sse: %g", sse);
        if (w > 0) {            /* if there was at least one tuple, */
          sse /= w;             /* compute mean squared error */
          fprintf(stderr, ", mse: %g, rmse: %g", sse, sqrt(sse));
        } }                     /* print some error measures */
      else {                    /* if the target att. is nominal */
        fprintf(stderr, "%g error(s) ", sse);
        fprintf(stderr, "(%.2f%%)", (w > 0) ? 100.0*(sse/w) : 0);
      }                         /* print an error indicator */
      fputc('\n', stderr);      /* terminate the error statistics */
    }
  }                             /* if (matinp) .. else .. */

  /* --- clean up --- */
  CLEANUP;                      /* clean up memory and close files */
  SHOWMEM;                      /* show (final) memory usage */
  return 0;                     /* return 'ok' */
}  /* main() */
