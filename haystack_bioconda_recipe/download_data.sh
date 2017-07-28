#!/bin/bash
set -e
set -o pipefail
set -x

if [[ -z "$1" ]]; then
    echo
    echo "Downloads sample data to <outdir>"
    echo "Usage: $0 <outdir>"
    echo
    exit 1
fi

OUTDIR=$(readlink -f $1)
mkdir -p $OUTDIR
TMP=$(mktemp -d)
cd $TMP
curl -O -L https://github.com/rfarouni/Haystack/archive/v0.5.0.tar.gz > v0.5.0.tar.gz
tar -xf v0.5.0.tar.gz
cd Haystack-v0.5.0
unzip haystack.zip
mv $TMP/Haystack-v0.5.0/haystack_dependencies/genomes $OUTDIR
mv $TMP/Haystack-v0.5.0/haystack_dependencies/testdata $OUTDIR
mv $TMP/Haystack-v0.5.0/haystack_dependencies/gene_annotations $OUTDIR
mv $TMP/Haystack-v0.5.0/haystack_dependencies/motif_databases $OUTDIR
mv $TMP/Haystack-v0.5.0/haystack_dependencies/extra $OUTDIR
