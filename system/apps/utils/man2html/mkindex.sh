#!/bin/sh

base=/atheos/Documentation
rpath=man
out=$base/$rpath

index=$out/index.html

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

# "Main"

header;

for d in $(ls $out | grep man);do
	url=index_$(echo $d | sed -e s/man//).html;
	name="Manual Section $(echo $d | sed -e s/man//)";
	add_link;
done;

footer;

