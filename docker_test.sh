#!/usr/bin/env bash 

docker pull lucapinello/haystack_bio
mkdir ${HOME}/haystack_genomes
wget https://www.dropbox.com/s/4yjx7ypj0c82ryh/data_h3k27ac_6cells.zip?dl=1
unzip data_h3k27ac_6cells.zip?dl=1 -d $HOME
cd $HOME/data_h3k27ac_6cells

docker run -v ${PWD}:${HOME}/data_h3k27ac_6cells \
           -v ${HOME}/haystack_genomes:/haystack_genomes \
           -w ${HOME}/data_h3k27ac_6cells -it lucapinello/haystack_bio haystack_pipeline samples_names.txt hg19  --blacklist hg19



