Haystack 
========
Epigenetic Variability and Motif Analysis Pipeline       
--------------------------------------------------
**Current version**: v0.5

[![Build Status](https://travis-ci.org/rfarouni/haystack_bio.svg?branch=master)](https://travis-ci.org/rfarouni/haystack_bio)


[![install with bioconda](https://img.shields.io/badge/install%20with-bioconda-brightgreen.svg?style=flat-square)](http://bioconda.github.io/recipes/haystack_bio/README.html)

Summary
-------
Haystack is a suite of computational tools to study  epigenetic variability, cross-cell-type plasticity of chromatin states and transcription factors (TFs) motifs providing mechanistic insights into chromatin structure, cellular identity and gene regulation. 

Haystack identifies  highly variable regions across different cell types also called _hotspots_, and the potential regulators that mediate the cell-type specific variation through integration of multiple data-types. 

Haystack can be used with  histone modifications data, DNase I hypersensitive sites data and methylation data obtained for example by ChIP-seq, DNase-Seq and Bisulfite-seq assays and measured across multiple cell-types. In addition, it  is also possible to integrate gene expression data obtained from array based or RNA-seq approaches.

In particular, haystack highlights enriched TF motifs in  variable and cell-type specific regions and quantifies their activity and specificity on nearby genes if gene expression data are available.

A summary of the pipeline and an example on H3k27ac data is shown in the following figure:

![Haystack Pipeline](http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/Final_figure.png)


**(A)** Haystack overview: modules and corresponding functions. **(B)** Hotspot analysis on H3k27ac: signal tracks, variability track and the hotspots of variability are computed from the ChIP-seq aligned data; in addition, the regions specific for a given cell type are extracted.  **(C)** Motif analysis on the regions specific for the H1hesc cell line: Pou5f1::Sox2 is significant; p-value and q-value, motif logo and average profile are calculated. **(D)** Transcription factor activity for Sox2 in H1esc (star) compared to the other cell types (circles), x-axis specificity of Sox2 expression (z-score), y-axis effect (z-score) on the gene nearby the regions containing the Sox2 motif.   

Haystack was designed to be highly modular. The whole pipeline can be called using the _haystack_pipeline_ command or alternatively the different modules can be used and combined indipendently.  For example it is possible to use only the motif analysis calling the _haystack_motifs_ module on a given set of genomic regions. A nice description of each module is present in the **_How to use HAYSTACK_** section.

Operating System Notes
-----------------

Haystack supports only 64-bit Linux and Mac OSX. The software has been tested on Ubuntu 14.04 LTS, Ubuntu 16.04 LTS, OS X 10.11, and OS X 10.12.  


Installation 
-----------------


To install haystack, bioconda needs to be configured on your system. For detailed installation instructions please visit 
the bioconda project's installation webpage at https://bioconda.github.io/. After you have bioconda configured, 
you can install haystack and its dependencies by simply running:

    conda install haystack_bio

Testing 
-----------------

To test if the package was successfully installed, please run the following command.

     haystack_hotspots -h
  
The  *-h* help flag generates a description of all the command line options that can be supplied to each of the individual modules. 
  

To test haystack, you would first need to download the genome file and then run the the pipeline on the sample data bundled inside the package. 
To download the human genome for example, please run:

     haystack_download_genome hg19
     
To test the software on the test sample data, please run  
    
         haystack_run_test

Note, that this is equivalent to running the following command manually.

    haystack_pipeline samples_names_genes.txt hg19 --output_directory $HOME/haystack_test_output --bin_size 200 --chrom_exclude 'chr(?!21)'

Here *haystack_test_output* is the name of the analysis output folder. 


How to use haystack
-------------------
Haystack consists of 5 modules:

1) **haystack_hotspots**: find the regions that are variable across different ChIP-seq, DNase-seq or Bisulfite-seq tracks (only BigWig processed file are supported for methylation data). The input is a folder containing bam files (with PCR duplicates removed) or bigwig (must be .bw), or a tab delimited text file with two columns containing: 1. the sample name and 2. the path of the corresponding .bam/.bw file. For example you can write inside a file called _samples_names_hotspot.txt_ something like that:
```
K562	./INPUT_DATA/K562H3k27ac_sorted_rmdup.bam	
GM12878	./INPUT_DATA/Gm12878H3k27ac_sorted_rmdup.bam	
HEPG2	./INPUT_DATA/Hepg2H3k27ac_sorted_rmdup.bam	
H1hesc	./INPUT_DATA/H1hescH3k27ac_sorted_rmdup.bam	
HSMM	./INPUT_DATA/HsmmH3k27ac_sorted_rmdup.bam	
NHLF	./INPUT_DATA/NhlfH3k27ac_sorted_rmdup.bam
```
The output will consist of:
- The normalized bigwig files for each track
- The hotspots i.e. the regions that are most variable
- The regions that are variable and specific for each track, this means that the signal is more enriched to a particular track compared to the rest.
- A session file (.xml) for the IGV software (http://www.broadinstitute.org/igv/) from the Broad Institute to easily visualize all the tracks produced, the hotspots and the specific regions for each cell line. To load it just drag and drop the file _OPEN_ME_WITH_IGV.xml_ from the output folder on top of the IGV window or alternatively load it in IGV with File-> Open Session... If you have trouble opening the file please update your IGV version. Additonaly, please don't move the .xml file only, you need all the files in the output folder to correctly load the session.

**_Examples_**
Suppose you have a folder called /users/luca/mybamfolder you can run the variability analysis with: 
	
	haystack_hotspots /users/luca/mybamfolder hg19

If you have instead a file with the samples description, like the _samples_names_hotspot.txt_  you can run the variability analysis with: 
	
	haystack_hotspots samples_names_hotspot.txt  hg19

**IMPORTANT:** if are running haystack_hotspots using bigwig files you need to add the option: **--input_is_bigwig**

2) **haystack_motifs**: find enriched transcription factor motifs in a given set of genomic regions
The input is a set of regions in .bed format (http://genome.ucsc.edu/FAQ/FAQformat.html#format1) and the reference genome, the output consist of an HTML report with:
- motif enriched with p and q values
- motif profiles and logos
- list of regions with a particular motifs and coordinates of the motifs in those regions
- list of closest genes to the regions with a particular motif 

**_Examples_**
To analyze the bed file file _myregions.bed_ on the _hg19_ genome run:
	
	haystack_motifs myregions.bed hg19

To specify a custom background file for the analysis, for example _mybackgroundregions.bed_ run:
	
	haystack_motifs myregions.bed hg19 --bed_bg_filename mybackgroundregions.bed

To use a particular motif database (the default is JASPAR) use:
	
	haystack_motifs myregions.bed hg19 --meme_motifs_filename my_database.meme

The database file must be in the MEME format: http://meme.nbcr.net/meme/doc/meme-format.html#min_format

3) **haystack_tf_activity_plane**: quantifies the specificity and the activity of the TFs highlighed by the **haystack_motif** integrating gene expression data.

The input consist of an 1. output folder of the **haystack_motif** tool, 2. a set of files containing gene expression data specified in a tab delimed file and 3. the target cell-type name to use to perfom the analysis. Each gene expression data file must be a tab delimited text file with two columns: 1. gene symbol 2. gene expression value. Such a file (one for each cell-type profiled) should look like this:
```
RNF14	7.408579
UBE2Q1	9.107306
UBE2Q2	7.847002
RNF10	9.500193
RNF11	7.545264
LRRC31	3.477048
RNF13	7.670409
CBX4	7.070998
REM1	6.148991
REM2	5.957589
.
.
.
```
The file that describe the samples for example a file called  _sample_names_tf_activity.txt_ should contain something like this:
```
K562	./INPUT_DATA/K562_genes.txt
GM12878	./INPUT_DATA/GM12878_genes.txt
HEPG2	./INPUT_DATA//HEPG2_genes.txt
H1hesc	./INPUT_DATA/h1hesc_genes.txt
HSMM	./INPUT_DATA/HSMM_genes.txt
NHLF	./INPUT_DATA/NHLF_genes.txt
```

The output is a set of figures each containing the TF activity plane for a given motif.

**_Example_**
Suppose the utility **haystack_motif** created the folder called _HAYSTACK_MOTIFS_on_K562/_  analyzing the cell type named K562 and you have wrote the _sample_names_tf_activity.txt_ as above you can run the TF activity analysis with:

	haystack_tf_activity_plane HAYSTACK_MOTIFS_on_K562/ sample_names_tf_activity.txt K562

4) **haystack_pipeline**: executes the whole pipeline automatically, i.e. 1) and 2) and optionally 3) (if gene expression files are provided) finding hotspots, specific regions, motifs and quantifiying their activity on nearby genes.

The input is a tab delimited text file with two or three columns containing 1. the sample name 2. the path of the corresponding bam file 3. the path of the gene expression file with the same format described in 3); Note that this last column is optional. 

For example you can have a file called _samples_names.txt_ with something like that:
```
K562	./INPUT_DATA/K562H3k27ac_sorted_rmdup.bam	./INPUT_DATA/K562_genes.txt
GM12878	./INPUT_DATA/Gm12878H3k27ac_sorted_rmdup.bam	./INPUT_DATA/GM12878_genes.txt
HEPG2	./INPUT_DATA/Hepg2H3k27ac_sorted_rmdup.bam	./INPUT_DATA//HEPG2_genes.txt
H1hesc	./INPUT_DATA/H1hescH3k27ac_sorted_rmdup.bam	./INPUT_DATA/h1hesc_genes.txt
HSMM	./INPUT_DATA/HsmmH3k27ac_sorted_rmdup.bam	./INPUT_DATA/HSMM_genes.txt
NHLF	./INPUT_DATA/NhlfH3k27ac_sorted_rmdup.bam	./INPUT_DATA/NHLF_genes.txt
```

Alternatively you can specify a folder containing .bam files (with PCR duplicates removed) or .bw files (bigwig format).

**_Examples_**
Suppose you have a folder called /users/luca/mybamfolder you can run the command with: 

	haystack_pipeline /users/luca/mybamfolder hg19 

Note:In this case the pipeline run 1) and 2), but not 3) since no gene expression data are provided.

If you have instead a file with the samples description containing .bam or .bw  filenames (note: it is not possible to mix .bam and .bw) and gene expression data, like the _samples_names.txt_ described above you can run the whole pipeline with: 
	
	haystack_pipeline samples_names.txt  hg19

5) **haystack_download_genome**: it allows you to download and add a reference genomes from UCSC to haystack in the appropriate format. To download a genome run: 
	
	 haystack_download_genome genome_name 

**_Example_**
To download the human genome assembly hg19 run: 
	
	haystack_download_genome hg19

Note: Probably you don't need to call this command explicitly since it is called when the other commands need to download a particular assembly.

You can get more details about all the parameters of each of these 5 commands using the -h  or --help flag that prints a nice description.


Precomputed Analysis
--------------------

We have run haystack on several ENCODE datasets for which you can download the the precomputed results (variability tracks, hotspots, specific regions, enriched motifs and activity planes):

1. Analysis on 12 ChIP-seq tracks of H3k27ac in human cell lines + gene expression: http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/HAYSTACK_H3k27ac.tar.gz
2. Analysis on  17 DNase-seq tracks in human cell lines + gene expression: (Gain) http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/HAYSTACK_DNASE.tar.gz  and (Loss) http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/HAYSTACK_DNASE_DEPLETED.tar.gz
3. Analysis on  10 RRBS-seq tracks of DNA-Methylation in human cell lines + gene expression: http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/HAYSTACK_Methylation.tar.gz
4. Analysis on 17 ChIP-seq tracks of H3k27me3 in human cell lines + gene expression: http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/HAYSTACK_H3k27me3.tar.gz


Docker Image
------------
If you like Docker, we also provide a Docker image:
	
	https://hub.docker.com/r/lucapinello/haystack_bio/

To use the image first install Docker: http://docker.com

Then type the command:

	docker pull lucapinello/haystack_bio

See an example on how to run haystack  with a Docker image see the section **Testing haystack ** below. __If you get memory errors try to allocate at least 8GB to the docker image in order to run haystack __.

The current version is compatible only with Unix like operating systems on 64 bit architectures and was tested on:
- CentOS 6.5
- Debian 6.0
- Ubuntu 12.04 and 14.04 LTS
- OSX Maverick and Mountain Lion


To test the whole pipeline you can download this set of bam file from the ENCODE project:
http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/haystack_test_dataset_h3k27ac.tar.gz

Decompress the file with the following command: 
	
	tar xvzf haystack_test_dataset_h3k27ac.tar.gz
	
Go into the folder with the test data:

	cd TEST_DATASET
	
If you use a Docker image instead run with the following command:

	docker run -v ${PWD}:/DATA -w /DATA  -i lucapinello/haystack_bio haystack_pipeline samples_names.txt hg19 

If you run Docker on Window you have to specify the full path:

	docker run -v //c/Users/Luca/Downloads/TEST_DATASET:/DATA -w /DATA  -i lucapinello/haystack_bio haystack_pipeline samples_names.txt hg19 

This will recreate the panels and the plots showed in the figure present in the summary, plus other panels and plots for all the other cell-types contained in the test dataset.


 
Operating System Notes
----------------------
**UBUNTU (tested on 14.04 LTS) in the Amazon Web Service (AWS) Cloud**

1. Launch and connect to the Amazon Instance you have chosen from the AWS console (is suggested to use an m3.large ) or to your Ubuntu machine.

2. Create a swap partition (**this step is only for the AWS cloud**)
  ```
  sudo dd if=/dev/zero of=/mnt/swapfile bs=1M count=20096
  sudo chown root:root /mnt/swapfile
  sudo chmod 600 /mnt/swapfile
  sudo mkswap /mnt/swapfile
  sudo swapon /mnt/swapfile
  sudo sh -c "echo '/mnt/swapfile swap swap defaults 0 0' >> /etc/fstab"
  sudo swapon -a
  ```
3. Install dependencies
  ```
  sudo apt-get update && sudo apt-get update && sudo apt-get install git wget default-jre python-setuptools python-pip  python-dev python-numpy  python-scipy python-matplotlib python-pandas python-imaging python-setuptools unzip ghostscript make gcc g++ zlib1g-dev zlib1g -y 
  ```
  
  __If you are installing it on a docker image you don't need the sudo before each apt-get command__

4. Install haystack 
  ```
  sudo pip install haystack_bio --no-use-wheel --verbose
  ```
 
5. Download and run the test dataset
  ```
  wget http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/haystack_test_dataset_h3k27ac.tar.gz
  tar xvzf haystack_test_dataset_h3k27ac.tar.gz
  cd TEST_DATASET
  haystack_pipeline samples_names.txt hg19
  ```
  
  All the results will be stored in the folder HAYSTACK_PIPELINE_RESULT	

**Apple OSX**

To install haystack on OSX you need the _Command Line Tools_ (usually shipped with Xcode). 
If you don't have them you can download from here: 
https://developer.apple.com/downloads/index.action

You may need to create a free apple developer account.

To generate the motif logo you need a recent version of XQuartz, download and install the dmg from here: http://xquartz.macosforge.org/landing/.

Updating from Yosemite may break the motif logo generation.
If you don't see the motif logo in the output of the haystack_motifs utility, please install the latest version XQuartz:http://xquartz.macosforge.org/landing/.

Alternatively if you don't want to update XQuartz you can fix the problem from the terminal typing the following commands:
```
sudo ln -s /opt/X11 /usr/X11
sudo ln -s /opt/X11 /usr/X11R6
```

In addition, you need to install Java for Windows.

Note: If you install haystack in a custom folder please make sure to select a path without white spaces.


Citation
--------
*Please cite the following article if you use haystack in your research*:
  * Luca Pinello, Jian Xu, Stuart H. Orkin, and Guo-Cheng Yuan. Analysis of chromatin-state plasticity identifies cell-type specific regulators of H3K27me3 patterns PNAS 2014; published ahead of print January 6, 2014, doi:10.1073/pnas.1322570111

Contacts
--------
Please send any comment or bug to lpinello AT jimmy DOT harvard DOT edu 

Third part software included and used in this distribution
-----------------------------------------------------------
1. PeakAnnotator: http://www.ebi.ac.uk/research/bertone/software
2. FIMO from the MEME suite (4.11.1): http://meme.nbcr.net/meme/
3. WebLogo: http://weblogo.berkeley.edu/logo.cgi
4. Sambamba (0.6.6): http://lomereiter.github.io/sambamba/
5. Bedtools (2.20.1): https://github.com/arq5x/bedtools2
6. bedGraphToBigWig and bigWigAverageOverBed from the UCSC Kent's Utilities: http://hgdownload.cse.ucsc.edu/admin/jksrc.zip
