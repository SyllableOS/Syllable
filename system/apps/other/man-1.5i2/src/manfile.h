struct manpage {
     struct manpage *next;
     char *filename;
     int type;
};

#define TYPE_MAN	1
#define TYPE_CAT	2
#define TYPE_SCAT	4

#define ONLY_ONE_PERSEC	8	/* do not return more pages from one section */
#define ONLY_ONE	24	/* return only a single page */

/* various standards have various ideas about where the cat pages
   ought to live */
#define FSSTND		32
#define	FHS		64

/* HP has a peculiar way to indicate that pages are compressed */
#define DO_HP		128	/* compressed file in man1.Z/ls.1 */

/* IRIX has a peculiar cat page naming */
#define DO_IRIX		256	/* cat page ls.z, not ls.1.z */

extern struct manpage *
manfile(char *name, char *section, int flags,
        char **sectionlist, char **manpath,
	char *(*tocat)(char *, char *, int));
