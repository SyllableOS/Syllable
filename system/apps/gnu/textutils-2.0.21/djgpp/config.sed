# Replace "test -f" with "test -x" so that e.g. gcc.exe is found.
/for ac_dir in \$PATH; do/,/IFS="\$ac_save_[Ii][Ff][Ss]"/ {
  s|test -f \$ac_dir/|test -x $ac_dir/|
}

/IFS="\${IFS=/,/IFS="\$ac_save_ifs"/ {
    s|test -f \$ac_dir/|test -x $ac_dir/|
}

# Replace (command) > /dev/null with `command > /dev/null`, since
# parenthesized commands always return zero status in the ported Bash,
# even if the named command doesn't exist
/if ([^|;]*null/{
  s,(,`,
  s,),,
  s,;  *then,`; then,
}

# po2tbl.sed.in is invalid on MSDOS
s|po2tbl\.sed\.in|po2tbl-sed.in|g

# Additional editing of Makefiles
/ac_given_INSTALL=/,/^CEOF/ {
  /^s%@l@%/a\
  /TEXINPUTS=/s,:,\\\\\\\\\\\\\\;,g\
  /PATH=/s,:,\\\\\\\;,g\
  s,po2tbl\\.sed\\.in,po2tbl-sed.in,g\
  s,config\\.h\\.in,config.h-in,g\
  s,\\.env-warn,_env-warn,g\
  s,Makefile\\.in\\.in,Makefile.in-in,g\
  s,Makefile\\.am\\.in,Makefile.am-in,g\
  s,jm-winsz\\\([12]\\\)\\.m4,jm\\\1-winsz.m4,g\
  s,missing-,miss-,g\
  s,basic-0-,bas-0-,g
}

# Makefile.in.in is renamed to Makefile.in-in...
/^CONFIG_FILES=/,/^EOF/ {
  s|po/Makefile.in|po/Makefile.in:po/Makefile.in-in|
}

# ...and config.h.in into config.h-in
/^ *CONFIG_HEADERS=/,/^EOF/ {
  s|config.h|config.h:config.h-in|
}

# DOS-style absolute file names should be supported as well
/\*) srcdir=/s,/\*,/*|[A-z]:/*,
/\$]\*) INSTALL=/s,\[/\$\]\*,&|[A-z]:/*,
/\$]\*) ac_rel_source=/s,\[/\$\]\*,&|[A-z]:/*,

# Switch the order of the two Sed commands, since DOS path names
# could include a colon
/ac_file_inputs=/s,\( -e "s%\^%\$ac_given_srcdir/%"\)\( -e "s%:% $ac_given_srcdir/%g"\),\2\1,
