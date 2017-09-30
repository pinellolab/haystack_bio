#!/usr/bin/env bash 
mkdir ${HOME}/haystack_genomes
wget -O bigwigs_h3k27ac_6cells.zip  https://www.dropbox.com/s/puz6azpn5z51lm6/bigwigs_h3k27ac_6cells.zip?dl=1
unzip bigwigs_h3k27ac_6cells.zip -d $HOME
rm bigwigs_h3k27ac_6cells.zip
cd $HOME/bigwigs_h3k27ac_6cells
rm -Rf HAYSTACK_PIPELINE_RESULTS

docker pull pinellolab/haystack_bio
docker run -v ${PWD}:/docker_data \
           -v ${HOME}/haystack_genomes:/haystack_genomes \
           -w /docker_data -it pinellolab/haystack_bio haystack_pipeline sample_names.txt hg19  --blacklist hg19 --input_is_bigwig

