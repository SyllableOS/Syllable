# Run paths

#export DLL_PATH=~/.nix-profile/lib:$DLL_PATH
export LD_LIBRARY_PATH=~/.nix-profile/lib:$LD_LIBRARY_PATH
export MANPATH=~/.nix-profile/man:~/.nix-profile/share/man:$MANPATH
export INFOPATH=~/.nix-profile/info:$INFOPATH

while read dir
do
	PYTHONPATH=$dir/site-packages:$PYTHONPATH
done < <(find ~/.nix-profile/lib -maxdepth 1 -name 'python*' | sort)
export PYTHONPATH

export PERL5LIB=~/.nix-profile/lib/site_perl/5.10.0:~/.nix-profile/lib/5.10.0:$PERL5LIB

# Paths for software compilation

export LIBRARY_PATH=~/.nix-profile/lib:$LIBRARY_PATH
export C_INCLUDE_PATH=~/.nix-profile/include:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=~/.nix-profile/include:$CPLUS_INCLUDE_PATH
export PKG_CONFIG_PATH=~/.nix-profile/lib/pkgconfig:~/.nix-profile/share/pkgconfig:$PKG_CONFIG_PATH
