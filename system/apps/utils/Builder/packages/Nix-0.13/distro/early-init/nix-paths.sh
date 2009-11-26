# Run paths

#export DLL_PATH=~/.nix-profile/lib:$DLL_PATH
export LD_LIBRARY_PATH=~/.nix-profile/lib:$LD_LIBRARY_PATH

while read dir
do
	PYTHONPATH=$dir/site-packages:$PYTHONPATH
done < <(find ~/.nix-profile/lib -maxdepth 1 -name python* | sort)
export PYTHONPATH

# Paths for software compilation

export LIBRARY_PATH=~/.nix-profile/lib:$LIBRARY_PATH
export C_INCLUDE_PATH=~/.nix-profile/include:$C_INCLUDE_PATH
export CPLUS_INCLUDE_PATH=~/.nix-profile/include:$CPLUS_INCLUDE_PATH
export PKG_CONFIG_PATH=~/.nix-profile/lib/pkgconfig:$PKG_CONFIG_PATH
