#!/bin/sh

index=$2

header()
{
	echo "<BODY><HTML>" > $index
	echo "<HR ALIGN=CENTER>">> $index
}

footer()
{
	echo "</BODY></HTML>" >> $index
}

add_link()
{
	echo -n "<A HREF=" >> $index
	echo -n "$url" >> $index
	echo -n ">" >> $index
	echo -n "$name" >> $index
	echo "</A><BR>" >> $index
}

mandir()
{
	echo -n "<H2>Manual Section " >> $index
	echo -n $(echo $dir | sed -e s/man// ) >> $index
	echo "</H2>" >> $index

	for f in $(ls $dir);do
		url=$dir/$f;
		name=$(echo $f | sed -e s/.html// );
		add_link;
	done;

	echo -n "<BR><HR ALIGN=CENTER><BR>" >> $index
}

# "Main"

header;

dir=$1;
mandir;

footer;

