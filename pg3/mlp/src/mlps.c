/*----------------------------------------------------------------------
  File    : mlps.c
  Contents: multilayer perceptron sensitivity analysis
  Author  : Christian Borgelt
  History : 2002.06.18 file created from file mlpx.c
            2003.08.16 slight changes in error message output
            2004.02.26 source files mmlps.c and mlps.c combined
            2007.01.10 use of attribute maps changed
            2007.02.14 adapted to modufied module tabread
            2007.03.16 table and matrix version combined
            2011.12.15 processing without table reading improved
            2013.08.12 adapted to definitions ATTID, VALID, TPLID etc.
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

/*----------------------------------------------------------------------
  Preprocessor Definitions
----------------------------------------------------------------------*/
#define PRGNAME     "mlps"
#define DESCRIPTION "multilayer perceptron sensitivity analysis"
#define VERSION     "version 2.2 (2014.10.24)         " \
                    "(c) 2002-2014   Christian Borgelt"

/* --- error codes --- */
/* error codes 0 to -5 defined in attset.h */
#define E_OPTION     (-6)       /* unknown option */
#define E_OPTARG     (-7)       /* missing option argument */
#define E_ARGCNT     (-8)       /* wrong number of arguments */
#define E_PARSE      (-9)       /* parse errors on input file */
#define E_PATCNT    (-10)       /* no pattern found */
#define E_PATSIZE   (-11)       /* invalid pattern size */

#define INPUT       "input"
#define HIDDEN      "hidden"
#define OUTPUT      "output"

#define SEC_SINCE(t)  ((double)(clock()-(t)) /(double)CLOCKS_PER_SEC)

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
  /* E_PATCNT  -10 */  "no pattern in file %s",
  /* E_PATSIZE -11 */  "invalid pattern size %"DIMID_FMT,
  /*           -12 */  "unknown error",
};

/*----------------------------------------------------------------------
  Global Variables
----------------------------------------------------------------------*/
static CCHAR   *prgname;        /* program name for error messages */
static SCANNER *scan   = NULL;  /* scanner (multilayer perceptron) */
static TABREAD *tread  = NULL;  /* table reader */
static ATTSET  *attset = NULL;  /* attribute set */
static ATTMAP  *attmap = NULL;  /* attribute map */
static MLP     *mlp    = NULL;  /* multilayer perceptron */
static FILE    *out    = NULL;  /* output file */
static double  *sens   = NULL;  /* sensitivity values */

/*----------------------------------------------------------------------
  Main Functions
----------------------------------------------------------------------*/

#ifndef NDEBUG                  /* if debug version */
  #undef  CLEANUP               /* clean up memory and close files */
  #define CLEANUP \
  if (sens)   free(sens);           \
  if (mlp)    mlp_deletex(mlp,  0); \
  if (attmap) am_delete(attmap, 0); \
  if (attset) as_delete(attset);    \
  if (tread)  trd_delete(tread, 1); \
  if (scan)   scn_delete(scan,  1); \
  if (out && (out != stdout)) fclose(out);
#endif

GENERROR(error, exit)           /* generic error reporting function */

/*--------------------------------------------------------------------*/

int main (int argc, char *argv[])
{                               /* --- main function */
  int     i, k = 0;             /* loop variables, buffers */
  char    *s;                   /* to traverse options */
  CCHAR   **optarg = NULL;      /* option argument */
  CCHAR   *fn_hdr  = NULL;      /* name of table header file */
  CCHAR   *fn_mlp  = NULL;      /* name of network file */
  CCHAR   *fn_tab  = NULL;      /* name of input pattern file */
  CCHAR   *fn_out  = NULL;      /* name of output file */
  CCHAR   *blanks  = NULL;      /* blank   characters */
  CCHAR   *fldseps = NULL;      /* field   separators */
  CCHAR   *recseps = NULL;      /* record  separators */
  CCHAR   *comment = NULL;      /* comment characters */
  int     matinp   =  0;        /* flag for numerical matrix input */
  int     mode     = AS_ATT|AS_NOXATT;    /* table file read flags */
  DIMID   dim      = -1;        /* data point/pattern dimension */
  int     magg     = MLP_MAX;   /* mode for sensitivity aggregation */
  int     norm     = 1;         /* normalize sensitivity */
  int     digs     = 6;         /* significant digits for sensitivity */
  double  *pat;                 /* to traverse the patterns */
  TPLID   n;                    /* number of data tuples */
  DIMID   x, o, c;              /* number of dimensions/fields */
  DIMID   p = 0;                /* number of patterns */
  double  w;                    /* weight of data tuples, buffer */
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
    printf("-s       sum sensitivity over output units      "
                    "(default: take maximum)\n");
    printf("-i       sum sensitivity over input  units      "
                    "(default: take maximum)\n");
    printf("-n       do not normalize sensitivity "
                    "(do not divide by number of patterns)\n");
    printf("-o#      significant digits for sensitivity     "
                    "(default: %d)\n", digs);
    printf("-r#      record  separators                     "
                    "(default: \"\\n\")\n");
    printf("-f#      field   separators                     "
                    "(default: \" \\t,\")\n");
    printf("-b#      blank   characters                     "
                    "(default: \" \\t\\r\")\n");
    printf("-C#      comment characters                     "
                    "(default: \"#\")\n");
    printf("mlpfile  file to read neural network from\n");
    printf("-d       use default header "
                    "(attribute names = field numbers)\n");
    printf("-h       read table header  "
                    "(attribute names) from hdrfile\n");
    printf("hdrfile  file containing table header "
                    "(attribute names)\n");
    printf("tabfile  table file to read "
                    "(attribute names in first record)\n");
    printf("outfile  output file for sensitivity values [optional]\n");
    return 0;                   /* print a usage message */
  }                             /* and abort the program */

  /* --- evaluate arguments --- */
  for (i = 1; i < argc; i++) {  /* traverse arguments */
    s = argv[i];                /* get option argument */
    if (optarg) { *optarg = s; optarg = NULL; continue; }
    if ((*s == '-') && *++s) {  /* -- if argument is an option */
      while (1) {               /* traverse characters */
        switch (*s++) {         /* evaluate option */
          case 's': magg  |= MLP_SUM;      break;
          case 'i': magg  |= MLP_SUMIN;    break;
          case 'n': norm   = 0;            break;
          case 'r': optarg = &recseps;     break;
          case 'f': optarg = &fldseps;     break;
          case 'b': optarg = &blanks;      break;
          case 'C': optarg = &comment;     break;
          case 'd': mode  |= AS_DFLT;      break;
          case 'h': optarg = &fn_hdr;      break;
          default : error(E_OPTION, *--s); break;
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
  if (optarg) error(E_OPTARG);  /* check option argument */
  if ((k != 2) && (k != 3))     /* check the number */
    error(E_ARGCNT);            /*  of arguments */
  if (fn_hdr && (strcmp(fn_hdr, "-") == 0))
    fn_hdr = "";                /* convert "-" to "" */
  i = ( fn_hdr && !*fn_hdr) ? 1 : 0;
  if  (!fn_mlp || !*fn_mlp) i++;
  if  (!fn_tab || !*fn_tab) i++;
  if (i > 1) error(E_STDIN);    /* stdin must not be used twice */
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
    if ((as_parse(attset, scan, AT_ALL, 1) != 0))
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
    w = 0;                      /* read the first pattern */
    k = vec_readx(&pat, &dim, tread);
    if (k) error(k, TRD_INFO(tread));
    x = mlp_incnt(mlp);         /* get the number of inputs */
    o = mlp_outcnt(mlp);        /* and outputs of the network */
    if ((dim != x) && (dim != x+o))
      error(E_PATSIZE, dim);    /* check the pattern size */
    sens = (double*)calloc((size_t)x, sizeof(double));
    if (!sens) error(E_NOMEM);  /* create a vector of sensitivities */
    for (p = 0; k == 0; p++) {  /* pattern read loop */
      mlp_exec(mlp, pat, NULL); /* execute the multilayer perceptron */
      for (c = 0; c < x; c++)   /* and compute sensitivity values */
        sens[c] += mlp_sens(mlp, c, magg);
      k = vec_read(pat, dim, tread);
    }                           /* read the next pattern */
    if (k < 0) error(k, TRD_INFO(tread));
    trd_delete(tread, 1);       /* close the input file and */
    tread = NULL;               /* delete the table reader */
    fprintf(stderr, "[%"DIMID_FMT" pattern(s)]", p);
    fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t)); }

  else {                        /* if table version */
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
    x = as_attcnt(attset);      /* get the number of attributes */
    o = mlp_trgid(mlp);         /* and the target attribute id */
    sens = (double*)calloc((size_t)x, sizeof(double));
    if (!sens) error(E_NOMEM);  /* create a vector of sensitivities */
    k = as_read(attset, tread, mode); /* read/generate table header */
    if (k < 0) error(-k, as_errmsg(attset, NULL, 0));
    i = mode; mode = (mode & ~(AS_DFLT|AS_ATT)) | AS_INST;
    if (i & AS_ATT)             /* if not done yet, read first tuple */
      k = as_read(attset, tread, mode);
    for (w = 0, n = 0; k == 0; n++) {
      mlp_inputx(mlp, NULL);    /* set the pattern from a tuple, */
      mlp_exec(mlp,NULL,NULL);  /* execute multilayer perceptron, */
      w += as_getwgt(attset);   /* sum the tuple weights and */
      for (c = 0; c < x; c++)   /* compute sensitivity values */
        if (c != o) sens[c] += mlp_sensx(mlp, c, magg);
      k = as_read(attset, tread, mode);
    }                           /* try to read the next tuple */
    if (k < 0) error(-k, as_errmsg(attset, NULL, 0));
    trd_delete(tread, 1);       /* delete the table reader */
    tread = NULL;               /* and clear the variable */
    fprintf(stderr, "[%"ATTID_FMT" attribute(s),", as_attcnt(attset)+1);
    fprintf(stderr, " %"TPLID_FMT, n);
    if (w != (double)n) fprintf(stderr, "/%g", w);
    fprintf(stderr, " tuple(s)] done [%.2fs].\n", SEC_SINCE(t));
  }                             /* if (matinp) .. else .. */

  /* --- print results --- */
  t = clock();                  /* start timer, open output file */
  if (fn_out && (strcmp(fn_out, "-") == 0)) fn_out = "";
  if (fn_out && *fn_out) { out = fopen(fn_out, "w"); }
  else                   { out = stdout; fn_out = "<stdout>"; }
  if (!out) error(E_FOPEN, fn_out);
  fprintf(stderr, "writing %s ... ", fn_out);
  if (matinp) {                 /* if matrix version */
    w = (norm && (p > 0)) ? 1.0/(double)p : 1;
    fprintf(out, "input sensitivity\n");
    for (c = 0; c < x; c++) {   /* traverse the input units */
      fprintf(out, "%2"DIMID_FMT" ", c);
      fprintf(out, "%.*g\n", digs, sens[c] *w);
    } }                         /* print sensitivity value */
  else {                        /* if table version */
    for (k = 0, c = 0; c < x; c++) {
      if (c == o) continue;     /* traverse the input attributes */
      i = att_width(as_att(attset, c), 0);
      if (i > k) k = i;         /* find the length of the */
    }                           /* longest attribute name */
    fprintf(out, "%-*s sensitivity\n", k, "input");
    w = (norm && (w > 0)) ? 1.0/w : 1;
    for (c = 0; c < x; c++) {   /* traverse the input attributes */
      if (c == o) continue;     /* print the attribute name */
      fprintf(out, "%-*s ", k, att_name(as_att(attset, c)));
      fprintf(out, "%.*g\n", digs, sens[c] *w);
    }                           /* print sensitivity value */
  }
  if (((out == stdout) ? fflush(out) : fclose(out)) != 0)
    error(E_FWRITE, fn_out);    /* close the output file */
  if (matinp) fprintf(stderr, "[%"DIMID_FMT" input(s)]",     x);
  else        fprintf(stderr, "[%"DIMID_FMT" attribute(s)]", x-1);
  fprintf(stderr, " done [%.2fs].\n", SEC_SINCE(t));

  /* --- clean up --- */
  CLEANUP;                      /* clean up memory and close files */
  SHOWMEM;                      /* show (final) memory usage */
  return 0;                     /* return 'ok' */
}  /* main() */
