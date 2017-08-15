*haystack_bio* 
========
Epigenetic Variability and Motif Analysis Pipeline       
--------------------------------------------------
**Current version**: v0.5

[![Build Status](https://travis-ci.org/rfarouni/haystack_bio.svg?branch=master)](https://travis-ci.org/rfarouni/haystack_bio)


[![install with bioconda](https://img.shields.io/badge/install%20with-bioconda-brightgreen.svg?style=flat-square)](http://bioconda.github.io/recipes/haystack_bio/README.html)

Summary
-------
***haystack_bio*** is a suite of computational tools to study  epigenetic variability, cross-cell-type plasticity of chromatin states and transcription factors (TFs) motifs providing mechanistic insights into chromatin structure, cellular identity and gene regulation. 

haystack_bio identifies  highly variable regions across different cell types also called _hotspots_, and the potential regulators that mediate the cell-type specific variation through integration of multiple data-types. 

haystack_bio can be used with  histone modifications data, DNase I hypersensitive sites data and methylation data obtained for example by ChIP-seq, DNase-Seq and Bisulfite-seq assays and measured across multiple cell-types. In addition, it  is also possible to integrate gene expression data obtained from array based or RNA-seq approaches.

In particular, haystack highlights enriched TF motifs in  variable and cell-type specific regions and quantifies their activity and specificity on nearby genes if gene expression data are available.

A summary of the pipeline and an example on H3k27ac data is shown in the following figure:

![Haystack Pipeline](http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/Final_figure.png)


**(A)** ***haystack_bio*** overview: modules and corresponding functions. **(B)** Hotspot analysis on H3k27ac: signal tracks, variability track and the hotspots of variability are computed from the ChIP-seq aligned data; in addition, the regions specific for a given cell type are extracted.  **(C)** Motif analysis on the regions specific for the H1hesc cell line: Pou5f1::Sox2 is significant; p-value and q-value, motif logo and average profile are calculated. **(D)** Transcription factor activity for Sox2 in H1esc (star) compared to the other cell types (circles), x-axis specificity of Sox2 expression (z-score), y-axis effect (z-score) on the gene nearby the regions containing the Sox2 motif.   

***haystack_bio*** was designed to be highly modular. The whole pipeline can be called using the _haystack_pipeline_ command or alternatively the different modules can be used and combined indipendently.  For example it is possible to use only the motif analysis calling the _haystack_motifs_ module on a given set of genomic regions. A nice description of each module is present in the **_How to use haystack_bio_** section.


Installation 
----------------

**Operating System Notes**

The software has been tested on CentOS 6.5, Ubuntu 14.04 LTS, Ubuntu 16.04 LTS, OS X 10.11, and OS X 10.12. 
Although ***haystack_bio*** supports only 64-bit Linux and Mac OSX, it can be run on Windows systems using a Docker container. 
The Docker container can also be run on any platform that has the Docker software installed. For instructions on how to install the Docker software and the docker image, please see below.

**Bioconda Installation for Linux and MacOS**

***haystack_bio*** depends on several other Python packages and a number of bioinformatics software tools in order to run. The easiest way to install ***haystack_bio*** is through Bioconda, 
a software repository channel for the conda package manager, configured on your system. Bioconda streamlines the process of building and installing any software dependency that a package requires. 
By running the following lines of code, ***haystack_bio*** and its dependencies can be installed automatically. 
The entire installation process consists of three steps: installing Miniconda, adding the Bioconda channel, and installing ***haystack_bio***. 
For the following steps, we are assuming you are on a 64-bit Linux or a MacOS system and that you have not installed Miniconda/Anaconda before on your system. 
If you have Minicoda/Anaconda 3 alread installed, you would need to create a separate environment for Python 2.7. 
Please consult [conda.io](https://conda.io/docs/py2or3.html#create-python-2-or-3-environments) for details. 
In case you encounter difficulty in installing Miniconda or adding the Bioconda channel, 
please refer to the Bioconda project's [website](https://bioconda.github.io/) for more detailed installation instructions.

**Step 1**: Download the latest Miniconda 2 (Python 2.7) to your home directory.

For Linux
    
    wget -c https://repo.continuum.io/miniconda/Miniconda2-latest-Linux-x86_64.sh -O $HOME/miniconda.sh

For Mac:

    wget -c https://repo.continuum.io/miniconda/Miniconda2-latest-MacOSX-x86_64.sh -O $HOME/miniconda.sh

 
**Step 2**: Give permissions to execute the file as a program and execute the program
 
    chmod +x $HOME/miniconda.sh
    $HOME/miniconda.sh -b

**Step 3**: Update the conda repository and add the repository channels in the following order of priority by running these four lines in the order shown.

     conda update --all --yes
     conda config --add channels defaults
     conda config --add channels conda-forge
     conda config --add channels bioconda

**Step 4**: Install ***haystack_bio*** and its dependencies by simply running:

    conda install haystack_bio
    
For other installation options, please consult the section *Other Installation Options*   
    

Testing Installation
-----------------

To test if the package was successfully installed, please run the following command.

     haystack_hotspots -h
  
The  *-h* help flag outputs a list of all the possible command line options that you can supply to the *haystack_hotspots* module. 
If you see such a list, then the software has been installed successfully.


To test the ***haystack_bio*** pipeline, you would first need to download the genome file and then run the the pipeline on the sample data bundled inside the package. 

To download the human genome assembly hg19, please run: 
	
	haystack_download_genome hg19

Also note that this command downloads the hg19 genome build from the [UCSC server](http://hgdownload.cse.ucsc.edu/goldenPath/hg19/bigZips/hg19.2bit)

and saves it to the genomes folder

    $HOME/miniconda/lib/python2.7/site-packages/haystack/haystack_data/genomes
  
Also note that the *hg19.2bit* file is 778M in size and could take a long time to download on a slow connection.
  
  
To test whether the entire pipeline can run without any problems on your system, please run  
    
         haystack_run_test

Note, that this is equivalent to running the following command manually.

    haystack_pipeline samples_names.txt hg19 --output_directory $HOME/haystack_test_output  --blacklist hg19 --chrom_exclude 'chr(?!21)'

Note: Since the sample test data included in the package is limited to  a small fraction of the genome (i.e. Chromosome 21) and is for four  cell types only,
the final motif analysis step in the pipeline might not return any positive findings. The test run output can be found in the folder $HOME/haystack_test_output. 


How to use *haystack_bio*
-------------------

***haystack_bio*** consists of the following three modules.

1) **haystack_hotspots**: finds regions that are variable across different ChIP-seq, DNase-seq or Bisulfite-seq tracks (only BigWig processed file are supported for methylation data). 
2) **haystack_motifs**: finds enriched transcription factor motifs in a given set of genomic regions.
3) **haystack_tf_activity_plane**: quantifies the specificity and the activity of the TFs highlighed by the **haystack_motif** integrating gene expression data.


The command **haystack_pipeline**  executes the whole pipeline automatically. That is, it executes Module 1 followed by Module 2 and Module 3 (if gene expression files are provided), 
finding hotspots, specific regions, motifs and quantifying their activity on nearby genes.


### Walk-Through Example Using Data from Six Cell Types
-------------------

In this section, we showcase the commands using the example we have in the paper. We recreate the output produced by the pipeline when running on the entire genome with six cell types.
 
Step 1: Open a command terminal in the directory of your choosing and download the complete set of data files as an archive file to that directory.
 
    wget http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/haystack_test_dataset_h3k27ac.tar.gz

Step 2: Decompress the archive file
	
	tar xvzf haystack_test_dataset_h3k27ac.tar.gz
	
The command will untar the data files archive into a folder called TEST_DATASET. Inside it you will see a _samples_names.txt_ file containing the relative paths to the data files.	
The input is a tab delimited text file with two or three columns containing 1. the sample name 2. the path of the corresponding bam file 3. the path of the gene expression file with the same format described in 3); Note that this last column is optional.

```
K562	./INPUT_DATA/K562H3k27ac_sorted_rmdup.bam	./INPUT_DATA/K562_genes.txt
GM12878	./INPUT_DATA/Gm12878H3k27ac_sorted_rmdup.bam	./INPUT_DATA/GM12878_genes.txt
HEPG2	./INPUT_DATA/Hepg2H3k27ac_sorted_rmdup.bam	./INPUT_DATA//HEPG2_genes.txt
H1hesc	./INPUT_DATA/H1hescH3k27ac_sorted_rmdup.bam	./INPUT_DATA/h1hesc_genes.txt
HSMM	./INPUT_DATA/HsmmH3k27ac_sorted_rmdup.bam	./INPUT_DATA/HSMM_genes.txt
NHLF	./INPUT_DATA/NhlfH3k27ac_sorted_rmdup.bam	./INPUT_DATA/NHLF_genes.txt
```

Step 4: Run the pipeline 
	
	 haystack_pipeline.py ./TEST_DATASET/samples_names.txt  hg19 --output_directory $HOME/HAYSTACK_OUTPUT_H3K27Aac --blacklist hg19 

The command saves the output to the folder "HAYSTACK_OUTPUT_H3K27Aac" in your home directory. All the results will be stored in inside the sub-folder HAYSTACK_PIPELINE_RESULT. 
This will recreate the panels and the plots showed in the figure present in the summary, plus other panels and plots for all the other cell-types contained in the test dataset.

 
Alternatively you can just provide the folder containing the BAM or bigwig files, but in this case the pipeline runs Module 1 and  Module 2, but not Module 3, since no gene expression data are provided.

	haystack_pipeline ./TEST_DATASET/ hg19 --output_directory $HOME/HAYSTACK_OUTPUT_H3K27Aac --blacklist hg19

-------------------

The inputs and outputs of the three modules of the pipeline are as follows.

### Module 1: *haystack_hotspots*


**Input**: The input is a folder containing BAM or bigwig files, or a tab delimited text file with two columns containing: 1. the sample name and 2. the path of the corresponding .bam/.bw file. For example you can write inside a file called _samples_names_hotspot.txt_ something like that:

```
K562	./INPUT_DATA/K562H3k27ac_sorted_rmdup.bam	
GM12878	./INPUT_DATA/Gm12878H3k27ac_sorted_rmdup.bam	
HEPG2	./INPUT_DATA/Hepg2H3k27ac_sorted_rmdup.bam	
H1hesc	./INPUT_DATA/H1hescH3k27ac_sorted_rmdup.bam	
HSMM	./INPUT_DATA/HsmmH3k27ac_sorted_rmdup.bam	
NHLF	./INPUT_DATA/NhlfH3k27ac_sorted_rmdup.bam
```

If you have instead a file with the samples description you can run the variability analysis with: 
	
	haystack_hotspots ./TEST_DATASET/samples_names.txt  hg19 --output_directory $HOME/HAYSTACK_OUTPUT_H3K27Aac --blacklist hg19

Alternatively, you can run the variability analysis without creating a _samples_names.txt_ file by providing the folder name in which the BAM files are located
	
	haystack_hotspots ./TEST_DATASET/ hg19 --blacklist hg19

**Output**: The output consists of:
- The normalized bigwig files for each track
- The hotspots i.e. the regions that are most variable
- The regions that are variable and specific for each track, this means that the signal is more enriched to a particular track compared to the rest.
- A session file (.xml) for the IGV software (http://www.broadinstitute.org/igv/) from the Broad Institute to easily visualize all the tracks produced, the hotspots and the specific regions for each cell line. To load it just drag and drop the file _OPEN_ME_WITH_IGV.xml_ from the output folder on top of the IGV window or alternatively load it in IGV with File-> Open Session... If you have trouble opening the file please update your IGV version. Additonaly, please don't move the .xml file only, you need all the files in the output folder to correctly load the session.

<p align="center">
  <img src="./figures/hotspots.png">
</p>


### Module 2: **haystack_motifs**


**Input**: The input is a set of regions in .bed format (http://genome.ucsc.edu/FAQ/FAQformat.html#format1) and the reference genome.

**_Examples_**
To analyze the bed file file _myregions.bed_ on the _hg19_ genome run:
	
	haystack_motifs myregions.bed hg19

To specify a custom background file for the analysis, for example _mybackgroundregions.bed_ run:
	
	haystack_motifs myregions.bed hg19 --bed_bg_filename mybackgroundregions.bed

To use a particular motif database (the default is JASPAR) use:
	
	haystack_motifs myregions.bed hg19 --meme_motifs_filename my_database.meme

The database file must be in the MEME format: http://meme.nbcr.net/meme/doc/meme-format.html#min_format


**Output**: The output consists of an HTML report with
- enriched motifs with corresponding p and q values
- motif profiles and logos
- list of regions with a particular motifs and coordinates of the motifs in those regions
- list of closest genes to the regions with a particular motif 

<p align="center">
  <img src="./figures/motif.png">
</p>

3) **haystack_tf_activity_plane**

**Input**: The input consists of 
- An output folder of the **haystack_motif** tool
- A set of files containing gene expression data specified in a tab delimited file
- The target cell-type name to use to perform the analysis. Each gene expression data file must be a tab delimited text file with two columns: 
    1. gene symbol 
    2. gene expression value. 
  
Such a file (one for each cell-type profiled) should look like this.
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
The file that describe the samples for example a file called  _sample_names_tf_activity.txt_ should have the following format.
```
K562	./INPUT_DATA/K562_genes.txt
GM12878	./INPUT_DATA/GM12878_genes.txt
HEPG2	./INPUT_DATA//HEPG2_genes.txt
H1hesc	./INPUT_DATA/h1hesc_genes.txt
HSMM	./INPUT_DATA/HSMM_genes.txt
NHLF	./INPUT_DATA/NHLF_genes.txt
```

**Output**: The output is a set of figures each containing the TF activity plane for a given motif.

Notes	
-------------------
	
**IMPORTANT:** Folder names and file paths should not have white spaces. Please use underscore instead. 	

**Note:** If you are running haystack_hotspots using bigwig files you need to add the option: **--input_is_bigwig**

The **haystack_download_genome** command allows you to download and add a reference genomes from UCSC to ***haystack_bio*** in the appropriate format. 
To download a  particular genome run: 
	
	 haystack_download_genome genome_name 

Probably you do not need to call this command explicitly since it is called automatically when you run the pipeline. 

Other Installation Options
-------------------

### **Docker Image**


Please visit [docker.com](https://www.docker.com/) for instructions on how to install Docker on your platform. 
For Linux platforms, make sure to run the Docker post-installation instructions in order to run the command without *sudo* privileges. 
After the installation is complete you can download the Docker image for ***haystack_bio***  onto your system by simply running:

	docker pull lucapinello/haystack_bio


To run ***haystack_bio*** use the following command:

	docker run -v ${PWD}:/DATA -w /DATA  -i lucapinello/haystack_bio haystack_pipeline samples_names.txt hg19 
	
If you get memory errors try to allocate at least 8GB to the docker image in order to run ***haystack_bio***. 
If you run Docker on Window you have to specify the full path of the data:

	docker run -v //c/Users/Luca/Downloads/TEST_DATASET:/DATA -w /DATA  -i lucapinello/haystack_bio haystack_pipeline samples_names.txt hg19 

### **Manual Installation**

For manual installation please execute the command

    python setup.py install 

after having all the depenencies instaled.

Citation
--------
*Please cite the following article if you use ***haystack_bio*** in your research*:

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