#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "defs.h"
#include "gripes.h"
#include "man.h"
#include "man-config.h"
#include "man-getopt.h"
#include "util.h"
#include "version.h"

int alt_system;
char *alt_system_name;
char *opt_manpath;
int global_apropos = 0;

static void
print_version (void) {
     gripe (VERSION, progname, version);
}

static void
usage (void) {
     print_version();
     gripe (USAGE1, progname);

     gripe (USAGE2);		/* only for alt_systems */

     gripe (USAGE3);
     gripe (USAGE4);
     gripe (USAGE5);    /* maybe only if troff found? */
     gripe (USAGE6);

     gripe (USAGE7);		/* only for alt_systems */

     gripe (USAGE8);
     exit(1);
}

static char short_opts[] = "C:M:P:S:acdDfFhkKm:p:tvVwW?";

#ifndef NOGETOPT
#define _GNU_SOURCE
#include <getopt.h>

static const struct option long_opts[] = {
    { "help",       no_argument,            NULL, 'h' },
    { "version",    no_argument,            NULL, 'v' },
    { "path",       no_argument,            NULL, 'w' },
    { "preformat",  no_argument,            NULL, 'F' },
    { NULL, 0, NULL, 0 }
};
#endif

/*
 * Get options from the command line and user environment.
 * Also reads the configuration file.
 */

void
man_getopt (int argc, char **argv) {
     int c;
     char *config_file = NULL;
     char *manp = NULL;

#ifndef NOGETOPT
     while ((c = getopt_long (argc, argv, short_opts, long_opts, NULL)) != -1){
#else
     while ((c = getopt (argc, argv, short_opts)) != -1) {
#endif
	  switch (c) {
	  case 'C':
	       no_privileges ();
	       config_file = my_strdup (optarg);
	       break;
	  case'F':
	       preformat = 1;
	       break;
	  case 'M':
	       manp = my_strdup (optarg);
	       break;
	  case 'P':
	       pager = my_strdup (optarg);
	       break;
	  case 'S':
	       colon_sep_section_list = my_strdup (optarg); 
	       break;
	  case 'a':
	       findall++;
	       break;
	  case 'c':
	       nocats++;
	       break;
	  case 'D':
	       debug++;
	  case 'd':
	       debug++;
	       break;
	  case 'f':
	       if (do_troff)
		    fatal (INCOMPAT, "-f", "-t");
	       if (apropos)
		    fatal (INCOMPAT, "-f", "-k");
	       if (print_where)
		    fatal (INCOMPAT, "-f", "-w");
	       whatis++;
	       break;
	  case 'k':
	       if (do_troff)
		    fatal (INCOMPAT, "-k", "-t");
	       if (whatis)
		    fatal (INCOMPAT, "-k", "-f");
	       if (print_where)
		    fatal (INCOMPAT, "-k", "-w");
	       apropos++;
	       break;
	  case 'K':
	       global_apropos++;
	       break;
	  case 'm':
	       alt_system++;
	       alt_system_name = my_strdup (optarg);
	       break;
	       /* or:  gripe (NO_ALTERNATE); exit(1); */
	  case 'p':
	       roff_directive = my_strdup (optarg);
	       break;
	  case 't':
	       if (apropos)
		    fatal (INCOMPAT, "-t", "-k");
	       if (whatis)
		    fatal (INCOMPAT, "-t", "-f");
	       if (print_where)
		    fatal (INCOMPAT, "-t", "-w");
	       do_troff++;
	       break;
	  case 'v':
	  case 'V':
	       print_version();
	       exit(0);
	  case 'W':
	       one_per_line++;
	       /* fall through */
	  case 'w':
	       if (apropos)
		    fatal (INCOMPAT, "-w", "-k");
	       if (whatis)
		    fatal (INCOMPAT, "-w", "-f");
	       if (do_troff)
		    fatal (INCOMPAT, "-w", "-t");
	       print_where++;
	       break;
	  case 'h':
	  case '?':
	  default:
	       usage();
	       break;
	  }
     }

     read_config_file (config_file);

     if (pager == NULL || *pager == '\0')
	  if ((pager = getenv ("MANPAGER")) == NULL)
	       if ((pager = getenv ("PAGER")) == NULL)
		    pager = getval ("PAGER");

     if (debug)
	  gripe (PAGER_IS, pager);

     if (do_compress && !*getval("COMPRESS")) {
	  if (debug)
	       gripe (NO_COMPRESS);
	  do_compress = 0;
     }

     if (do_troff && !*getval ("TROFF")) {
	  gripe (NO_TROFF, configuration_file);
	  exit (1);
     }

     opt_manpath = manp;		/* do not yet expand manpath -
					   maybe it is not needed */

     if (alt_system_name == NULL || *alt_system_name == '\0')
	  if ((alt_system_name = getenv ("SYSTEM")) != NULL)
	       alt_system_name = my_strdup (alt_system_name);

}
