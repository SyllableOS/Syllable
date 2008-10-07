# Syllable hints file ( http://syllable.org )
# Kurt Skauen, for AtheOS
# Kaj de Vos, for Syllable

#d_suidsafe='undef'
ignore_versioned_solibs='y'
#libc=/system/indexes/lib/libc.so.2

libpth='/resources/indexes/lib /system/indexes/lib /system/libraries ./'
#usrinc='/system/indexes/include'

#libs=' '
libswanted='nsl ndbm db dl m c crypt util'

d_htonl='define'
d_htons='define'
d_ntohl='define'
d_ntohs='define'

d_locconv='undef'

# POSIX and BSD functions are scattered over several non-standard libraries
# in AtheOS, so I figured it would be safer to let the linker figure out
# which symbols are available.

usenm='false'

# Hopefully, the native malloc knows better than Perl's.
usemymalloc='n'

# Syllable native FS does not support hard-links, but link() is defined
# (for other FSes).

d_link='undef'
dont_use_nlink='define'

ld='gcc'
cc='gcc'

ldlibpthname=LIBRARY_PATH
