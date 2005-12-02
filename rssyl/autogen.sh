echo "Running aclocal ..."
aclocal
echo "Running autoheader ..."
autoheader
echo "Running libtoolize --force ..."
libtoolize --force
echo "Running automake --add-missing ..."
automake --add-missing
echo "Running autoconf ..."
autoconf
echo "Done!"
