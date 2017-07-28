#!/bin/bash

$PYTHON setup.py install

HAYSTACK_DEPENDENCIES_FOLDER=$PREFIX/share/haystack_data
python setup.py install

echo $outdir

