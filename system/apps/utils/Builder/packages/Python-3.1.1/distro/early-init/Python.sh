# Look up modules in other locations

# For main libpython.so:
export PYTHONPATH=/resources/Python/lib

for dir in `find /resources/indexes/lib -maxdepth 1 -name 'python*' | sort`
do
	PYTHONPATH=$dir/site-packages:$PYTHONPATH
done
