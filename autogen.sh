#!/bin/sh

aclocal
libtoolize --copy --automake
autoheader
automake --add-missing --copy --gnu
autoconf
