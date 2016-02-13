#!/bin/bash

if [[ -z $2 ]]; then
	echo "Usage $0 <package> <version>"
	exit
fi

if [[ ! -d $1 ]]; then
	echo "No such pacakge $1 to bundle"
	exit
fi

PKG=${1%%/}

tar jcvf ${PKG}-${2}.tar.bz2 ${PKG}/
gpg -s -a -b ${PKG}-${2}.tar.bz2
