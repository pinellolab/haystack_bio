from __future__ import division
import os
import sys
import time
import subprocess as sb
import glob
import shutil
import argparse
import logging

HAYSTACK_VERSION=0.1

logging.basicConfig(level=logging.INFO,
                    format='%(levelname)-5s @ %(asctime)s:\n\t %(message)s \n',
                    datefmt='%a, %d %b %Y %H:%M:%S',
                    stream=sys.stderr,
                    filemode="w"
                    )
error   = logging.critical		
warn    = logging.warning
debug   = logging.debug
info    = logging.info



def check_file(filename):
    try:
        with open(filename): pass
    except IOError:
        error('I cannot open the file:'+filename)
        sys.exit(1)



print '\n[H A Y S T A C K   P I P E L I N E]'
print('\n-SELECTION OF HOTSPOTS OF VARIABILITY AND ENRICHED MOTIFS- [Luca Pinello - lpinello@jimmy.harvard.edu]\n')

#mandatory
parser = argparse.ArgumentParser(description='HAYSTACK Parameters')
parser.add_argument('samples_filename_or_bam_folder', type=str,  help='A tab delimeted file with in each row (1) a sample name, (2) the path to the corresponding bam filename, (3 optional) the path to the corresponding gene expression filaneme. Alternatively it is possible to specify a folder containing some .bam files to analyze.')
parser.add_argument('genome_name', type=str,  help='Genome assembly to use from UCSC (for example hg19, mm9, etc.)')

#optional
parser.add_argument('--name',  help='Define a custom output filename for the report', default='')
parser.add_argument('--output_directory',type=str, help='Output directory (default: current directory)',default='')
parser.add_argument('--bin_size', type=int,help='bin size to use(default: 500bp)',default=500)
parser.add_argument('--recompute_all',help='Ignore any file previously precalculated fot the command haystack_hotstpot',action='store_true')
parser.add_argument('--depleted', help='Look for cell type specific regions with depletion of signal instead of enrichment',action='store_true')
parser.add_argument('--input_is_bigwig', help='Use the bigwig format instead of the bam format for the input. Note: The files must have extension .bw',action='store_true')
parser.add_argument('--disable_quantile_normalization',help='Disable quantile normalization (default: False)',action='store_true')
parser.add_argument('--transformation',type=str,help='Variance stabilizing transformation among: none, log2, angle (default: angle)',default='angle',choices=['angle', 'log2', 'none'])
parser.add_argument('--version',help='Print version and exit.',action='version', version='Version %.1f' % HAYSTACK_VERSION)

args = parser.parse_args()
args_dict=vars(args)
for key,value in args_dict.items():
        exec('%s=%s' %(key,repr(value)))

if input_is_bigwig:
        extension_to_check='.bw'
        info('Input is set BigWig (.bw)')
else:
        extension_to_check='.bam'
        info('Input is set compressed SAM (.bam)')

if name:
        directory_name='HAYSTACK_PIPELINE_RESULTS_on_%s' % name

else:
        directory_name='HAYSTACK_PIPELINE_RESULTS'

if output_directory:
        output_directory=os.path.join(output_directory,directory_name)
else:
        output_directory=directory_name

#check folder or sample filename

USE_GENE_EXPRESSION=True

if os.path.isfile(samples_filename_or_bam_folder):
        BAM_FOLDER=False
        bam_filenames=[]
        gene_expression_filenames=[]
        sample_names=[]

        with open(samples_filename_or_bam_folder) as infile:
            for line in infile:

                if not line.strip():
                        continue
                
                if line.startswith('#'): #skip optional header line or empty lines
                        info('Skipping header/comment line:%s' % line)
                        continue

                fields=line.strip().split()
                n_fields=len(fields)

                if n_fields==2:

                    USE_GENE_EXPRESSION=False
                    
                    sample_names.append(fields[0])
                    bam_filenames.append(fields[1])

                elif n_fields==3:

                    USE_GENE_EXPRESSION=USE_GENE_EXPRESSION and True

                    sample_names.append(fields[0])
                    bam_filenames.append(fields[1])
                    gene_expression_filenames.append(fields[2])
                else:
                    error('The samples file format is wrong!')
                    sys.exit(1)
        
else:
        if os.path.exists(samples_filename_or_bam_folder):
                BAM_FOLDER=True
                USE_GENE_EXPRESSION=False
                bam_filenames=glob.glob(os.path.join(samples_filename_or_bam_folder,'*'+extension_to_check))

                if not bam_filenames:
                    error('No bam/bigwig  files to analyze in %s. Exiting.' % samples_filename_or_bam_folder)
                    sys.exit(1)
                
                sample_names=[os.path.basename(bam_filename).replace(extension_to_check,'') for bam_filename in bam_filenames]
        else:
                error("The file or folder %s doesn't exist. Exiting." % samples_filename_or_bam_folder)
                sys.exit(1)


#check all the files before starting
info('Checking samples files location...')
for bam_filename in bam_filenames:
        check_file(bam_filename)

if USE_GENE_EXPRESSION:
    for gene_expression_filename in gene_expression_filenames:
            check_file(gene_expression_filename)

if not os.path.exists(output_directory):
        os.makedirs(output_directory)

#copy back the file used
if not BAM_FOLDER:
        shutil.copy2(samples_filename_or_bam_folder,output_directory)

#write hotspots conf files
sample_names_hotspots_filename=os.path.join(output_directory,'sample_names_hotspots.txt')

with open(sample_names_hotspots_filename,'w+') as outfile:
    for sample_name,bam_filename in zip(sample_names,bam_filenames):
        outfile.write('%s\t%s\n' % (sample_name, bam_filename))

#write tf activity  conf files
if USE_GENE_EXPRESSION:
        sample_names_tf_activity_filename=os.path.join(output_directory,'sample_names_tf_activity.txt')

        with open(sample_names_tf_activity_filename,'w+') as outfile:
                for sample_name,gene_expression_filename in zip(sample_names,gene_expression_filenames):
                        outfile.write('%s\t%s\n' % (sample_name, gene_expression_filename))

        tf_activity_directory=os.path.join(output_directory,'HAYSTACK_TFs_ACTIVITY_PLANES')


#CALL HAYSTACK HOTSPOTS
cmd_to_run='haystack_hotspots %s %s --output_directory %s --bin_size %d %s %s %s %s %s' % \
            (sample_names_hotspots_filename, genome_name,output_directory,bin_size,
             ('--recompute_all' if recompute_all else ''),
             ('--depleted' if depleted else ''),
             ('--input_is_bigwig' if input_is_bigwig else ''),
             ('--disable_quantile_normalization' if disable_quantile_normalization else ''),
             '--transformation %s' % transformation )
print cmd_to_run
sb.call(cmd_to_run ,shell=True)        

#CALL HAYSTACK MOTIFS
motif_directory=os.path.join(output_directory,'HAYSTACK_MOTIFS')
for sample_name in sample_names:
    specific_regions_filename=os.path.join(output_directory,'HAYSTACK_HOTSPOTS','SPECIFIC_REGIONS','Regions_specific_for_%s*.bed' %sample_name)
    bg_regions_filename=glob.glob(os.path.join(output_directory,'HAYSTACK_HOTSPOTS','SPECIFIC_REGIONS','Background_for_%s*.bed' %sample_name))[0]
    #bg_regions_filename=glob.glob(specific_regions_filename.replace('Regions_specific','Background')[:-11]+'*.bed')[0] #lo zscore e' diverso...
    #print specific_regions_filename,bg_regions_filename
    cmd_to_run='haystack_motifs %s %s --bed_bg_filename %s --output_directory %s --name %s' % (specific_regions_filename,genome_name, bg_regions_filename,motif_directory, sample_name)
    print cmd_to_run
    sb.call(cmd_to_run,shell=True)

    if USE_GENE_EXPRESSION:
            #CALL HAYSTACK TF ACTIVITY 
            motifs_output_folder=os.path.join(motif_directory,'HAYSTACK_MOTIFS_on_%s' % sample_name) 
            if os.path.exists(motifs_output_folder):
                cmd_to_run='haystack_tf_activity_plane %s %s %s --output_directory %s'  %(motifs_output_folder,sample_names_tf_activity_filename,sample_name,tf_activity_directory)
                print cmd_to_run
                sb.call(cmd_to_run,shell=True) 
     
