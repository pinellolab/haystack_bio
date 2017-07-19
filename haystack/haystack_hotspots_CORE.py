from __future__ import division
import os
import sys
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
# commmon functions
from haystack_common import determine_path, which, check_file

from pybedtools import BedTool
import multiprocessing

n_processes = multiprocessing.cpu_count()


HAYSTACK_VERSION="0.4.0"


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


def get_args():
    # mandatory
    parser = argparse.ArgumentParser(description='HAYSTACK Parameters')
    parser.add_argument('samples_filename', type=str,
                        help='A tab delimited file with in each row (1) a sample name, (2) the path to the corresponding bam or bigwig filename')
    parser.add_argument('genome_name', type=str, help='Genome assembly to use from UCSC (for example hg19, mm9, etc.)')

    # optional
    parser.add_argument('--bin_size', type=int, help='bin size to use(default: 500bp)', default=500)
    parser.add_argument('--disable_quantile_normalization', help='Disable quantile normalization (default: False)',
                        action='store_true')
    parser.add_argument('--th_rpm', type=float,
                        help='Percentile on the signal intensity to consider for the hotspots (default: 99)',
                        default=99)
    parser.add_argument('--transformation', type=str,
                        help='Variance stabilizing transformation among: none, log2, angle (default: angle)',
                        default='angle', choices=['angle', 'log2', 'none'])
    parser.add_argument('--recompute_all', help='Ignore any file previously precalculated', action='store_true')
    parser.add_argument('--z_score_high', type=float, help='z-score value to select the specific regions(default: 1.5)',
                        default=1.5)
    parser.add_argument('--z_score_low', type=float,
                        help='z-score value to select the not specific regions(default: 0.25)', default=0.25)
    parser.add_argument('--name', help='Define a custom output filename for the report', default='')
    parser.add_argument('--output_directory', type=str, help='Output directory (default: current directory)',
                        default='')
    parser.add_argument('--blacklist', help='Exclude blacklisted regions.', action='store_true')
    parser.add_argument('--chrom_exclude', help='Exclude chromosomes. For example (_|chrM|chrX|chrY).', default='chrX|chrY')
    parser.add_argument('--read_ext', type=int, help='Read extension', default='200')
    parser.add_argument('--max_regions_percentage', type=float,
                        help='Upper bound on the %% of the regions selected  (deafult: 0.1, 0.0=0%% 1.0=100%%)',
                        default=0.1)
    parser.add_argument('--depleted',
                        help='Look for cell type specific regions with depletion of signal instead of enrichment',
                        action='store_true')
    parser.add_argument('--input_is_bigwig',
                        help='Use the bigwig format instead of the bam format for the input. Note: The files must have extension .bw',
                        action='store_true')
    parser.add_argument('--version', help='Print version and exit.', action='version',
                        version='Version %s' % HAYSTACK_VERSION)
    return parser

def check_required_packages():

    if which('samtools') is None:
        error(
            'Haystack requires samtools. Please install using bioconda')
        sys.exit(1)

    if which('bedtools') is None:
        error('Haystack requires bedtools. Please install using bioconda')
        sys.exit(1)

    if which('bedGraphToBigWig') is None:
        info(
            'To generate the bigwig files Haystack requires bedGraphToBigWig. Please install using bioconda')
        sys.exit(1)

    if which('sambamba') is None:
        info(
            'Haystack requires sambamba. Please install using bioconda')
        sys.exit(1)

    if which('bigWigAverageOverBed') is None:
        info(
            'Haystack requires bigWigAverageOverBed. Please install using bioconda')
        sys.exit(1)


def get_data_filepaths(samples_filename):
    # check folder or sample filename
    if os.path.isfile(samples_filename):
        data_filenames = []
        sample_names = []
        with open(samples_filename) as infile:
            for line in infile:

                if not line.strip():
                    continue

                if line.startswith('#'):  # skip optional header line or empty lines
                    info('Skipping header/comment line:%s' % line)
                    continue

                fields = line.strip().split("\t")
                n_fields = len(fields)

                if n_fields == 2:
                    sample_names.append(fields[0])
                    data_filenames.append(fields[1])
                else:
                    error('The samples file format is wrong!')
                    sys.exit(1)

    dir_path = os.path.dirname(os.path.realpath(samples_filename))
    data_filenames = [os.path.join(dir_path, filename) for filename in data_filenames]
    # check all the files before starting
    info('Checking samples files location...')
    for data_filename in data_filenames:
        check_file(data_filename)

    return sample_names, data_filenames

def initialize_genome(genome_directory, genome_name):

    info('Initializing Genome:%s' % genome_name)
    genome_2bit = os.path.join(genome_directory, genome_name + '.2bit')

    if os.path.exists(genome_2bit):
        Genome_2bit(genome_2bit)
    else:
        info("\nIt seems you don't have the required genome file.")

        download_genome(genome_name, genome_directory)
        if os.path.exists(genome_2bit):
            info('Genome correctly downloaded!')
            Genome_2bit(genome_2bit)
        else:
            error('Sorry I cannot download the required file for you. Check your Internet connection.')
            sys.exit(1)

def create_tiled_genome(genome_directory, genome_name, genome_sorted_bins_file, chr_len_filename, bin_size, chrom_exclude, blacklist):

    from re import search

    info('Creating bins of %dbp in %s' % (bin_size, genome_sorted_bins_file))

    if chrom_exclude:
        chr_len_filtered_filename = os.path.join(genome_directory,
                                                 "%s_chr_lengths_filtered.txt" % genome_name)

        with open(chr_len_filtered_filename, 'wb') as f:
            f.writelines(line for line in open(chr_len_filename)
                         if not search(chrom_exclude, line.split()[0]))
    else:
        chr_len_filtered_filename = chr_len_filename

    tiled_genome = BedTool(). \
        window_maker(g=chr_len_filtered_filename,
                     w=bin_size).sort()

    if blacklist:

        blacklist_filepath = os.path.join(genome_directory,
                                          'blacklist.bed')
        check_file(blacklist_filepath)

        tiled_genome.intersect(blacklist_filepath,
                               wa=True,
                               v=True,
                               output=genome_sorted_bins_file)
    else:
        tiled_genome.saveas(genome_sorted_bins_file)


def average_bigwigs(data_filenames, binned_rpm_filenames, bigwig_filenames, genome_sorted_bins_file):

    for data_filename, binned_rpm_filename, bigwig_filename in zip(data_filenames, binned_rpm_filenames, bigwig_filenames):

        if not os.path.exists(bigwig_filename) or recompute_all:

            info('Processing:%s' % data_filename)

            cmd = 'bigWigAverageOverBed %s %s  /dev/stdout | sort -s -n -k 1,1 | cut -f5 > %s' % (
                data_filename, genome_sorted_bins_file, binned_rpm_filename)
            sb.call(cmd, shell=True)
            shutil.copy2(data_filename, bigwig_filename)


def to_bedgraph(bam_filenames, rpm_filenames, binned_rpm_filenames, chr_len_filename, genome_sorted_bins_file, read_ext):

    for bam_filename, rpm_filename, binned_rpm_filename in zip(bam_filenames, rpm_filenames, binned_rpm_filenames):

        if not os.path.exists(rpm_filename) or recompute_all:

            info('Processing:%s' % bam_filename)

            info('Computing Scaling Factor...')
            scaling_factor = get_scaling_factor(bam_filename)
            info('Scaling Factor: %e' % scaling_factor)

            info('Converting bam to bed and extending read length...')
            bed_coveraged_extended = BedTool(bam_filename). \
                bam_to_bed(). \
                slop(r=read_ext,
                     l=0,
                     s=True,
                     g=chr_len_filename). \
                genome_coverage(bg=True,
                                scale=scaling_factor,
                                g=chr_len_filename).sort()

            info('Computing coverage over bins...')
            info('Normalizing counts by scaling factor...')

            bedgraph = BedTool(genome_sorted_bins_file). \
                map(b=bed_coveraged_extended,
                    c=4,
                    o='mean',
                    null=0.0). \
                saveas(rpm_filename)

            # bedgraph = BedTool(genome_sorted_bins_file). \
            #     intersect(bed_extended, c=True). \
            #     each(normalize_count, scaling_factor). \
            #     saveas(rpm_filename)

            info('Bedgraph saved...')
            info('Making constant binned (%dbp) rpm values file' % bin_size)
            bedgraph.to_dataframe()['name'].to_csv(binned_rpm_filename,
                                                   index=False)

# def normalize_count(feature, scaling_factor):
#     feature.name = str(int(feature.name) * scaling_factor)
#     return feature

def get_scaling_factor(bam_filename):

    from pysam import AlignmentFile

    infile = AlignmentFile(bam_filename, "rb")
    numreads = infile.count(until_eof=True)
    scaling_factor = (1.0 / float(numreads)) * 1000000
    return scaling_factor


def to_bigwig(bigwig_filenames, rpm_filenames, chr_len_filename):

    for bigwig_filename, rpm_filename in zip(bigwig_filenames, rpm_filenames):

        if not os.path.exists(bigwig_filename) or recompute_all:

            cmd = 'bedGraphToBigWig "%s" "%s" "%s"' % (rpm_filename,
                                                       chr_len_filename,
                                                       bigwig_filename)
            sb.call(cmd, shell=True)

        # try:
        #     os.remove(rpm_filename)
        #
        # except:
        #     pass



# convert bam files to genome-wide rpm tracks

def filter_dedup_bams(bam_filenames, bam_filtered_nodup_filenames):

    for  bam_filename, bam_filtered_nodup_filename in zip(bam_filenames, bam_filtered_nodup_filenames):


        if not os.path.exists(bam_filtered_nodup_filename) or recompute_all:


            info('Processing:%s' % bam_filename)

            bam_temp_filename = os.path.join(os.path.dirname(bam_filtered_nodup_filename),
                                             '%s.temp%s' % os.path.splitext(os.path.basename(bam_filtered_nodup_filename)))

            info('Removing  unmapped, mate unmapped, not primary alignment, low MAPQ reads, and reads failing qc')

            # cmd = 'sambamba view -f bam -l 0 -t %d -F "not (unmapped or mate_is_unmapped or failed_quality_control or duplicate or secondary_alignment) and mapping_quality >= 30" "%s"  -o "%s"' % (
            #     n_processes, bam_filename, bam_temp_filename)

            cmd = 'sambamba view -f bam -l 0 -t %d -F "not failed_quality_control" "%s"  -o "%s"' % (n_processes,
                                                                                                     bam_filename,
                                                                                                     bam_temp_filename)

            sb.call(cmd, shell=True)

            info('Removing  optical duplicates')

            cmd = 'sambamba markdup  -l 5 -t %d --hash-table-size=17592186044416 --overflow-list-size=20000000 --io-buffer-size=256 "%s" "%s" ' % (n_processes,
                                                                                                                                                   bam_temp_filename,
                                                                                                                                                   bam_filtered_nodup_filename)
            sb.call(cmd, shell=True)

            try:
                os.remove(bam_temp_filename)
                os.remove(bam_temp_filename + '.bai')
            except:
                pass



def load_rpm_tracks(col_names, binned_rpm_filenames):
    # load all the tracks
    info('Loading the processed tracks')
    df_chip = {}
    for col_name, binned_rpm_filename in zip(col_names, binned_rpm_filenames):
        df_chip[col_name] = pd.read_csv(binned_rpm_filename,
                                        squeeze=True,
                                        header=None)
        info('Loading %s from file %s' % (col_name, binned_rpm_filename))

    df_chip = pd.DataFrame(df_chip)

    return df_chip


def df_chip_to_bigwig(df_chip, coordinates_bin, col_names, bedgraph_track_filenames, bigwig_track_filenames):

    coord_quantile = coordinates_bin.copy()

    # write quantile normalized tracks
    for col, bedgraph_track_filename, bigwig_track_filename in zip(col_names, bedgraph_track_filenames, bigwig_track_filenames):

        if not os.path.exists(bigwig_track_filename) or recompute_all:

            info('Writing binned track: %s' % bigwig_track_filename)
            coord_quantile['rpm_normalized'] = df_chip.loc[:, col]
            coord_quantile.dropna().to_csv(bigwig_track_filename,
                                           sep='\t',
                                           header=False,
                                           index=False)

            cmd = 'bedGraphToBigWig "%s" "%s" "%s"' % (bedgraph_track_filename,
                                                       chr_len_filename,
                                                       bigwig_track_filename)
            sb.call(cmd, shell=True)
            # try:
            #     os.remove(normalized_output_filename)
            # except:
            #     pass


def main(input_args=None):


    print '\n[H A Y S T A C K   H O T S P O T]'
    print('\n-SELECTION OF VARIABLE REGIONS- [Luca Pinello - lpinello@jimmy.harvard.edu]\n')
    print 'Version %s\n' % HAYSTACK_VERSION


    #step 1

    check_required_packages()

    # step 2

    parser = get_args()

    # input_args=['/mnt/hd2/test_data/samples_names.txt',
    #             'hg19',
    #             '--output_directory',
    #             '/mnt/hd2/test_data/OUTPUT5',
    #             '--bin_size',
    #             '200',
    #             '--chrom_exclude',
    #             '']
    #args = parser.parse_args(input_args)

    args = parser.parse_args()
    info(vars(args))


    samples_filename = args.samples_filename
    genome_name = args.genome_name
    # optional
    name = args.name
    output_directory = args.output_directory
    disable_quantile_normalization = args.disable_quantile_normalization
    bin_size = args.bin_size
    recompute_all = args.recompute_all
    depleted = args.depleted
    input_is_bigwig = args.input_is_bigwig
    transformation = args.transformation
    z_score_high = args.z_score_high
    z_score_low = args.z_score_low
    th_rpm = args.th_rpm
    chrom_exclude = args.chrom_exclude  # '_|chrM|chrX|chrY'
    blacklist = args.blacklist
    max_regions_percentage = args.max_regions_percentage
    read_ext = args.read_ext

    if depleted:
        z_score_high = -z_score_high
        z_score_low = -z_score_low



    # step 3: create directories

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


    filtered_bam_directory = os.path.join(output_directory, 'FILTERED_BAMS')
    if not os.path.exists(filtered_bam_directory):
        os.makedirs(filtered_bam_directory)

    tracks_directory = os.path.join(output_directory, 'TRACKS')
    if not os.path.exists(tracks_directory):
        os.makedirs(tracks_directory)

    intermediate_directory = os.path.join(output_directory, 'INTERMEDIATE')
    if not os.path.exists(intermediate_directory):
        os.makedirs(intermediate_directory)

    specific_regions_directory = os.path.join(output_directory, 'SPECIFIC_REGIONS')
    if not os.path.exists(specific_regions_directory):
        os.makedirs(specific_regions_directory)

    # step 4: set pathnames


    genome_directory = determine_path('genomes')

    sample_names, data_filenames = get_data_filepaths(samples_filename)

    chr_len_filename = os.path.join(genome_directory, "%s_chr_lengths.txt" % genome_name)

    genome_sorted_bins_file = os.path.join(output_directory, '%s.%dbp.bins.sorted.bed'
                                           % (os.path.basename(genome_name), bin_size))

    bam_filtered_nodup_filenames = [os.path.join(
        filtered_bam_directory,
        '%s.filtered.nodup%s' % (os.path.splitext(os.path.basename(data_filename))))
        for data_filename in data_filenames]

    col_names = '%s.%dbp'% (sample_names, bin_size)

    binned_rpm_filenames = os.path.join(intermediate_directory,
                                       '%s.%dbp.rpm' % (sample_names, bin_size))

    bigwig_filenames = os.path.join(tracks_directory,
                                   '%s.bw' % sample_names)

    rpm_filenames = os.path.join(tracks_directory,
                                '%s.bedgraph' % sample_names)



    bedgraph_track_filenames = os.path.join(tracks_directory,'%s.bedgraph' % col_names)

    bigwig_track_filenames = bedgraph_track_filenames.replace('.bedgraph', '.bw')


    bedgraph_track_normalized_filenames = os.path.join(tracks_directory, '%s_quantile_normalized.bedgraph' % col_names)

    bigwig_track_normalized_filenames = bedgraph_track_normalized_filenames.replace('.bedgraph', '.bw')


    bedgraph_iod_track_filename = os.path.join(tracks_directory,
                                               'VARIABILITY.bedgraph')
    bw_iod_track_filename = os.path.join(tracks_directory,
                                         'VARIABILITY.bw')
    bedgraph_hpr_filename = os.path.join(tracks_directory,
                                         'SELECTED_VARIABILITY_HOTSPOT.bedgraph')
    bed_hpr_fileaname = os.path.join(output_directory,
                                     'SELECTED_VARIABILITY_HOTSPOT.bed')







    # step 5
    initialize_genome(genome_directory, genome_name)
    check_file(chr_len_filename)


    # step 6

    if not os.path.exists(genome_sorted_bins_file) or recompute_all:
        create_tiled_genome(genome_directory,
                            genome_name,
                            genome_sorted_bins_file,
                            chr_len_filename,
                            bin_size,
                            chrom_exclude,
                            blacklist)


    # step 7

    info('Convert files to genome-wide rpm tracks')

    if input_is_bigwig:

        average_bigwigs(data_filenames,
                       binned_rpm_filenames,
                       bigwig_filenames,
                       genome_sorted_bins_file)

    else:

        filter_dedup_bams(data_filenames, bam_filtered_nodup_filenames)

        info('Building BedGraph RPM track...')
        to_bedgraph(bam_filtered_nodup_filenames,
                    rpm_filenames,
                    binned_rpm_filenames,
                    chr_len_filename,
                    genome_sorted_bins_file,
                    read_ext)

        info('Converting BedGraph to BigWig')
        to_bigwig(bigwig_filenames,
                  rpm_filenames,
                  chr_len_filename)


    # step 8
    info('Normalize rpm tracks')


    df_chip = load_rpm_tracks(col_names, binned_rpm_filenames)

    coordinates_bin = pd.read_csv(genome_sorted_bins_file,
                                  names=['chr_id', 'bpstart', 'bpend'],
                                  sep='\t',
                                  header=None,
                                  usecols=[0, 1, 2])

    if disable_quantile_normalization:
        info('Skipping quantile normalization...')
        df_chip_to_bigwig(df_chip,
                          coordinates_bin,
                          col_names,
                          bedgraph_track_filenames,
                          bigwig_track_filenames)

    else:
        info('Normalizing the data...')

        df_chip = pd.DataFrame(quantile_normalization(df_chip.values),
                               columns=df_chip.columns,
                               index=df_chip.index)
        df_chip_to_bigwig(df_chip,
                          coordinates_bin,
                          col_names,
                          bedgraph_track_normalized_filenames,
                          bigwig_track_normalized_filenames)

        #import pyBigWig

       # bw1 = pyBigWig.open("/mnt/hd2/test_data/OUTPUT3/HAYSTACK_HOTSPOTS/TRACKS/K562.200bp_quantile_normalized.bw")
        #bw2 = pyBigWig.open("/mnt/hd2/test_data/HAYSTACK_HOTSPOTS/TRACKS/K562.200bp_quantile_normalized.bw")
        #bw1.header()



    # NOT COMPLETE

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
                    coord_zscore['z-score'] = df_chip_hpr_zscore.loc[
                        df_chip_hpr_zscore.loc[:, col] < z_score_high, col]
                else:
                    coord_zscore['z-score'] = df_chip_hpr_zscore.loc[
                        df_chip_hpr_zscore.loc[:, col] > z_score_high, col]
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
                    coord_zscore['z-score'] = df_chip_hpr_zscore.loc[
                        df_chip_hpr_zscore.loc[:, col] > z_score_low, col]
                else:
                    coord_zscore['z-score'] = df_chip_hpr_zscore.loc[
                        df_chip_hpr_zscore.loc[:, col] < z_score_low, col]

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

    #########################

    # step 10
    info('Determine HP regions')
    hpr_idxs, coordinates_bin, df_chip_hpr_zscore, hpr_iod_scores = find_hpr_coordinates(df_chip, coordinates_bin)
    info('hpr to bigwig')
    hpr_to_bigwig(coordinates_bin)
    info('hpr to bedgraph')
    hpr_to_bedgraph(hpr_idxs, coordinates_bin)
    info('Save files')
    write_specific_regions(coordinates_bin, df_chip_hpr_zscore)
    create_igv_track_file(hpr_iod_scores)

    info('All done! Ciao!')
    sys.exit(0)
