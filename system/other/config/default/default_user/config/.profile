PS1=['\u@\h:\w']

if [ -e ~/config/setupenv ]; then
. /bin/runparts ~/config/setupenv
fi

PATH=$PATH:./

cd $HOME
