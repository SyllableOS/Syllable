/* Entries for config.h.in that aren't automatically generated.  */

/* Define if you have the Andrew File System.  */
#undef AFS

/* Define If you want find -nouser and -nogroup to make tables of
   used UIDs and GIDs at startup instead of using getpwuid or
   getgrgid when needed.  Speeds up -nouser and -nogroup unless you
   are running NIS or Hesiod, which make password and group calls
   very expensive.  */
#undef CACHE_IDS

/* Define to use SVR4 statvfs to get filesystem type.  */
#undef FSTYPE_STATVFS

/* Define to use SVR3.2 statfs to get filesystem type.  */
#undef FSTYPE_USG_STATFS

/* Define to use AIX3 statfs to get filesystem type.  */
#undef FSTYPE_AIX_STATFS

/* Define to use 4.3BSD getmntent to get filesystem type.  */
#undef FSTYPE_MNTENT

/* Define to use 4.4BSD and OSF1 statfs to get filesystem type.  */
#undef FSTYPE_STATFS

/* Define to use Ultrix getmnt to get filesystem type.  */
#undef FSTYPE_GETMNT

/* Define to `unsigned long' if <sys/types.h> doesn't define.  */
#undef dev_t

/* Define to `unsigned long' if <sys/types.h> doesn't define.  */
#undef ino_t

