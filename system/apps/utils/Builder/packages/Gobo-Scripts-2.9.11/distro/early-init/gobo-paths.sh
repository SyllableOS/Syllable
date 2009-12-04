# Run paths

export PATH=$PATH:/System/Links/Executables
#export DLL_PATH=$DLL_PATH:/System/Links/Libraries
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/System/Links/Libraries
export MANPATH=$MANPATH:/System/Links/Manuals:/System/Links/Shared/man
export INFOPATH=$INFOPATH:/System/Links/Manuals/info

while read dir
do
	PYTHONPATH=$PYTHONPATH:$dir/site-packages
done < <(find /System/Links/Libraries -maxdepth 1 -name 'python*' | sort --reverse)
export PYTHONPATH

export PERL5LIB=$PERL5LIB:/System/Links/Libraries/site_perl/5.10.0:/System/Links/Libraries/5.10.0

# Paths for software compilation

export LIBRARY_PATH=$LIBRARY_PATH:/System/Links/Libraries
export C_INCLUDE_PATH=$C_INCLUDE_PATH:/System/Links/Headers
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/System/Links/Headers
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/System/Links/Libraries/pkgconfig:/System/Links/Shared/pkgconfig
