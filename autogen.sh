#!/bin/sh

aclocal -I m4
libtoolize --automake
autoheader
automake -a
autoconf
