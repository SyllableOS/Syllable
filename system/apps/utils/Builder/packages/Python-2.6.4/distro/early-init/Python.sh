# Look up modules in other locations

for dir in <(find /resources/indexes/lib -maxdepth 1 -name 'python*' | sort)
do
	PYTHONPATH=$dir/site-packages:$PYTHONPATH
done
export PYTHONPATH
