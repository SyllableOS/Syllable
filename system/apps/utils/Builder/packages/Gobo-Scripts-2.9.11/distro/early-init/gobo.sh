# Run paths

export PATH=/System/Index/bin:/System/Links/Executables:$PATH
#export DLL_PATH=/System/Index/lib:/System/Links/Libraries:$DLL_PATH
# LD_LIBRARY_PATH is modified earlier in /etc/profile
export MANPATH=/System/Index/share/man:/System/Links/Manuals:/System/Links/Shared/man:$MANPATH
export INFOPATH=/System/Index/share/info:/System/Links/Manuals/info:$INFOPATH

for dir in `find /System/Links/Libraries -maxdepth 1 -name 'python*' | sort` \
	 `find /System/Index/lib -maxdepth 1 -name 'python*' | sort`
do
	PYTHONPATH=$dir/site-packages:$PYTHONPATH
done
# For GObject:
export PYTHONPATH=/System/Index/lib/python2.6/site-packages/gtk-2.0:$PYTHONPATH

export PERL5LIB=/System/Index/lib/site_perl/5.10.0:/System/Index/lib/5.10.0:/System/Links/Libraries/site_perl/5.10.0:/System/Links/Libraries/5.10.0:$PERL5LIB

# Paths for software compilation

export LIBRARY_PATH=/System/Index/lib:/System/Links/Libraries:$LIBRARY_PATH
export C_INCLUDE_PATH=/System/Index/include:/System/Links/Headers:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/System/Index/include:/System/Links/Headers:$CPLUS_INCLUDE_PATH
export PKG_CONFIG_PATH=/System/Index/lib/pkgconfig:/System/Index/share/pkgconfig:/System/Links/Libraries/pkgconfig:/System/Links/Shared/pkgconfig:$PKG_CONFIG_PATH
