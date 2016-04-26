from __future__ import division
import os
import sys
import time
import subprocess as sb
import glob
import shutil
import numpy as np
from scipy.stats import zscore
import pandas as pd
import matplotlib as mpl
mpl.use('Agg')
import pylab as pl
import argparse
import logging
from bioutilities import Genome_2bit
import xml.etree.cElementTree as ET
import pickle as cp

#commmon functions
from haystack_common import determine_path,query_yes_no,which,logging,error,warn,debug,info,check_file,CURRENT_PLATFORM,HAYSTACK_VERSION,system_env


def quantile_normalization(A):
        AA = np.zeros_like(A)
        I = np.argsort(A,axis=0)
        AA[I,np.arange(A.shape[1])] =np.mean(A[I,np.arange(A.shape[1])],axis=1)[:,np.newaxis]

        return AA


def smooth(x,window_len=200,window='hanning'):
    if not window in ['flat', 'hanning', 'hamming', 'bartlett', 'blackman']:
        raise ValueError, "Window is on of 'flat', 'hanning', 'hamming', 'bartlett', 'blackman'"


    s=np.r_[x[window_len-1:0:-1],x,x[-1:-window_len:-1]]
    if window == 'flat': #moving average
        w=np.ones(window_len,'d')
    else:
        w=eval('np.'+window+'(window_len)')
    y=np.convolve(w/w.sum(),s,mode='valid')
    return y[window_len/2:-window_len/2+1]

#write the IGV session file
def rem_base_path(path,base_path):
            return path.replace(os.path.join(base_path,''),'')

def find_th_rpm(df_chip,th_rpm):
    return np.min(df_chip.apply(lambda x: np.percentile(x,th_rpm)))

def log2_transform(x):
    return np.log2(x+1)
    
def angle_transform(x):
    return np.arcsin(np.sqrt(x)/1000000.0)

#def run_haystack():

def main():

    print '\n[H A Y S T A C K   H O T S P O T]'
    print('\n-SELECTION OF VARIABLE REGIONS- [Luca Pinello - lpinello@jimmy.harvard.edu]\n')
    print 'Version %s\n' % HAYSTACK_VERSION
    
    
    if which('samtools') is None:
            error('Haystack requires samtools free available at: http://sourceforge.net/projects/samtools/files/samtools/0.1.19/')
            sys.exit(1)
    
    if which('bedtools') is None:
            error('Haystack requires bedtools free available at: https://github.com/arq5x/bedtools2/releases/tag/v2.20.1')
            sys.exit(1)
    
    if which('bedGraphToBigWig') is None:
            info('To generate the bigwig files Haystack requires bedGraphToBigWig please download from here: http://hgdownload.cse.ucsc.edu/admin/exe/ and add to your PATH')
    
    #mandatory
    parser = argparse.ArgumentParser(description='HAYSTACK Parameters')
    parser.add_argument('samples_filename_or_bam_folder', type=str,  help='A tab delimeted file with in each row (1) a sample name, (2) the path to the corresponding bam filename. Alternatively it is possible to specify a folder containing some .bam files to analyze.' )
    parser.add_argument('genome_name', type=str,  help='Genome assembly to use from UCSC (for example hg19, mm9, etc.)')
    
    #optional
    parser.add_argument('--bin_size', type=int,help='bin size to use(default: 500bp)',default=500)
    parser.add_argument('--disable_quantile_normalization',help='Disable quantile normalization (default: False)',action='store_true')
    parser.add_argument('--th_rpm',type=float,help='Percentile on the signal intensity to consider for the hotspots (default: 99)', default=99)
    parser.add_argument('--transformation',type=str,help='Variance stabilizing transformation among: none, log2, angle (default: angle)',default='angle',choices=['angle', 'log2', 'none'])
    parser.add_argument('--recompute_all',help='Ignore any file previously precalculated',action='store_true')
    parser.add_argument('--z_score_high', type=float,help='z-score value to select the specific regions(default: 1.5)',default=1.5)
    parser.add_argument('--z_score_low', type=float,help='z-score value to select the not specific regions(default: 0.25)',default=0.25)
    parser.add_argument('--name',  help='Define a custom output filename for the report', default='')
    parser.add_argument('--output_directory',type=str, help='Output directory (default: current directory)',default='')
    parser.add_argument('--use_X_Y', help='Force to process the X and Y chromosomes (default: not processed)',action='store_true')
    parser.add_argument('--max_regions_percentage', type=float , help='Upper bound on the %% of the regions selected  (deafult: 0.1, 0.0=0%% 1.0=100%%)' , default=0.1)
    parser.add_argument('--depleted', help='Look for cell type specific regions with depletion of signal instead of enrichment',action='store_true')
    parser.add_argument('--input_is_bigwig', help='Use the bigwig format instead of the bam format for the input. Note: The files must have extension .bw',action='store_true')
    parser.add_argument('--version',help='Print version and exit.',action='version', version='Version %s' % HAYSTACK_VERSION)
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
            
    #check folder or sample filename
    if os.path.isfile(samples_filename_or_bam_folder):
            BAM_FOLDER=False
            bam_filenames=[]
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
                        sample_names.append(fields[0])
                        bam_filenames.append(fields[1])
                    else:
                        error('The samples file format is wrong!')
                        sys.exit(1)
    
            
    else:
            if os.path.exists(samples_filename_or_bam_folder):
                    BAM_FOLDER=True
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
    
    info('Initializing Genome:%s' %genome_name)
    
    genome_directory=determine_path('genomes')
    genome_2bit=os.path.join(genome_directory,genome_name+'.2bit')
    
    if os.path.exists(genome_2bit):
            genome=Genome_2bit(genome_2bit)
    else:
            info("\nIt seems you don't have the required genome file.")
            if query_yes_no('Should I download it for you?'):
                    sb.call('haystack_download_genome %s' %genome_name,shell=True,env=system_env)
                    if os.path.exists(genome_2bit):
                            info('Genome correctly downloaded!')
                            genome=Genome_2bit(genome_2bit)
                    else:
                            error('Sorry I cannot download the required file for you. Check your Internet connection.')
                            sys.exit(1)
            else:
                    error('Sorry I need the genome file to perform the analysis. Exiting...')
                    sys.exit(1)
    
    chr_len_filename=os.path.join(genome_directory, "%s_chr_lengths.txt" % genome_name)
    check_file(chr_len_filename)
    
    
    if name:
            directory_name='HAYSTACK_HOTSPOTS_on_%s' % name
    
    else:
            directory_name='HAYSTACK_HOTSPOTS'
    
    
    if output_directory:
            output_directory=os.path.join(output_directory,directory_name)
    else:
            output_directory=directory_name
            
    
    if not os.path.exists(output_directory):
    	os.makedirs(output_directory)    
    
                    
    genome_sorted_bins_file=os.path.join(output_directory,'%s.%dbp.bins.sorted.bed' %(os.path.basename(genome_name),bin_size))
    
    
    tracks_directory=os.path.join(output_directory,'TRACKS')
    if not os.path.exists(tracks_directory):
            os.makedirs(tracks_directory)   
    
    
    intermediate_directory=os.path.join(output_directory,'INTERMEDIATE')
    if not os.path.exists(intermediate_directory):
            os.makedirs(intermediate_directory)   
    
    if not os.path.exists(genome_sorted_bins_file) or recompute_all:
            info('Creating bins of %dbp for %s in %s' %(bin_size,chr_len_filename,genome_sorted_bins_file))
            sb.call('bedtools makewindows -g %s -w %s |  bedtools sort -i stdin |' %  (chr_len_filename, bin_size)+ "perl -nle 'print "+'"$_\t$.";'+"' /dev/stdin> %s" % genome_sorted_bins_file,shell=True,env=system_env)
            
    
    #convert bam files to genome-wide rpm tracks
    for base_name,bam_filename in zip(sample_names,bam_filenames):
    
        info('Processing:%s' %bam_filename)
        
        rpm_filename=os.path.join(tracks_directory,'%s.bedgraph' %base_name)
        binned_rpm_filename=os.path.join(intermediate_directory,'%s.%dbp.rpm' % (base_name,bin_size))
        bigwig_filename=os.path.join(tracks_directory,'%s.bw' %base_name)
    
        if  input_is_bigwig and which('bigWigAverageOverBed'):
                        if not os.path.exists(binned_rpm_filename) or recompute_all:
                                cmd='bigWigAverageOverBed %s %s  /dev/stdout | sort -s -n -k 1,1 | cut -f5 > %s' % (bam_filename,genome_sorted_bins_file,binned_rpm_filename)
                                sb.call(cmd,shell=True,env=system_env)
                                shutil.copy2(bam_filename,bigwig_filename)
    
        else:    
                if not os.path.exists(binned_rpm_filename) or recompute_all:
                        info('Computing Scaling Factor...')
                        cmd='samtools view -c -F 512 %s' % bam_filename
                        #print cmd
                        proc=sb.Popen(cmd, stdout=sb.PIPE,shell=True,env=system_env)
                        (stdout, stderr) = proc.communicate()
                        #print stdout,stderr
                        scaling_factor=(1.0/float(stdout.strip()))*1000000
    
                        info('Scaling Factor: %e' %scaling_factor)
    
                        info('Building BedGraph RPM track...')
                        cmd='samtools view -b -F 512 %s | bamToBed | slopBed  -r %s -l 0 -s -i stdin -g %s | genomeCoverageBed -g  %s -i stdin -bg -scale %.32f > %s'  %(bam_filename,bin_size,chr_len_filename,chr_len_filename,scaling_factor,rpm_filename)
                        #print cmd
    
                
                        proc=sb.call(cmd,shell=True,env=system_env)
    
                if which('bedGraphToBigWig'):
                    if not os.path.exists(bigwig_filename) or recompute_all:
                            info('Converting BedGraph to BigWig')
                            cmd='bedGraphToBigWig %s %s %s' %(rpm_filename,chr_len_filename,bigwig_filename)
                            proc=sb.call(cmd,shell=True,env=system_env)
    
                else:
                    info('Sorry I cannot create the bigwig file.\nPlease download and install bedGraphToBigWig from here: http://hgdownload.cse.ucsc.edu/admin/exe/ and add to your PATH')
    
                if not os.path.exists(binned_rpm_filename) or recompute_all:      
                        info('Make constant binned (%dbp) rpm values file' % bin_size)
                        cmd='bedtools sort -i %s |  bedtools map -a %s -b stdin -c 4 -o mean -null 0.0 | cut -f5 > %s'   %(rpm_filename,genome_sorted_bins_file,binned_rpm_filename)
                        proc=sb.call(cmd,shell=True,env=system_env)
                
                try:    
                        os.remove(rpm_filename)
                except:
                        pass
    
    
    #load coordinates of bins
    coordinates_bin=pd.read_csv(genome_sorted_bins_file,names=['chr_id','bpstart','bpend'],sep='\t',header=None,usecols=[0,1,2])
    N_BINS=coordinates_bin.shape[0]
    if not use_X_Y:
        coordinates_bin=coordinates_bin.ix[(coordinates_bin['chr_id']!='chrX') & (coordinates_bin['chr_id']!='chrY')]  
    
    #load all the tracks
    info('Loading the processed tracks') 
    df_chip={}
    for state_file in  glob.glob(os.path.join(intermediate_directory,'*.rpm')):
            col_name=os.path.basename(state_file).replace('.rpm','')
            df_chip[col_name]=pd.read_csv(state_file,squeeze=True,header=None)
            info('Loading:%s' % col_name)
    
    df_chip=pd.DataFrame(df_chip)
    
    if disable_quantile_normalization:
            info('Skipping quantile normalization...')
    else:
            info('Normalizing the data...')
            df_chip=pd.DataFrame(quantile_normalization(df_chip.values),columns=df_chip.columns,index=df_chip.index)
    
    
    if which('bedGraphToBigWig'):        
            #write quantile normalized tracks
            coord_quantile=coordinates_bin.copy()
            for col in df_chip:
    
                if disable_quantile_normalization:
                        normalized_output_filename=os.path.join(tracks_directory,'%s.bedgraph' % os.path.basename(col))
                else:
                        normalized_output_filename=os.path.join(tracks_directory,'%s_quantile_normalized.bedgraph' % os.path.basename(col))
                        
                normalized_output_filename_bigwig=normalized_output_filename.replace('.bedgraph','.bw')
      
                if not os.path.exists(normalized_output_filename_bigwig) or recompute_all:         
                        info('Writing binned track: %s' % normalized_output_filename_bigwig )    
                        coord_quantile['rpm_normalized']=df_chip.ix[:,col]
                        coord_quantile.dropna().to_csv(normalized_output_filename,sep='\t',header=False,index=False)
                
                        cmd='bedGraphToBigWig %s %s %s' %(normalized_output_filename,chr_len_filename,normalized_output_filename_bigwig)
                        proc=sb.call(cmd,shell=True,env=system_env)
                        try:
                                os.remove(normalized_output_filename)
                        except:
                                pass
    else:
            info('Sorry I cannot creat the bigwig file.\nPlease download and install bedGraphToBigWig from here: http://hgdownload.cse.ucsc.edu/admin/exe/ and add to your PATH')
         
            
    #th_rpm=np.min(df_chip.apply(lambda x: np.percentile(x,th_rpm)))
    th_rpm=find_th_rpm(df_chip,th_rpm)
    info('Estimated th_rpm:%s' % th_rpm)
    
    df_chip_not_empty=df_chip.ix[(df_chip>th_rpm).any(1),:]
    

    
    if transformation=='log2':
            df_chip_not_empty=df_chip_not_empty.applymap(log2_transform)
            info('Using log2 transformation')
    
    elif transformation =='angle':     
            df_chip_not_empty=df_chip_not_empty.applymap(angle_transform )
            info('Using angle transformation')
    
    else:
            info('Using no transformation')
            
    iod_values=df_chip_not_empty.var(1)/df_chip_not_empty.mean(1)
    
    ####calculate the inflation point a la superenhancers
    scores=iod_values
    min_s=np.min(scores)
    max_s=np.max(scores)
    
    N_POINTS=len(scores)
    x=np.linspace(0,1,N_POINTS)
    y=sorted((scores-min_s)/(max_s-min_s))
    m=smooth((np.diff(y)/np.diff(x)),50)
    m=m-1
    m[m<=0]=np.inf
    m[:int(len(m)*(1-max_regions_percentage))]=np.inf
    idx_th=np.argmin(m)+1
    
    #print idx_th,
    th_iod=sorted(iod_values)[idx_th]
    #print th_iod
    
    
    hpr_idxs=iod_values>th_iod
    #print len(iod_values),len(hpr_idxs),sum(hpr_idxs), sum(hpr_idxs)/float(len(hpr_idxs)),
    
    info('Selected %f%% regions (%d)' %( sum(hpr_idxs)/float(len(hpr_idxs))*100, sum(hpr_idxs)))
    coordinates_bin['iod']=iod_values
    
    #we remove the regions "without" signal in any of the cell types
    coordinates_bin.dropna(inplace=True)
    
    
    #create a track for IGV
    bedgraph_iod_track_filename=os.path.join(tracks_directory,'VARIABILITY.bedgraph')
    bw_iod_track_filename=os.path.join(tracks_directory,'VARIABILITY.bw')
    
    if not os.path.exists(bw_iod_track_filename) or recompute_all:   
    
            info('Generating variability track in bigwig format in:%s' % bw_iod_track_filename)
    
            coordinates_bin.to_csv(bedgraph_iod_track_filename,sep='\t',header=False,index=False)
            sb.call('bedGraphToBigWig %s %s %s' % (bedgraph_iod_track_filename,chr_len_filename,bw_iod_track_filename ),shell=True,env=system_env)
            try:
                    os.remove(bedgraph_iod_track_filename)
            except:
                    pass
    
    
    #Write the HPRs
    bedgraph_hpr_filename=os.path.join(tracks_directory,'SELECTED_VARIABILITY_HOTSPOT.bedgraph')
    
    to_write=coordinates_bin.ix[hpr_idxs[hpr_idxs].index]
    to_write.dropna(inplace=True)
    to_write['bpstart']=to_write['bpstart'].astype(int)
    to_write['bpend']=to_write['bpend'].astype(int)
    
    to_write.to_csv(bedgraph_hpr_filename,sep='\t',header=False,index=False)
    
    bed_hpr_fileaname=os.path.join(output_directory,'SELECTED_VARIABILITY_HOTSPOT.bed')
    
    if not os.path.exists(bed_hpr_fileaname) or recompute_all:  
            info('Writing the HPRs in: %s' % bed_hpr_fileaname)
            sb.call('bedtools sort -i %s | bedtools merge -i stdin >  %s' %(bedgraph_hpr_filename,bed_hpr_fileaname),shell=True,env=system_env)
    
    #os.remove(bedgraph_hpr_filename)
    
    df_chip_hpr=df_chip_not_empty.ix[hpr_idxs,:]
    df_chip_hpr_zscore=df_chip_hpr.apply(zscore,axis=1)
    
    
    specific_regions_directory=os.path.join(output_directory,'SPECIFIC_REGIONS')
    if not os.path.exists(specific_regions_directory):
            os.makedirs(specific_regions_directory)   
    
    if depleted:
            z_score_high=-z_score_high
            z_score_low=-z_score_low
    
    
    #write target
    info('Writing Specific Regions for each cell line...')
    coord_zscore=coordinates_bin.copy()
    for col in df_chip_hpr_zscore:
    
            regions_specific_filename='Regions_specific_for_%s_z_%.2f.bedgraph' % (os.path.basename(col).replace('.rpm',''),z_score_high)
            specific_output_filename=os.path.join(specific_regions_directory,regions_specific_filename)
            specific_output_bed_filename=specific_output_filename.replace('.bedgraph','.bed')
    
            if not os.path.exists(specific_output_bed_filename) or recompute_all:  
                    if depleted:
                            coord_zscore['z-score']=df_chip_hpr_zscore.ix[df_chip_hpr_zscore.ix[:,col]<z_score_high,col]
                    else:
                            coord_zscore['z-score']=df_chip_hpr_zscore.ix[df_chip_hpr_zscore.ix[:,col]>z_score_high,col]
                    coord_zscore.dropna().to_csv(specific_output_filename,sep='\t',header=False,index=False)
    
                    info('Writing:%s' % specific_output_bed_filename )
                    sb.call('bedtools sort -i %s | bedtools merge -i stdin >  %s' %(specific_output_filename,specific_output_bed_filename),shell=True,env=system_env)
    
    
    #write background
    info('Writing Background Regions for each cell line...')
    coord_zscore=coordinates_bin.copy()
    for col in df_chip_hpr_zscore:
    
            regions_bg_filename='Background_for_%s_z_%.2f.bedgraph' % (os.path.basename(col).replace('.rpm',''),z_score_low)
            bg_output_filename=os.path.join(specific_regions_directory,'Background_for_%s_z_%.2f.bedgraph' % (os.path.basename(col).replace('.rpm',''),z_score_low))
            bg_output_bed_filename=bg_output_filename.replace('.bedgraph','.bed')
    
            if not os.path.exists(bg_output_bed_filename) or recompute_all:
    
                    if depleted:
                            coord_zscore['z-score']=df_chip_hpr_zscore.ix[df_chip_hpr_zscore.ix[:,col]>z_score_low,col]
                    else:
                            coord_zscore['z-score']=df_chip_hpr_zscore.ix[df_chip_hpr_zscore.ix[:,col]<z_score_low,col]
                    coord_zscore.dropna().to_csv(bg_output_filename,sep='\t',header=False,index=False)
    
                    info('Writing:%s' % bg_output_bed_filename )
                    sb.call('bedtools sort -i %s | bedtools merge -i stdin >  %s' %(bg_output_filename,bg_output_bed_filename),shell=True,env=system_env)    
    
    
    ###plot selection
    pl.figure()
    pl.title('Selection of the HPRs')
    pl.plot(x,y,'r',lw=3)
    pl.plot(x[idx_th],y[idx_th],'*',markersize=20)
    pl.hold(True)
    x_ext=np.linspace(-0.1,1.2,N_POINTS)
    y_line=(m[idx_th]+1.0)*(x_ext -x[idx_th])+ y[idx_th];
    pl.plot(x_ext,y_line,'--k',lw=3)
    pl.xlim(0,1.1)
    pl.ylim(0,1)
    pl.xlabel('Fraction of bins')
    pl.ylabel('Score normalized')
    pl.savefig(os.path.join(output_directory,'SELECTION_OF_VARIABILITY_HOTSPOT.pdf'))
    pl.close()
    
    
    
    igv_session_filename=os.path.join(output_directory,'OPEN_ME_WITH_IGV.xml')
    info('Creating an IGV session file (.xml) in: %s' %igv_session_filename)
    
    session = ET.Element("Session")
    session.set("genome",genome_name)
    session.set("hasGeneTrack","true")
    session.set("version","7")
    resources = ET.SubElement(session, "Resources")
    panel= ET.SubElement(session, "Panel")
    
    resource_items=[]
    track_items=[]
    
    hpr_iod_scores=scores[scores>th_iod]
    min_h=np.mean(hpr_iod_scores)-2*np.std(hpr_iod_scores)
    max_h=np.mean(hpr_iod_scores)+2*np.std(hpr_iod_scores)
    mid_h=np.mean(hpr_iod_scores)
    #write the tracks
    for sample_name in sample_names:
        if disable_quantile_normalization:
                track_full_path=os.path.join(output_directory,'TRACKS','%s.%dbp.bw' % (sample_name,bin_size))
        else:
                track_full_path=os.path.join(output_directory,'TRACKS','%s.%dbp_quantile_normalized.bw' % (sample_name,bin_size))
    
        track_filename=rem_base_path(track_full_path,output_directory)        
    
        if os.path.exists(track_full_path):    
                resource_items.append( ET.SubElement(resources, "Resource"))
                resource_items[-1].set("path",track_filename)
                track_items.append(ET.SubElement(panel, "Track" ))
                track_items[-1].set('color',"0,0,178")
                track_items[-1].set('id',track_filename)
                track_items[-1].set("name",sample_name)
    
    resource_items.append(ET.SubElement(resources, "Resource"))
    resource_items[-1].set("path",rem_base_path(bw_iod_track_filename,output_directory))
    
    track_items.append(ET.SubElement(panel, "Track" ))
    track_items[-1].set('color',"178,0,0")
    track_items[-1].set('id',rem_base_path(bw_iod_track_filename,output_directory))
    track_items[-1].set('renderer',"HEATMAP")
    track_items[-1].set("colorScale","ContinuousColorScale;%e;%e;%e;%e;0,153,255;255,255,51;204,0,0" % (mid_h,min_h,mid_h,max_h))
    track_items[-1].set("name",'VARIABILITY')
    
    resource_items.append(ET.SubElement(resources, "Resource"))
    resource_items[-1].set("path",rem_base_path(bed_hpr_fileaname,output_directory))
    track_items.append(ET.SubElement(panel, "Track" ))
    track_items[-1].set('color',"178,0,0")
    track_items[-1].set('id',rem_base_path(bed_hpr_fileaname,output_directory))
    track_items[-1].set('renderer',"HEATMAP")
    track_items[-1].set("colorScale","ContinuousColorScale;%e;%e;%e;%e;0,153,255;255,255,51;204,0,0" % (mid_h,min_h,mid_h,max_h))
    track_items[-1].set("name",'HOTSPOTS')
    
    for sample_name in sample_names:
        track_full_path=glob.glob(os.path.join(output_directory,'SPECIFIC_REGIONS','Regions_specific_for_%s*.bedgraph' %sample_name))[0]    
        specific_track_filename=rem_base_path(track_full_path,output_directory)
        if os.path.exists(track_full_path):
                resource_items.append( ET.SubElement(resources, "Resource"))
                resource_items[-1].set("path",specific_track_filename)
    
                track_items.append(ET.SubElement(panel, "Track" ))
                track_items[-1].set('color',"178,0,0")
                track_items[-1].set('id',specific_track_filename)
                track_items[-1].set('renderer',"HEATMAP")
                track_items[-1].set("colorScale","ContinuousColorScale;%e;%e;%e;%e;0,153,255;255,255,51;204,0,0" % (mid_h,min_h,mid_h,max_h))
                track_items[-1].set("name",'REGION SPECIFIC FOR %s' % sample_name)
    
    tree = ET.ElementTree(session)
    tree.write(igv_session_filename,xml_declaration=True)
    
    info('All done! Ciao!')
    sys.exit(0)
