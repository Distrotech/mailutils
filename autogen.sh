#!/bin/sh

# This trick forces automake to install COPYING
trap "mv $$.COPYING.LIB COPYING.LIB" 1 2 3 15 
mv COPYING.LIB $$.COPYING.LIB
autoreconf -f -i -s
mv $$.COPYING.LIB COPYING.LIB