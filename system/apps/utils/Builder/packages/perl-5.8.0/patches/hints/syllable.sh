# Syllable hints file ( http://syllable.sf.net/ )
# Kurt Skauen, kurt@atheos.cx for AtheOS
# Kaj de Vos, for Syllable

prefix="/usr/perl"

libpth='/system/libs /usr/lib'
usrinc='/include'

libs=' '
#libswanted=' '

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

