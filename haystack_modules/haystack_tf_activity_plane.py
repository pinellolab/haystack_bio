from __future__ import division
import os
import sys
import time

import matplotlib as mpl
mpl.use('Agg')

import pandas as pd
import glob
import os
import sys
import pylab as plt
import numpy as np
import argparse
from itertools import chain
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

print '\n[H A Y S T A C K   T F  A C T I V I T Y  P L A N E]'
print('\n-TFs Activity on Gene Expression- [Luca Pinello - lpinello@jimmy.harvard.edu]\n')

class FileWrapper(file):
    def __init__(self, comment_literal, *args):
        super(FileWrapper, self).__init__(*args)
        self._comment_literal = comment_literal

    def next(self):
        while True:
            line = super(FileWrapper, self).next()
            if not line.startswith(self._comment_literal):
                return line

def determine_path(folder):
    return os.path.dirname(which('haystack_tf_activity_plane')).replace('bin',folder)


def which(program):
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None

def check_file(filename):
    try:
        with open(filename): pass
    except IOError:
        error('I cannot open the file:'+filename)
        sys.exit(1)


def zscore_series(series):
    return (series - series.mean()) / np.std(series, ddof = 0)

#mandatory
parser = argparse.ArgumentParser(description='HAYSTACK Parameters',prog='haystack_tf_activity_plane')
parser.add_argument('haystack_motifs_output_folder', type=str,  help='A path to a folder created by the haystack_motifs utility')
parser.add_argument('gene_expression_samples_filename', type=str,  help='A file containing the list of sample names and locations')
parser.add_argument('target_cell_type', type=str,  help='The sample name to use as a target for the analysis')

#optional
parser.add_argument('--motif_mapping_filename', type=str, help='Custom motif to gene mapping file (the default is for JASPAR CORE 2014 database)')
parser.add_argument('--output_directory',type=str, help='Output directory (default: current directory)',default='')
parser.add_argument('--name',  help='Define a custom output filename for the report', default='')
parser.add_argument('--plot_all',  help='Disable the filter on the TF activity and correlation (default z-score TF>0 and rho>0.3)',action='store_true')
parser.add_argument('--version',help='Print version and exit.',action='version', version='Version %.1f' % HAYSTACK_VERSION)

args = parser.parse_args()

args_dict=vars(args)
for key,value in args_dict.items():
    exec('%s=%s' %(key,repr(value)))


if not os.path.exists(haystack_motifs_output_folder):
    error("The haystack_motifs_output_folder specified: %s doesn't exist!")
    sys.exit(1)

check_file(gene_expression_samples_filename)



if motif_mapping_filename:
     check_file(motif_mapping_filename)
else:
    motif_mapping_filename=os.path.join(determine_path('motif_databases'),'JASPAR_CORE_2014_vertebrates_mapped_to_gene_human_mouse.txt')
    
if name:
    directory_name='HAYSTACK_TFs_ACTIVITY_PLANES_on_'+name
else:
    directory_name='HAYSTACK_TFs_ACTIVITY_PLANES_on_'+target_cell_type

if output_directory:
    output_directory=os.path.join(output_directory, directory_name)
else:
    output_directory=directory_name    

#read motif to gene mapping file
def group_motif_mapping(x):
    gene_names=map(str,list(x.GENES.values))
    gene_names=','.join(list(chain.from_iterable(map(lambda z: z.split(','),gene_names))))
    return pd.Series({'MOTIF_ID':x.MOTIF_ID.values[0],'MOTIF_NAME':x.MOTIF_NAME.values[0],'GENES':gene_names})

motif_mapping=pd.read_table(motif_mapping_filename,header=None,names=['MOTIF_ID','MOTIF_NAME','GENES'],index_col=0)
motif_mapping=motif_mapping.reset_index().groupby('MOTIF_ID').apply(group_motif_mapping)
motif_mapping=motif_mapping.set_index('MOTIF_ID')

#load mapping filename
df_gene_mapping=pd.read_table(FileWrapper("#",gene_expression_samples_filename, "r"),header=None,index_col=0,names=['Sample_name','Sample_file'])

if target_cell_type not in df_gene_mapping.index:
    error('\nThe target_cell_type must be among these sample names:\n\n%s' %'\t'.join(df_gene_mapping.index.values))
    sys.exit(1)

#load gene expression and calculate ranking
gene_values=[]
for sample_name in df_gene_mapping.index:
    info('Load gene expression file for :%s' % sample_name)
    gene_values.append(pd.read_table(df_gene_mapping.ix[sample_name,'Sample_file'],index_col=0,names=['Gene_Symbol',sample_name]))
    
gene_values=pd.concat(gene_values,axis=1)
gene_ranking=gene_values.rank(ascending=True)    

#create output folder
if not os.path.exists(output_directory):
    os.makedirs(output_directory)
    

#For each motif make the plots
for motif_gene_filename in glob.glob(os.path.join(haystack_motifs_output_folder,'genes_lists')+'/*.bed'):

    current_motif_id=os.path.basename(motif_gene_filename).split('_')[0]
   
    info('Analyzing %s from:%s' %(current_motif_id, motif_gene_filename))
    

    #genes closeby the motif sites
    mapped_genes=list(pd.read_table(motif_gene_filename)['Symbol'].values)
    
    #target genes average activity
    ds_values=zscore_series(gene_ranking.ix[mapped_genes,:].mean() )
    
    if current_motif_id in motif_mapping.index:
        current_motif_name=motif_mapping.ix[current_motif_id].MOTIF_NAME

        for gene_name in set(map(str.upper,motif_mapping.ix[current_motif_id].GENES.split(','))):
            
           
            #specificity of the TF
            try:
                tf_values=zscore_series(gene_ranking.ix[gene_name.upper()])
            except:
                warn('The expression values of the gene %s  are not present. Skipping it.' % gene_name.upper())
                continue
            
            
            tf_value=tf_values[target_cell_type]
            ds_value=ds_values[target_cell_type]
                                    
            #correlation 
            ro=np.corrcoef(tf_values,ds_values)[0,1]
            info('Gene:%s TF z-score:%.2f Targets z-score:%.2f  Correlation:%.2f' %(gene_name,tf_value,ds_value,ro))
            
            if (tf_value>0 and np.abs(ro)>0.3) or plot_all:
            
                x_min=min(-4,tf_values.min()*1.1)
                x_max=max(4,tf_values.max()*1.1)
                y_min=min(-4,ds_values.min()*1.1)
                y_max=max(4,ds_values.max()*1.1)
                
                fig = plt.figure( figsize=(10, 10), dpi=80, facecolor='w', edgecolor='w')
                ax = fig.add_subplot(111)
                plt.grid()
                plt.plot([x_min,x_max],[0,0],'k')
                plt.plot([0,0],[y_min,y_max],'k')
                ax.scatter(tf_values,ds_values, s=100, facecolors='none', edgecolors='k',label='rest of cell-types')
                ax.hold(True)
                ax.plot(tf_values[target_cell_type],ds_values[target_cell_type],'*r',markersize=30,linestyle='None',label=target_cell_type)
                ax.legend(loc='center', bbox_to_anchor=(0.5, -0.1),ncol=3, fancybox=True, shadow=True,numpoints=1)
               
                ax.set_aspect('equal')
                plt.text(x_min*0.98,y_max*0.85,r'$\rho$=%.2f' % ro,fontsize=14)
                plt.xlim(x_min,x_max)
                plt.ylim(y_min,y_max)
    
                plt.xlabel('TF z-score',fontsize=16)
                plt.ylabel('Targets z-score',fontsize=16)
                plt.title('Motif: %s (%s) Gene: %s' % (current_motif_name ,current_motif_id ,gene_name),fontsize=17)
                plt.savefig(os.path.join(output_directory,'%s_motif_%s(%s)_gene_%s.pdf' % (target_cell_type,current_motif_name.replace('::','_') ,current_motif_id ,gene_name)))
                plt.close()
                
            
    else:
        warn('Sorry the motif %s is not mappable to gene' % current_motif_id)

info('All done! Ciao!')
sys.exit(0)

