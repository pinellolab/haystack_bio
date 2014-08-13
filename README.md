HAYSTACK
========
Epigenetic Variability and Motif Analysis Pipeline       
--------------------------------------------------
**Current version**: 0.1

Summary
-------
Haystack is a suite of computational tools to study 
epigenetic variability, cross-cell-type plasticity of chromatin states and transcription factors (TFs) motifs providing mechanistic insights into chromatin structure, cellular identity and gene regulation. 

Haystack identifies  highly variable regions across different cell types also called _hotspots_, and the potential regulators that mediate the cell-type specific variation through integration of multiple data-types. 

Haystack can be used with  histone modifications data, DNase I hypersensitive sites data and methylation data obtained for example by ChIP-seq, DNase-Seq and Bisulfite-seq assays and measured across multiple cell-types. In addition, it  is also possible to integrate gene expression data obtained from array based or RNA-seq approaches.

In particualar, Haystack highlights enriched TF motifs in  variable and cell-type specific regions and quantifies their activity and specificity on nearby genes if gene expression data are available.

A summary of the pipeline and an example on H3k27ac data is shown in the following figure:

![Haystack Pipeline](http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/figure_paper.png)

(A) Haystack overview: modules and corresponding functions. (B) Hotspot analysis on H3k27ac: signal tracks, variability track and the hotspots of variability are computed from the ChIP-seq aligned data; in addition, the regions specific for a given cell type are extracted.  (C) Motif analysis on the regions specific for the H1hesc cell line: Sox2 is significant; q-value, motif logo and average profile are calculated. (D) Transcription factor activity for Sox2 in H1esc (red star) compared to the other cell types (black circles), x-axis specificity of Sox2 expression (z-score), y-axis effect (z-score) on the gene nearby the regions containing the Sox2 motif.  

Haystack was designed to be highly modular. The whole pipeline can be called using the _haystack_pipeline_ command or alternatively the different modules can be used and combined indipendently.  For example it is possible to use only the motif analysis calling the _haystack_motifs_ module on a given set of genomic regions. A nice description of each module is present in the **_How to use HAYSTACK_** section.


Installation
------------
First download and extract the setup zip file from here: 

https://github.com/lucapinello/HAYSTACK/archive/master.zip

or from here if you want preloaded the human and mouse genomes (hg19 and mm9):

http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/haystack_setup_with_genomes.zip

From the Haystack-master folder created run the following command:

  <code>python INSTALL_HAYSTACK.py</code>

The script will try to install HAYSTACK in your HOME folder and will setup all the dependencies, eventually asking to install automatically the missing ones for you.

If you want to install HAYSTACK on a different location run the following command:

  <code>python INSTALL_HAYSTACK.py /MY_FOLDER </code>

The current version is compatible only with Unix like operating systems on 64 bit architectures and was tested on:
- CentOS 6.5
- Debian 6.0
- Ubuntu 12.04 LTS
- OSX Maverick and Mountain Lion
- Cygwin x86_64 (1.7) on Windows 7/8 64 bit

Operating System Notes
----------------------
**Apple OSX**

To install HAYSTACK on OSX you need the _Command Line Tools_ (usually shipped with Xcode). 
If you don't have them you can download from here: 
https://developer.apple.com/downloads/index.action

You may need to create a free apple developer account.

**CYGWIN**

On Cygwin, please be sure to have the following packages installed **before** running the setup:
```
ghostscript
python 
python-numpy
perl
gcc-core
gcc-g++
make
zlib-devel
libfreetype6
libfreetype-devel 
liblapack-dev
libpng-devel 
python-gtk2.0 
libgtk2.0-devel
libuuid-devel
libmysqlclient-dev
```

In addition, you need to install Java for Windows.

Note: If you install HAYSTACK in a custom folder please make sure to select a path without white spaces.

It is reaccomended to perform a rebase of Cygwin after the installation:
 
1. Open a Windows command shell (cmd) 
2. type <code>cd \cygwin64\bin</code>  
3. type <code>ash /usr/bin/rebaseall</code>

If you get strange errors about  _"address space is already occupied"_  when running HAYSTACK your antivirus may interfeer with the Cygwin as explained here: 
http://stackoverflow.com/questions/11107155/how-to-fix-address-space-is-already-occupied-error-on-fetch-commit

Try to temporaly disable it and restart Cygwin.

How to use HAYSTACK
-------------------
HAYSTACK consists of 5 modules:

1) **haystack_hotspots**: find the regions that are variable across different ChIP-seq, DNase-seq or Bisulfite-seq tracks. The input is a folder containing bam files (with PCR duplicates removed) or bigwig (must be .bw), or a tab delimited text file with two columns containing: 1. the sample name and 2. the path of the corresponding .bam/.bw file. For example you can write inside a file called _samples_names_hotspot.txt_ something like that:
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
- A session file (.xml) for the IGV software (http://www.broadinstitute.org/igv/) from the Broad Institute to easily visualize all the tracks produced, the hotspots and the specific regions for each cell line. To load it just drag and drop the file _OPEN_ME_WITH_IGV.xml_ from the output folder on top of the IGV window or alternatively load it in IGV with File-> Open Session...

**_Examples_**
Suppose you have a folder called /users/luca/mybamfolder you can run the variability analysis with: 
	
	haystack_hotspots /users/luca/mybamfolder hg19
If you have instead a file with the samples description, like the _samples_names_hotspot.txt_  you can run the variability analysis with: 
	
	haystack_hotspots samples_names_hotspot.txt  hg19
  	
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

4) **haystack_pipeline**: executes the wholw pipeline automatically, i.e. 1) and 2) and optionally 3) (if gene expression files are provided) finding hotspots, specific regions, motifs and quantifiying their activity on nearby genes.

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

5) **download_genome**: it allows you to download and add a reference genomes from UCSC to Haystack in the appropriate format. To download a genome run: 
	
	 download_genome genome_name 

**_Example_**
To download the human genome assembly hg19 run: 
	
	download_genome hg19

Note: Probably you don't need to call this command explicitely since it is called when the other commands need to download a particular assembly.

You can get more details about all the parameters of each of these 5 commands using the -h  or --help flag that prints a nice description.


Testing HAYSTACK
----------------

To test the whole pipeline you can download this set of bam file from the ENCODE project:
http://bcb.dfci.harvard.edu/~lpinello/HAYSTACK/haystack_test_dataset_h3k27ac.tar.gz

Decompress the file with the following command: 
	
	tar xvzf test_haystack_h3k27ac.tar.gz
	
Go into the folder with the test data:

	cd TEST_DATASET

Then run the haystack_pipeline command using the provided samples_names.txt file :

	haystack_pipeline samples_names.txt hg19
	
This will recreate the panels and the plots showed in the figure present in the summary, plus other panels and plots for all the other cell-types contained in the test dataset.

Citation
--------
*Please cite the following article if you use HAYSTACK in your research*:
  * Luca Pinello, Jian Xu, Stuart H. Orkin, and Guo-Cheng Yuan. Analysis of chromatin-state plasticity identifies cell-type specific regulators of H3K27me3 patterns PNAS 2014; published ahead of print January 6, 2014, doi:10.1073/pnas.1322570111

Contacts
--------
Please send any comment or bug to lpinello AT jimmy DOT harvard DOT edu 

Third part software included and used in this distribution
-----------------------------------------------------------
1. PeakAnnotator: http://www.ebi.ac.uk/research/bertone/software
2. FIMO from the MEME suite (4.9.1): http://meme.nbcr.net/meme/
3. WebLogo: http://weblogo.berkeley.edu/logo.cgi
4. Samtools (0.1.19): http://samtools.sourceforge.net/
5. Bedtools (2.20.1): https://github.com/arq5x/bedtools2
6. bedGraphToBigWig and bigWigAverageOverBed from the UCSC Kent's Utilities: http://hgdownload.cse.ucsc.edu/admin/jksrc.zip
