#!/bin/sh

aclocal
libtoolize --copy --automake
automake --add-missing --copy --gnu
autoheader
autoconf
