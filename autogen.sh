#!/bin/sh

aclocal -I m4
libtoolize --copy --automake
autoheader
automake --add-missing --copy --gnu
autoconf
