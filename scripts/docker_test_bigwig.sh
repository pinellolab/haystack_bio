#!/usr/bin/env bash 
docker pull pinellolab/haystack_bio
mkdir ${HOME}/haystack_genomes
wget https://www.dropbox.com/s/puz6azpn5z51lm6/bigwigs_h3k27ac_6cells.zip?dl=1
unzip bigwigs_h3k27ac_6cells.zip?dl=1 -d $HOME
cd $HOME/bigwigs_h3k27ac_6cells

docker run -v ${PWD}:/docker_data \
           -v ${HOME}/haystack_genomes:/haystack_genomes \
           -w /docker_data -it pinellolab/haystack_bio haystack_pipeline samples_names.txt hg19  --blacklist hg19 --input_is_bigwig


