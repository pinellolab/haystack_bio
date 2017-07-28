#!/bin/bash

$PYTHON setup.py install

outdir=$PREFIX/share/$PKG_NAME-$PKG_VERSION-$PKG_BUILDNUM
echo $outdir

