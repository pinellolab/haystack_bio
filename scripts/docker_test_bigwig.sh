#!/usr/bin/env bash 
mkdir ${HOME}/haystack_genomes
wget -O bigwigs_h3k27ac_6cells.zip https://www.dropbox.com/s/5x9gz54y4p1tsr8/h3k27ac_6cells_bigwigs.zip?dl=1
rm -Rf $HOME/bigwigs_h3k27ac_6cells
unzip bigwigs_h3k27ac_6cells.zip -d $HOME
rm bigwigs_h3k27ac_6cells.zip
cd $HOME/bigwigs_h3k27ac_6cells
rm -Rf HAYSTACK_PIPELINE_RESULTS

docker pull pinellolab/haystack_bio
docker run -v ${PWD}:/docker_data \
           -v ${HOME}/haystack_genomes:/haystack_genomes \
           -w /docker_data -it pinellolab/haystack_bio haystack_pipeline sample_names.txt hg19  --blacklist hg19 --input_is_bigwig

