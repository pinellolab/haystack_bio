from __future__ import division
import os
import sys
import subprocess as sb
import glob
import shutil
import multiprocessing

try:
    import cPickle as cp
except:
    import pickle as cp

import pandas as pd
import matplotlib as mpl

mpl.use('Agg')
import pylab as pl
import xml.etree.cElementTree as ET
from pybedtools import BedTool
from bioutilities import Genome_2bit
import re

# commmon functions haystack hotspots
import numpy as np
from scipy.stats import zscore
import urllib2

HAYSTACK_VERSION = "0.4.0"

import logging

logging.basicConfig(level=logging.DEBUG,
                    format='%(levelname)-5s @ %(asctime)s:\n\t %(message)s \n',
                    datefmt='%a, %d %b %Y %H:%M:%S',
                    stream=sys.stderr,
                    filemode="w", filename='example.log'
                    )

error = logging.critical
warn = logging.warning
debug = logging.debug
info = logging.info
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
logging.debug("start")


def check_library(library_name):
    try:
        return __import__(library_name)
    except:
        error('You need to install %s module to use haystack!' % library_name)
        sys.exit(1)


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


def check_program(binary_name, download_url=None):
    if not which(binary_name):
        error(
            'You need to install and have the command #####%s##### in your PATH variable to use CRISPResso!\n Please read the documentation!' % binary_name)
        if download_url:
            error('You can download it from here:%s' % download_url)
        sys.exit(1)


def check_file(filename):
    try:
        with open(filename):
            pass
    except IOError:
        raise Exception('I cannot open the file: ' + filename)


def quantile_normalization(A):
    AA = np.zeros_like(A)
    I = np.argsort(A, axis=0)
    AA[I, np.arange(A.shape[1])] = np.mean(A[I, np.arange(A.shape[1])], axis=1)[:, np.newaxis]

    return AA


def smooth(x, window_len=200):
    s = np.r_[x[window_len - 1:0:-1], x, x[-1:-window_len:-1]]
    w = np.hanning(window_len)
    y = np.convolve(w / w.sum(), s, mode='valid')
    return y[int(window_len / 2):-int(window_len / 2) + 1]


# write the IGV session file
def rem_base_path(path, base_path):
    return path.replace(os.path.join(base_path, ''), '')


def find_th_rpm(df_chip, th_rpm):
    return np.min(df_chip.apply(lambda x: np.percentile(x, th_rpm)))


def log2_transform(x):
    return np.log2(x + 1)


def angle_transform(x):
    return np.arcsin(np.sqrt(x) / 1000000.0)


def download_genome(name, output_directory=None):
    urlpath = "http://hgdownload.cse.ucsc.edu/goldenPath/%s/bigZips/%s.2bit" % (name, name)
    genome_url_origin = urllib2.urlopen(urlpath)

    genome_filename = os.path.join(output_directory, "%s.2bit" % name)

    print 'Downloading %s in %s...' % (urlpath, genome_filename)

    if os.path.exists(genome_filename):
        print 'File %s exists, skipping download' % genome_filename
    else:

        with open(genome_filename, 'wb') as genome_file_destination:
            shutil.copyfileobj(genome_url_origin, genome_file_destination)

        print 'Downloded %s in %s:' % (urlpath, genome_filename)

    g = Genome_2bit(genome_filename, verbose=True)

    chr_len_filename = os.path.join(output_directory, "%s_chr_lengths.txt" % name)
    if not os.path.exists(chr_len_filename):
        print 'Extracting chromosome lengths'
        g.write_chr_len(chr_len_filename)
        print 'Done!'
    else:
        print 'File %s exists, skipping generation' % chr_len_filename

    meme_bg_filename = os.path.join(output_directory, "%s_meme_bg" % name)
    if not os.path.exists(meme_bg_filename):
        print 'Calculating nucleotide frequencies....'
        g.write_meme_background(meme_bg_filename)
        print 'Done!'
    else:
        print 'File %s exists, skipping generation' % meme_bg_filename


# commmon functions haystack hotspots
import numpy as np
from scipy.stats import zscore
import urllib2


def quantile_normalization(A):
    AA = np.zeros_like(A)
    I = np.argsort(A, axis=0)
    AA[I, np.arange(A.shape[1])] = np.mean(A[I, np.arange(A.shape[1])], axis=1)[:, np.newaxis]

    return AA


def smooth(x, window_len=200):
    s = np.r_[x[window_len - 1:0:-1], x, x[-1:-window_len:-1]]
    w = np.hanning(window_len)
    y = np.convolve(w / w.sum(), s, mode='valid')
    return y[int(window_len / 2):-int(window_len / 2) + 1]


# write the IGV session file
def rem_base_path(path, base_path):
    return path.replace(os.path.join(base_path, ''), '')


def find_th_rpm(df_chip, th_rpm_param):
    return np.min(df_chip.apply(lambda x: np.percentile(x, th_rpm_param)))


def log2_transform(x):
    return np.log2(x + 1)


def angle_transform(x):
    return np.arcsin(np.sqrt(x) / 1000000.0)


def download_genome(name, output_directory=None):
    urlpath = "http://hgdownload.cse.ucsc.edu/goldenPath/%s/bigZips/%s.2bit" % (name, name)
    genome_url_origin = urllib2.urlopen(urlpath)

    genome_filename = os.path.join(output_directory, "%s.2bit" % name)

    print 'Downloading %s in %s...' % (urlpath, genome_filename)

    if os.path.exists(genome_filename):
        print 'File %s exists, skipping download' % genome_filename
    else:

        with open(genome_filename, 'wb') as genome_file_destination:
            shutil.copyfileobj(genome_url_origin, genome_file_destination)

        print 'Downloded %s in %s:' % (urlpath, genome_filename)

    g = Genome_2bit(genome_filename, verbose=True)

    chr_len_filename = os.path.join(output_directory, "%s_chr_lengths.txt" % name)
    if not os.path.exists(chr_len_filename):
        print 'Extracting chromosome lengths'
        g.write_chr_len(chr_len_filename)
        print 'Done!'
    else:
        print 'File %s exists, skipping generation' % chr_len_filename

    meme_bg_filename = os.path.join(output_directory, "%s_meme_bg" % name)
    if not os.path.exists(meme_bg_filename):
        print 'Calculating nucleotide frequencies....'
        g.write_meme_background(meme_bg_filename)
        print 'Done!'
    else:
        print 'File %s exists, skipping generation' % meme_bg_filename


def determine_path(folder):
    _ROOT = os.getcwd()
    return os.path.join(_ROOT, folder)


########

samples_filename = determine_path('test_data/samples_names.txt')
output_directory = determine_path('haystack_analysis/output')
motif_directory = determine_path('motif_databases')
annotation_directory = determine_path('gene_annotations')
genome_directory = determine_path('genomes')

samples_filename_or_bam_folder = samples_filename
genome_name = 'hg19'

# optional
name = ''
output_directory = output_directory
bin_size = 200
recompute_all = True
depleted = True
input_is_bigwig = False
disable_quantile_normalization = False
transformation = 'angle'
z_score_high = 1.5
z_score_low = 0.25
th_rpm = 99
meme_motifs_filename = ''  # os.path.join(MOTIF_DIR, 'JASPAR_CORE_2016_vertebrates.meme')
motif_mapping_filename = ''  # os.path.join(MOTIF_DIR, 'JASPAR_CORE_2016_vertebrates_mapped_to_gene_human_mouse.txt')
plot_all = True
use_X_Y = True
chrom_exclude = ''  # '_|chrM|chrX|chrY'
ext = 200
blacklist=True
max_regions_percentage=0.1
n_processes = multiprocessing.cpu_count()
temp_directory = ''  # os.path.join(_ROOT, 'tmp')
version = 'Version %s' % HAYSTACK_VERSION

"/mnt/hd2/Dropbox (Partners HealthCare)/PROJECTS/2017_07_HAYSTACK/test_data/samples_names.txt" hg19 --output_directory "/mnt/hd2/Dropbox (Partners HealthCare)/PROJECTS/2017_07_HAYSTACK/haystack_analysis/output_haystack" --bin_size 200

######################

if depleted:
    z_score_high = -z_score_high
    z_score_low = -z_score_low


if meme_motifs_filename:
    check_file(meme_motifs_filename)

if motif_mapping_filename:
    check_file(motif_mapping_filename)

# if not os.path.exists(temp_directory):
#    error('The folder specified with --temp_directory: %s does not exist!' % temp_directory)
#    sys.exit(1)

if input_is_bigwig:
    extension_to_check = '.bw'
    info('Input is set BigWig (.bw)')
else:
    extension_to_check = '.bam'
    info('Input is set compressed SAM (.bam)')

if name:
    directory_name = 'HAYSTACK_PIPELINE_RESULTS_on_%s' % name

else:
    directory_name = 'HAYSTACK_PIPELINE_RESULTS'

if output_directory:
    output_directory = os.path.join(output_directory, directory_name)
else:
    output_directory = directory_name

# check folder or sample filename

USE_GENE_EXPRESSION = True

if os.path.isfile(samples_filename_or_bam_folder):
    BAM_FOLDER = False
    bam_filenames = []
    gene_expression_filenames = []
    sample_names = []

    dir_path = os.path.dirname(os.path.realpath(samples_filename_or_bam_folder))

    with open(samples_filename_or_bam_folder) as infile:
        for line in infile:

            if not line.strip():
                continue

            if line.startswith('#'):  # skip optional header line or empty lines
                info('Skipping header/comment line:%s' % line)
                continue

            fields = line.strip().split("\t")
            n_fields = len(fields)

            if n_fields == 2:

                USE_GENE_EXPRESSION = False

                sample_names.append(fields[0])
                bam_filenames.append(fields[1])

            elif n_fields == 3:

                USE_GENE_EXPRESSION = USE_GENE_EXPRESSION and True

                sample_names.append(fields[0])
                bam_filenames.append(fields[1])
                gene_expression_filenames.append(fields[2])
            else:
                error('The samples file format is wrong!')

    bam_filenames = [os.path.join(dir_path, filename) for filename in bam_filenames]
    gene_expression_filenames = [os.path.join(dir_path, filename) for filename in gene_expression_filenames]

else:
    if os.path.exists(samples_filename_or_bam_folder):
        BAM_FOLDER = True
        USE_GENE_EXPRESSION = False
        bam_filenames = glob.glob(os.path.join(samples_filename_or_bam_folder, '*' + extension_to_check))

        if not bam_filenames:
            error('No bam/bigwig  files to analyze in %s. Exiting.' % samples_filename_or_bam_folder)
            sys.exit(1)

        sample_names = [os.path.basename(bam_filename).replace(extension_to_check, '') for bam_filename in
                        bam_filenames]
    else:
        error("The file or folder %s doesn't exist. Exiting." % samples_filename_or_bam_folder)
        sys.exit(1)

# check all the files before starting
info('Checking samples files location...')
for bam_filename in bam_filenames:
    check_file(bam_filename)

if USE_GENE_EXPRESSION:
    for gene_expression_filename in gene_expression_filenames:
        check_file(gene_expression_filename)

if not os.path.exists(output_directory):
    os.makedirs(output_directory)

# copy back the file used
if not BAM_FOLDER:
    shutil.copy2(samples_filename_or_bam_folder, output_directory)

# write hotspots conf files
sample_names_hotspots_filename = os.path.join(output_directory, 'sample_names_hotspots.txt')

with open(sample_names_hotspots_filename, 'w+') as outfile:
    for sample_name, bam_filename in zip(sample_names, bam_filenames):
        outfile.write('%s\t%s\n' % (sample_name, bam_filename))

# write tf activity  conf files
if USE_GENE_EXPRESSION:
    sample_names_tf_activity_filename = os.path.join(output_directory, 'sample_names_tf_activity.txt')

    with open(sample_names_tf_activity_filename, 'w+') as outfile:
        for sample_name, gene_expression_filename in zip(sample_names, gene_expression_filenames):
            outfile.write('%s\t%s\n' % (sample_name, gene_expression_filename))

    tf_activity_directory = os.path.join(output_directory, 'HAYSTACK_TFs_ACTIVITY_PLANES')

if name:
    directory_name = 'HAYSTACK_HOTSPOTS_on_%s' % name

else:
    directory_name = 'HAYSTACK_HOTSPOTS'

if output_directory:
    output_directory = os.path.join(output_directory, directory_name)
else:
    output_directory = directory_name

if not os.path.exists(output_directory):
    os.makedirs(output_directory)

tracks_directory = os.path.join(output_directory, 'TRACKS')
if not os.path.exists(tracks_directory):
    os.makedirs(tracks_directory)

intermediate_directory = os.path.join(output_directory, 'INTERMEDIATE')
if not os.path.exists(intermediate_directory):
    os.makedirs(intermediate_directory)


specific_regions_directory = os.path.join(output_directory, 'SPECIFIC_REGIONS')
if not os.path.exists(specific_regions_directory):
        os.makedirs(specific_regions_directory)

#############################

chr_len_filename = os.path.join(genome_directory, "%s_chr_lengths.txt" % genome_name)
check_file(chr_len_filename)

genome_sorted_bins_file = os.path.join(output_directory,
                                                '%s.%dbp.bins.sorted.bed' % (os.path.basename(genome_name),
                                                                                     bin_size))
bedgraph_iod_track_filename = os.path.join(tracks_directory,
                                           'VARIABILITY.bedgraph')
bw_iod_track_filename = os.path.join(tracks_directory,
                                     'VARIABILITY.bw')
bedgraph_hpr_filename = os.path.join(tracks_directory,
                                     'SELECTED_VARIABILITY_HOTSPOT.bedgraph')
bed_hpr_fileaname = os.path.join(output_directory,
                                 'SELECTED_VARIABILITY_HOTSPOT.bed')

########################################

def intialize_genome(genome_name):

    info('Initializing Genome:%s' % genome_name)
    genome_2bit = os.path.join(genome_directory, genome_name + '.2bit')

    if os.path.exists(genome_2bit):
        genome = Genome_2bit(genome_2bit)
    else:
        info("\nIt seems you don't have the required genome file.")

        download_genome(genome_name, genome_directory)
        if os.path.exists(genome_2bit):
            info('Genome correctly downloaded!')
            genome = Genome_2bit(genome_2bit)
        else:
            error('Sorry I cannot download the required file for you. Check your Internet connection.')
            sys.exit(1)

    return genome


def average_bigwig(bam_filename, binned_rpm_filename, bigwig_filename):
    cmd = 'bigWigAverageOverBed %s %s  /dev/stdout | sort -s -n -k 1,1 | cut -f5 > %s' % (
        bam_filename, genome_sorted_bins_file, binned_rpm_filename)
    sb.call(cmd, shell=True)
    shutil.copy2(bam_filename, bigwig_filename)


def create_tiled_genome():

    if chrom_exclude:
        chr_len_filtered_filename = os.path.join(genome_directory,
                                                 "%s_chr_lengths_filtered.txt" % genome_name)

        with open(chr_len_filtered_filename, 'wb') as f:
            f.writelines(line for line in open(chr_len_filename)
                         if not re.search(chrom_exclude, line.split()[0]))
    else:
        chr_len_filtered_filename = chr_len_filename

    tiled_genome = BedTool().\
        window_maker(g=chr_len_filtered_filename,
                                          w=bin_size). \
        sort()

    if blacklist:

        blacklist = os.path.join(genome_directory,
                                 'blacklist.bed')
        check_file(blacklist)

        tiled_genome.intersect(blacklist,
                               wa=True,
                               v=True,
                               output=genome_sorted_bins_file)
    else:
        tiled_genome.saveas(genome_sorted_bins_file)


def normalize_count(feature, scaling_factor):
    feature.name = str(int(feature.name) * scaling_factor)
    return feature


def get_scaling_factor(bam_filename):

    from pysam import AlignmentFile

    infile = AlignmentFile(bam_filename, "rb")
    numreads = infile.count(until_eof=True)
    scaling_factor = (1.0 / float(numreads)) * 1000000

    return scaling_factor


def filter_dedup_bam(bam_filename, bam_filtered_nodup_filename):
    bam_temp_filename = os.path.join(os.path.dirname(bam_filtered_nodup_filename),
                                     '%s.temp%s' % os.path.splitext(os.path.basename(bam_filtered_nodup_filename)))

    info('Removing  unmapped, mate unmapped, not primary alignment, low MAPQ reads, and reads failing qc')

    cmd = 'sambamba view -f bam -l 0 -t %d -F "not (unmapped or mate_is_unmapped or failed_quality_control or duplicate or secondary_alignment) and mapping_quality >= 30" "%s"  -o "%s"' % (
        n_processes, bam_filename, bam_temp_filename)
    sb.call(cmd, shell=True)

    info('Removing  optical duplicates')

    cmd = 'sambamba markdup  -l 5 -t %d --hash-table-size=17592186044416 --overflow-list-size=20000000 --io-buffer-size=256 "%s" "%s" ' % (
        n_processes, bam_temp_filename, bam_filtered_nodup_filename)
    sb.call(cmd, shell=True)

    try:
        os.remove(bam_temp_filename)
        os.remove(bam_temp_filename + '.bai')
    except:
        pass


def to_bedgraph(bam_filtered_nodup_filename, rpm_filename, binned_rpm_filename):
    info('Converting bam to bed and extending read length...')
    bed_extended = BedTool(bam_filtered_nodup_filename). \
        bam_to_bed(). \
        slop(r=ext,
             l=0,
             s=True,
             g=chr_len_filename)

    info('Computing Scaling Factor...')
    scaling_factor = get_scaling_factor(bam_filtered_nodup_filename)

    info('Computing coverage over bins...')
    info('Normalizing counts by scaling factor...')

    bedgraph = BedTool(genome_sorted_bins_file). \
        intersect(bed_extended, c=True). \
        each(normalize_count, scaling_factor). \
        saveas(rpm_filename)

    info('Bedgraph saved...')
    info('Making constant binned (%dbp) rpm values file' % bin_size)
    bedgraph.to_dataframe()['name'].to_csv(binned_rpm_filename,
                                           index=False)


def to_bigwig(bigwig_filename, rpm_filename):
    if which('bedGraphToBigWig'):
        if not os.path.exists(bigwig_filename) or recompute_all:
            cmd = 'bedGraphToBigWig "%s" "%s" "%s"' % (rpm_filename,
                                                       chr_len_filename,
                                                       bigwig_filename)
            sb.call(cmd, shell=True)

            try:
                os.remove(rpm_filename)

            except:
                pass

    else:
        info(
            'Sorry I cannot create the bigwig file.\nPlease download and install bedGraphToBigWig from here: http://hgdownload.cse.ucsc.edu/admin/exe/ and add to your PATH')


def to_rpm_tracks(sample_names, bam_filenames, skipfilter=False):

    for base_name, bam_filename in zip(sample_names, bam_filenames):

        info('Processing:%s' % bam_filename)

        rpm_filename = os.path.join(tracks_directory,
                                    '%s.bedgraph' % base_name)
        binned_rpm_filename = os.path.join(intermediate_directory,
                                           '%s.%dbp.rpm' % (base_name, bin_size))
        bigwig_filename = os.path.join(tracks_directory,
                                       '%s.bw' % base_name)
        bam_filtered_nodup_filename = os.path.join(dir_path,
                                                   '%s.filtered.nodup%s' % (os.path.splitext(
                                                       os.path.basename(bam_filename))))

        if not os.path.exists(binned_rpm_filename) or recompute_all:

            if input_is_bigwig and which('bigWigAverageOverBed'):

                average_bigwig(bam_filename, binned_rpm_filename, bigwig_filename)

            else:

                if not skipfilter:
                    info('Filtering and deduping BAM file...')

                    filter_dedup_bam(bam_filename, bam_filtered_nodup_filename)

                info('Building BedGraph RPM track...')

                to_bedgraph(bam_filtered_nodup_filename, rpm_filename, binned_rpm_filename)

                info('Converting BedGraph to BigWig')

                to_bigwig(bigwig_filename, rpm_filename)


def load_rpm_tracks(binned_rpm_filenames):
    # load all the tracks
    info('Loading the processed tracks')
    df_chip = {}
    for binned_rpm_filename in binned_rpm_filenames:
        col_name = os.path.basename(binned_rpm_filename).replace('.rpm', '')
        df_chip[col_name] = pd.read_csv(binned_rpm_filename,
                                        squeeze=True,
                                        header=None)
        info('Loading:%s' % col_name)

    df_chip = pd.DataFrame(df_chip)

    coordinates_bin = pd.read_csv(genome_sorted_bins_file,
                                  names=['chr_id', 'bpstart', 'bpend'],
                                  sep='\t',
                                  header=None, usecols=[0, 1, 2])

    return df_chip, coordinates_bin


def df_chip_to_bigwig(df_chip, coordinates_bin, normalized=False):

    coord_quantile = coordinates_bin.copy()

    if which('bedGraphToBigWig'):
        # write quantile normalized tracks
        for col in df_chip:

            if normalized:
                normalized_output_filename = os.path.join(tracks_directory,
                                                          '%s_quantile_normalized.bedgraph' % os.path.basename(
                                                              col))
            else:
                normalized_output_filename = os.path.join(tracks_directory,
                                                          '%s.bedgraph' % os.path.basename(col))


            normalized_output_filename_bigwig = normalized_output_filename.replace('.bedgraph', '.bw')

            if not os.path.exists(normalized_output_filename_bigwig) or recompute_all:
                info('Writing binned track: %s' % normalized_output_filename_bigwig)
                coord_quantile['rpm_normalized'] = df_chip.loc[:, col]
                coord_quantile.dropna().to_csv(normalized_output_filename,
                                               sep='\t',
                                               header=False,
                                               index=False)

                cmd = 'bedGraphToBigWig "%s" "%s" "%s"' % (normalized_output_filename,
                                                     chr_len_filename,
                                                     normalized_output_filename_bigwig)
                sb.call(cmd, shell=True)
                try:
                    os.remove(normalized_output_filename)
                except:
                    pass
    else:
        info(
            'Sorry I cannot creat the bigwig file.\nPlease download and install bedGraphToBigWig from here: http://hgdownload.cse.ucsc.edu/admin/exe/ and add to your PATH')

# load coordinates of bins

def find_hpr_coordinates(df_chip, coordinates_bin):

    # th_rpm=np.min(df_chip.apply(lambda x: np.percentile(x,th_rpm)))
    th_rpm_est = find_th_rpm(df_chip, th_rpm)
    info('Estimated th_rpm:%s' % th_rpm_est)

    df_chip_not_empty = df_chip.loc[(df_chip > th_rpm_est).any(1), :]

    if transformation == 'log2':
        df_chip_not_empty = df_chip_not_empty.applymap(log2_transform)
        info('Using log2 transformation')

    elif transformation == 'angle':
        df_chip_not_empty = df_chip_not_empty.applymap(angle_transform)
        info('Using angle transformation')

    else:
        info('Using no transformation')

    iod_values = df_chip_not_empty.var(1) / df_chip_not_empty.mean(1)

    ####calculate the inflation point a la superenhancers
    scores = iod_values
    min_s = np.min(scores)
    max_s = np.max(scores)

    N_POINTS = len(scores)
    x = np.linspace(0, 1, N_POINTS)
    y = sorted((scores - min_s) / (max_s - min_s))
    m = smooth((np.diff(y) / np.diff(x)), 50)
    m = m - 1
    m[m <= 0] = np.inf
    m[:int(len(m) * (1 - max_regions_percentage))] = np.inf
    idx_th = np.argmin(m) + 1



    ###

    ###plot selection
    pl.figure()
    pl.title('Selection of the HPRs')
    pl.plot(x, y, 'r', lw=3)
    pl.plot(x[idx_th], y[idx_th], '*', markersize=20)
    x_ext = np.linspace(-0.1, 1.2, N_POINTS)
    y_line = (m[idx_th] + 1.0) * (x_ext - x[idx_th]) + y[idx_th];
    pl.plot(x_ext, y_line, '--k', lw=3)
    pl.xlim(0, 1.1)
    pl.ylim(0, 1)
    pl.xlabel('Fraction of bins')
    pl.ylabel('Score normalized')
    pl.savefig(os.path.join(output_directory, 'SELECTION_OF_VARIABILITY_HOTSPOT.pdf'))
    pl.close()

    # print idx_th,
    th_iod = sorted(iod_values)[idx_th]
    # print th_iod

    hpr_idxs = iod_values > th_iod

    hpr_iod_scores = iod_values[hpr_idxs]
    # print len(iod_values),len(hpr_idxs),sum(hpr_idxs), sum(hpr_idxs)/float(len(hpr_idxs)),

    info('Selected %f%% regions (%d)' % (sum(hpr_idxs) / float(len(hpr_idxs)) * 100, sum(hpr_idxs)))
    coordinates_bin['iod'] = iod_values
    # we remove the regions "without" signal in any of the cell types
    coordinates_bin.dropna(inplace=True)

    df_chip_hpr_zscore = df_chip_not_empty.loc[hpr_idxs, :].apply(zscore, axis=1)

    return hpr_idxs, coordinates_bin, df_chip_hpr_zscore, hpr_iod_scores

def hpr_to_bigwig(coordinates_bin):

    # create a track for IGV

    if not os.path.exists(bw_iod_track_filename) or recompute_all:

        info('Generating variability track in bigwig format in:%s' % bw_iod_track_filename)

        coordinates_bin.to_csv(bedgraph_iod_track_filename,
                               sep='\t',
                               header=False,
                               index=False)
        sb.call('bedGraphToBigWig "%s" "%s" "%s"' % (bedgraph_iod_track_filename,
                                                     chr_len_filename,
                                                     bw_iod_track_filename),
                shell=True)
        try:
            os.remove(bedgraph_iod_track_filename)
        except:
            pass

def hpr_to_bedgraph(hpr_idxs, coordinates_bin):

    to_write = coordinates_bin.loc[hpr_idxs[hpr_idxs].index]
    to_write.dropna(inplace=True)
    to_write['bpstart'] = to_write['bpstart'].astype(int)
    to_write['bpend'] = to_write['bpend'].astype(int)

    to_write.to_csv(bedgraph_hpr_filename,
                    sep='\t',
                    header=False,
                    index=False)


    if not os.path.exists(bed_hpr_fileaname) or recompute_all:
        info('Writing the HPRs in: "%s"' % bed_hpr_fileaname)
        sb.call('sort -k1,1 -k2,2n "%s" | bedtools merge -i stdin >  "%s"' % (bedgraph_hpr_filename,
                                                                              bed_hpr_fileaname),
                shell=True)

def write_specific_regions(coordinates_bin, df_chip_hpr_zscore):

    # write target
    info('Writing Specific Regions for each cell line...')
    coord_zscore = coordinates_bin.copy()
    for col in df_chip_hpr_zscore:
        regions_specific_filename = 'Regions_specific_for_%s_z_%.2f.bedgraph' % (
            os.path.basename(col).replace('.rpm', ''), z_score_high)
        specific_output_filename = os.path.join(specific_regions_directory,
                                                regions_specific_filename)
        specific_output_bed_filename = specific_output_filename.replace('.bedgraph', '.bed')

        if not os.path.exists(specific_output_bed_filename) or recompute_all:
            if depleted:
                coord_zscore['z-score'] = df_chip_hpr_zscore.loc[df_chip_hpr_zscore.loc[:, col] < z_score_high, col]
            else:
                coord_zscore['z-score'] = df_chip_hpr_zscore.loc[df_chip_hpr_zscore.loc[:, col] > z_score_high, col]
            coord_zscore.dropna().to_csv(specific_output_filename,
                                         sep='\t',
                                         header=False,
                                         index=False)

            info('Writing:%s' % specific_output_bed_filename)
            sb.call('sort -k1,1 -k2,2n "%s" | bedtools merge -i stdin >  "%s"' % (specific_output_filename,
                                                                                  specific_output_bed_filename),
                    shell=True)
    # write background
    info('Writing Background Regions for each cell line...')
    coord_zscore = coordinates_bin.copy()
    for col in df_chip_hpr_zscore:

        bg_output_filename = os.path.join(specific_regions_directory,
                                          'Background_for_%s_z_%.2f.bedgraph' % (
                                              os.path.basename(col).replace('.rpm', ''), z_score_low))
        bg_output_bed_filename = bg_output_filename.replace('.bedgraph', '.bed')

        if not os.path.exists(bg_output_bed_filename) or recompute_all:

            if depleted:
                coord_zscore['z-score'] = df_chip_hpr_zscore.loc[df_chip_hpr_zscore.loc[:, col] > z_score_low, col]
            else:
                coord_zscore['z-score'] = df_chip_hpr_zscore.loc[df_chip_hpr_zscore.loc[:, col] < z_score_low, col]

            coord_zscore.dropna().to_csv(bg_output_filename,
                                         sep='\t',
                                         header=False,
                                         index=False)

            info('Writing:%s' % bg_output_bed_filename)
            sb.call(
                'sort -k1,1 -k2,2n -i "%s" | bedtools merge -i stdin >  "%s"' % (bg_output_filename,
                                                                                 bg_output_bed_filename),
                shell=True)

def create_igv_track_file(hpr_iod_scores):

    igv_session_filename = os.path.join(output_directory, 'OPEN_ME_WITH_IGV.xml')
    info('Creating an IGV session file (.xml) in: %s' % igv_session_filename)

    session = ET.Element("Session")
    session.set("genome", genome_name)
    session.set("hasGeneTrack", "true")
    session.set("version", "7")
    resources = ET.SubElement(session, "Resources")
    panel = ET.SubElement(session, "Panel")

    resource_items = []
    track_items = []

    min_h = np.mean(hpr_iod_scores) - 2 * np.std(hpr_iod_scores)
    max_h = np.mean(hpr_iod_scores) + 2 * np.std(hpr_iod_scores)
    mid_h = np.mean(hpr_iod_scores)
    # write the tracks
    for sample_name in sample_names:
        if disable_quantile_normalization:
            track_full_path = os.path.join(output_directory, 'TRACKS', '%s.%dbp.bw' % (sample_name, bin_size))
        else:
            track_full_path = os.path.join(output_directory, 'TRACKS',
                                           '%s.%dbp_quantile_normalized.bw' % (sample_name, bin_size))

        track_filename = rem_base_path(track_full_path, output_directory)

        if os.path.exists(track_full_path):
            resource_items.append(ET.SubElement(resources, "Resource"))
            resource_items[-1].set("path", track_filename)
            track_items.append(ET.SubElement(panel, "Track"))
            track_items[-1].set('color', "0,0,178")
            track_items[-1].set('id', track_filename)
            track_items[-1].set("name", sample_name)

    resource_items.append(ET.SubElement(resources, "Resource"))
    resource_items[-1].set("path", rem_base_path(bw_iod_track_filename, output_directory))

    track_items.append(ET.SubElement(panel, "Track"))
    track_items[-1].set('color', "178,0,0")
    track_items[-1].set('id', rem_base_path(bw_iod_track_filename, output_directory))
    track_items[-1].set('renderer', "HEATMAP")
    track_items[-1].set("colorScale",
                        "ContinuousColorScale;%e;%e;%e;%e;0,153,255;255,255,51;204,0,0" % (
                        mid_h, min_h, mid_h, max_h))
    track_items[-1].set("name", 'VARIABILITY')

    resource_items.append(ET.SubElement(resources, "Resource"))
    resource_items[-1].set("path", rem_base_path(bed_hpr_fileaname, output_directory))
    track_items.append(ET.SubElement(panel, "Track"))
    track_items[-1].set('color', "178,0,0")
    track_items[-1].set('id', rem_base_path(bed_hpr_fileaname, output_directory))
    track_items[-1].set('renderer', "HEATMAP")
    track_items[-1].set("colorScale",
                        "ContinuousColorScale;%e;%e;%e;%e;0,153,255;255,255,51;204,0,0" % (
                        mid_h, min_h, mid_h, max_h))
    track_items[-1].set("name", 'HOTSPOTS')

    for sample_name in sample_names:
        track_full_path = \
            glob.glob(os.path.join(output_directory, 'SPECIFIC_REGIONS',
                                   'Regions_specific_for_%s*.bedgraph' % sample_name))[0]
        specific_track_filename = rem_base_path(track_full_path, output_directory)
        if os.path.exists(track_full_path):
            resource_items.append(ET.SubElement(resources, "Resource"))
            resource_items[-1].set("path", specific_track_filename)

            track_items.append(ET.SubElement(panel, "Track"))
            track_items[-1].set('color', "178,0,0")
            track_items[-1].set('id', specific_track_filename)
            track_items[-1].set('renderer', "HEATMAP")
            track_items[-1].set("colorScale",
                                "ContinuousColorScale;%e;%e;%e;%e;0,153,255;255,255,51;204,0,0" % (
                                    mid_h, min_h, mid_h, max_h))
            track_items[-1].set("name", 'REGION SPECIFIC FOR %s' % sample_name)

    tree = ET.ElementTree(session)
    tree.write(igv_session_filename, xml_declaration=True)

# step 1
genome = intialize_genome(genome_name)

# step 2
info('Creating bins of %dbp in %s' % (bin_size, genome_sorted_bins_file))
if not os.path.exists(genome_sorted_bins_file) or recompute_all:
    create_tiled_genome(genome_sorted_bins_file)

# step 3
info('Convert files to genome-wide rpm tracks')
to_rpm_tracks(sample_names, bam_filenames, skipfilter=False)


# step 4
info('Normalize rpm tracks')
binned_rpm_filenames = glob.glob(os.path.join(intermediate_directory, '*.rpm'))
df_chip, coordinates_bin =load_rpm_tracks(binned_rpm_filenames)

if disable_quantile_normalization:
    info('Skipping quantile normalization...')
    df_chip_to_bigwig(df_chip, coordinates_bin)

else:
    info('Normalizing the data...')
    df_chip = pd.DataFrame(quantile_normalization(df_chip.values),
                               columns=df_chip.columns,
                               index=df_chip.index)
    df_chip_to_bigwig(df_chip, coordinates_bin, normalized=True)

# step 5
info('Determine HP regions')
hpr_idxs, coordinates_bin, df_chip_hpr_zscore, hpr_iod_scores = find_hpr_coordinates(df_chip, coordinates_bin)
info('Save files')
hpr_to_bigwig(coordinates_bin)
hpr_to_bedgraph(hpr_idxs, coordinates_bin)
write_specific_regions(coordinates_bin, df_chip_hpr_zscore)
create_igv_track_file(hpr_iod_scores)


info('All done! Ciao!')
sys.exit(0)

#import pyBigWig

#  bw1 = pyBigWig.open("/mnt/hd2/test_data/OUTPUT5/HAYSTACK_HOTSPOTS/TRACKS/K562.200bp_quantile_normalized.bw")
#  bw2 = pyBigWig.open("/mnt/hd2/test_data/HAYSTACK_HOTSPOTS/TRACKS/K562.200bp_quantile_normalized.bw")
#  bw1.header()
#  bw2.header()
# bw1.values("chr1", 100000, 100003)# NOT COMPLETE
# bw2.values("chr1", 100000, 100003)