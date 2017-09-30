#!/usr/bin/env bash 
mkdir ${HOME}/haystack_genomes
wget -O data_h3k27ac_6cells.zip https://www.dropbox.com/s/4yjx7ypj0c82ryh/data_h3k27ac_6cells.zip?dl=1
unzip data_h3k27ac_6cells.zip -d $HOME
rm data_h3k27ac_6cells.zip
cd $HOME/data_h3k27ac_6cells
rm -Rf HAYSTACK_PIPELINE_RESULTS

docker pull pinellolab/haystack_bio
docker run -v ${PWD}:/docker_data \
           -v ${HOME}/haystack_genomes:/haystack_genomes \
           -w /docker_data -it pinellolab/haystack_bio haystack_pipeline samples_names.txt hg19  --blacklist hg19


