# Run paths

export PATH=/System/Links/Executables:$PATH
#export DLL_PATH=/System/Links/Libraries:$DLL_PATH
# LD_LIBRARY_PATH is modified earlier in /etc/profile
export MANPATH=/System/Links/Manuals:/System/Links/Shared/man:$MANPATH
export INFOPATH=/System/Links/Manuals/info:$INFOPATH

for dir in `find /System/Links/Libraries -maxdepth 1 -name 'python*' | sort`
do
	PYTHONPATH=$dir/site-packages:$PYTHONPATH
done
export PYTHONPATH

export PERL5LIB=/System/Links/Libraries/site_perl/5.10.0:/System/Links/Libraries/5.10.0:$PERL5LIB

# Paths for software compilation

export LIBRARY_PATH=/System/Links/Libraries:$LIBRARY_PATH
export C_INCLUDE_PATH=/System/Links/Headers:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=/System/Links/Headers:$CPLUS_INCLUDE_PATH
export PKG_CONFIG_PATH=/System/Links/Libraries/pkgconfig:/System/Links/Shared/pkgconfig:$PKG_CONFIG_PATH
