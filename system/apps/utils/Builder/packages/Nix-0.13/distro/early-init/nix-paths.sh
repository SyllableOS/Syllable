# Run paths

#export DLL_PATH=$DLL_PATH:~/.nix-profile/lib
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:~/.nix-profile/lib
export MANPATH=$MANPATH:~/.nix-profile/man:~/.nix-profile/share/man
export INFOPATH=$INFOPATH:~/.nix-profile/info

while read dir
do
	PYTHONPATH=$PYTHONPATH:$dir/site-packages
done < <(find ~/.nix-profile/lib -maxdepth 1 -name 'python*' | sort --reverse)
export PYTHONPATH

export PERL5LIB=$PERL5LIB:~/.nix-profile/lib/site_perl/5.10.0:~/.nix-profile/lib/5.10.0

# Paths for software compilation

export LIBRARY_PATH=$LIBRARY_PATH:~/.nix-profile/lib
export C_INCLUDE_PATH=$C_INCLUDE_PATH:~/.nix-profile/include
export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:~/.nix-profile/include
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:~/.nix-profile/lib/pkgconfig:~/.nix-profile/share/pkgconfig
