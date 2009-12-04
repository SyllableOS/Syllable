# Look up modules in other locations

while read dir
do
	PYTHONPATH=$dir/site-packages:$PYTHONPATH
done < <(find /resources/indexes/lib -maxdepth 1 -name 'python*' | sort)
export PYTHONPATH
