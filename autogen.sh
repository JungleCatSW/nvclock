#!/bin/sh

echo "#undef VERSION" > config.h.in

#Gentoo (1.6x?) contains a buggy version of aclocal which results
#in missing macros for the configure script like:
#>./configure: line 533: syntax error near unexpected token `config.h'
#>./configure: line 533: `AM_CONFIG_HEADER(config.h)'
if [ -e /etc/gentoo-release ]
then
    cp acinclude.m4 aclocal.m4
fi

aclocal
autoheader
autoconf

