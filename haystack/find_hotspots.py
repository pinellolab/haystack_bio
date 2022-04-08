from __future__ import division
import os
import sys
import subprocess as sb
import numpy as np
import pandas as pd
import argparse
#from pybedtools import BedTool
import multiprocessing
import glob
from haystack_common import determine_path, check_file, check_required_packages, initialize_genome, HAYSTACK_VERSION

import logging
logging.basicConfig(level=logging.INFO,
                    format='%(levelname)-5s @ %(asctime)s:\n\t %(message)s \n',
                    datefmt='%a, %d %b %Y %H:%M:%S',
                    stream=sys.stderr,
                    filemode="w")
error = logging.critical
warn = logging.warning
debug = logging.debug
info = logging.info

do_not_recompute = None
keep_intermediate_files= None


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

def get_scaling_factor(bam_filename):

    cmd = 'sambamba view -c %s' % bam_filename
    # print cmd
    proc = sb.Popen(cmd, stdout=sb.PIPE,
                    shell=True)
    (stdout, stderr) = proc.communicate()
    # print stdout,stderr
    scaling_factor = (1.0 / float(stdout.strip())) * 1000000

    return scaling_factor

def get_args():
    # mandatory
    parser = argparse.ArgumentParser(description='HAYSTACK Parameters')
    parser.add_argument('samples_filename_or_bam_folder',
                        type=str,
                        help='A tab delimited file with in each row (1) a sample name, '
                             '(2) the path to the corresponding bam or bigwig filename. '
                             'Alternatively it is possible to specify a folder containing some .bam files to analyze.')
    parser.add_argument('genome_name',
                        type=str,
                        help='Genome assembly to use from UCSC (for example hg19, mm9, etc.)')

    # optional
    parser.add_argument('--output_directory',
                        type=str,
                        help='Output directory (default: current directory)',
                        default='')
    parser.add_argument('--bin_size',
                        type=int,
                        help='bin size to use(default: 500bp)',
                        default=500)
    parser.add_argument('--chrom_exclude',
                        type=str,
                        help='Exclude chromosomes that contain given (regex) string. For example _random|chrX|chrY excludes  random, X, and Y chromosome regions',
                        default='_|chrX|chrY')
    parser.add_argument('--th_rpm',
                        type=float,
                        help='Percentile on the signal intensity to consider for the hotspots (default: 99)',
                        default=99)
    parser.add_argument('--transformation',
                        type=str,
                        help='Variance stabilizing transformation among: none, log2, angle (default: angle)',
                        default='angle',
                        choices=['angle', 'log2', 'none'])
    parser.add_argument('--z_score_high',
                        type=float,
                        help='z-score value to select the specific regions(default: 1.5)',
                        default=1.5)
    parser.add_argument('--z_score_low',
                        type=float,
                        help='z-score value to select the not specific regions (default: 0.25)',
                        default=0.25)
    parser.add_argument('--read_ext',
                        type=int,
                        help='Read extension in bps (default: 200)',
                        default=200)
    parser.add_argument('--max_regions_percentage',
                        type=float,
                        help='Upper bound on the %% of the regions selected  (default: 0.1, 0.0=0%% 1.0=100%%)',
                        default=0.1)
    parser.add_argument('--name',
                        help='Define a custom output filename for the report',
                        default='')
    parser.add_argument('--blacklist',
                        type=str,
                        help='Exclude blacklisted regions. Blacklisted regions are not excluded by default. '
                             'Use hg19 to blacklist regions for the human genome build 19, '
                             'otherwise provide the filepath for a bed file with blacklisted regions.',
                        default='')
    parser.add_argument('--depleted',
                        help='Look for cell type specific regions with depletion of signal instead of enrichment',
                        action='store_true')
    parser.add_argument('--disable_quantile_normalization',
                        help='Disable quantile normalization (default: False)',
                        action='store_true')
    parser.add_argument('--do_not_recompute',
                        help='Keep any file previously precalculated',
                        action='store_true')
    parser.add_argument('--do_not_filter_bams',
                        help='Use BAM files as provided. Do not remove reads that are unmapped, mate unmapped,'
                             ' not primary aligned or low MAPQ reads, reads failing qc and optical duplicates',
                        action='store_true')
    parser.add_argument('--input_is_bigwig',
                        help='Use the bigwig format instead of the bam format for the input. '
                             'Note: The files must have extension .bw',
                        action='store_true')
    parser.add_argument('--keep_intermediate_files',
                        help='keep intermediate bedgraph files ',
                        action='store_true')
    parser.add_argument('--n_processes',
                        type=int,
                        help='Specify the number of processes to use. The default is #cores available.',
                        default=min(4, multiprocessing.cpu_count()))
    parser.add_argument('--version',
                        help='Print version and exit.',
                        action='version',
                        version='Version %s' % HAYSTACK_VERSION)
    return parser



def get_data_filepaths(samples_filename_or_bam_folder, input_is_bigwig):
    # check folder or sample filename
    if not os.path.exists(samples_filename_or_bam_folder):
        error("The file or folder %s doesn't exist. Exiting." %
              samples_filename_or_bam_folder)
        sys.exit(1)
    if os.path.isfile(samples_filename_or_bam_folder):
        data_filenames = []
        sample_names = []
        with open( samples_filename_or_bam_folder) as infile:
            for line in infile:
                if not line.strip():
                    continue
                if line.startswith('#'): # skip optional header line
                    info('Skipping header/comment line:%s' % line)
                    continue
                fields = line.strip().split()
                n_fields = len(fields)
                if n_fields == 2:
                    sample_names.append(fields[0])
                    data_filenames.append(fields[1])
                else:
                    error('The samples file format is wrong!')
                    sys.exit(1)
        # dir_path = os.path.dirname(os.path.realpath(samples_filename_or_bam_folder))
        # data_filenames = [os.path.join(dir_path, filename)
        #                     for filename in data_filenames]
    else:
        if input_is_bigwig:
            extension_to_check = '.bw'
            info('Input is set BigWig (.bw)')
        else:
            extension_to_check = '.bam'
            info('Input is set compressed SAM (.bam)')

        data_filenames = glob.glob(os.path.join(samples_filename_or_bam_folder,
                                                '*' + extension_to_check))
        if not data_filenames:
            error('No bam/bigwig  files to analyze in %s. Exiting.'
                  %  samples_filename_or_bam_folder)
            sys.exit(1)
        sample_names = [os.path.basename(data_filename).replace(extension_to_check, '')
                        for data_filename in data_filenames]
    # check all the files before starting
    info('Checking samples files location...')
    for data_filename in data_filenames:
        check_file(data_filename)
    return sample_names, data_filenames


def create_tiled_genome(genome_name,
                        output_directory,
                        chr_len_filename,
                        bin_size,
                        chrom_exclude,
                        blacklist):

    from re import search
    genome_directory = determine_path('genomes')
    annotations_directory = determine_path('gene_annotations')

    genome_sorted_bins_file = os.path.join(output_directory, '%s.%dbp.bins.sorted.bed'
                                           % (os.path.basename(genome_name), bin_size))


    chr_len_sorted_filtered_filename = os.path.join(output_directory,
                                                    "%s_chr_lengths_sorted_filtered.txt" % genome_name)


    if not (os.path.exists(genome_sorted_bins_file) and do_not_recompute):

        info('Sorting chromosome lengths file once again to double check....')

        cmd = ' sort -k1,1 -k2,2n  "%s" -o  "%s" ' % (chr_len_filename,
                                                      chr_len_filename)

        sb.call(cmd, shell=True)

        info('Creating bins of %dbp in %s' % (bin_size, genome_sorted_bins_file))

        if chrom_exclude:

            with open(chr_len_sorted_filtered_filename, 'wb') as f:
                f.writelines(line for line in open(chr_len_filename)
                             if not search(chrom_exclude, line.split()[0]))
        else:
            chr_len_sorted_filtered_filename = chr_len_filename


        cmd = 'bedtools makewindows -g "%s" -w %s   > "%s" ' % (chr_len_sorted_filtered_filename,
                                                                bin_size,
                                                                genome_sorted_bins_file)

        sb.call(cmd, shell=True)

        if blacklist == 'none':
            info('Tiled genome file created will not be blacklist filtered')

        else:
            info('Tiled genome file created will be blacklist filtered')

            if blacklist=='hg19':
                info('Using hg19 blacklist file %s to filter out the regions' %blacklist)
                blacklist_filepath = os.path.join(annotations_directory,
                                                  'hg19_blacklisted_regions.bed')
                check_file(blacklist_filepath)
            elif os.path.isfile(blacklist):

                info('Using blacklist file %s to filter out the regions' % blacklist)
                blacklist_filepath = blacklist
                check_file(blacklist_filepath)


            else:
                error('Incorrect blacklist option provided. '
                      'It is neither a file nor a genome')
                sys.exit(1)

            info('Sort blacklist file')

            cmd = ' sort -k1,1 -k2,2n  "%s" -o  "%s" ' % (blacklist_filepath,
                                                          blacklist_filepath)

            sb.call(cmd, shell=True)


            genome_sorted_bins_filtered_file = genome_sorted_bins_file.replace('.bed', '.filtered.bed')

            info(' filter out blacklist regions')

            cmd = 'bedtools intersect -sorted -a "%s" -b "%s"  -v  > %s ' % (genome_sorted_bins_file,
                                                                             blacklist_filepath,
                                                                             genome_sorted_bins_filtered_file)

            sb.call(cmd, shell=True)

            if not keep_intermediate_files:
                info('Deleting %s' % genome_sorted_bins_file)
                try:
                    os.remove(genome_sorted_bins_file)
                except:
                    pass

            genome_sorted_bins_file = genome_sorted_bins_filtered_file

    return genome_sorted_bins_file


### if bigwig

def create_tiled_genome_with_id(genome_sorted_bins_file):

    if os.path.exists(genome_sorted_bins_file):

        info(' Adding an id column to tiled genome file')

        genome_sorted_bins_id_file = genome_sorted_bins_file.replace('.bed', '_with_id.bed')

        awk_command = "awk \'{printf(\"%s\\t%d\\t%d\\tp_%s\\n\", $1,$2,$3,NR)}\'" + ' < "%s" > "%s" '  % (genome_sorted_bins_file,
                                                                                                          genome_sorted_bins_id_file)

        sb.call(awk_command, shell=True)

    else:
        error('The file %s does not exist. Tiled genome was not generated in previous step' % genome_sorted_bins_file)
        sys.exit(1)

    return genome_sorted_bins_id_file

def to_binned_tracks_if_bigwigs(data_filenames,
                                intermediate_directory,
                                binned_sample_names,
                                genome_sorted_bins_id_file):

    binned_rpm_filenames = [os.path.join(intermediate_directory,
                                         '%s.rpm' % binned_sample_name)
                            for binned_sample_name in binned_sample_names]


    binned_bedgraph_unsorted_filenames = [os.path.join(intermediate_directory,
                                         '%s.unsorted.rpm.bedgraph' % binned_sample_name)
                            for binned_sample_name in binned_sample_names]


    binned_bedgraph_filenames = [os.path.join(intermediate_directory,
                                         '%s.rpm.bedgraph' % binned_sample_name)
                            for binned_sample_name in binned_sample_names]




    for data_filename, binned_bedgraph_unsorted_filename, binned_rpm_filename, binned_bedgraph_filename in zip(data_filenames,
                                                                                                               binned_bedgraph_unsorted_filenames,
                                                                                                               binned_rpm_filenames,
                                                                                                               binned_bedgraph_filenames):

        info('Processing:%s' % data_filename)

        if not (os.path.exists(binned_rpm_filename) and do_not_recompute):
            info('Computing average score of bigwig over each bin in tiled genome')

            cmd1 = 'bigWigAverageOverBed "%s" "%s" /dev/null -bedOut="%s"' % (
                data_filename,
                genome_sorted_bins_id_file,
                binned_bedgraph_unsorted_filename)

            sb.call(cmd1, shell=True)


            info('Sorting the output of bigWigAverageOverBed and saving the fifth column (mean) to rpm file')


            cmd2 = 'sort -k1,1 -k2,2n "%s" | cut -f1,2,3,5 | tee "%s" | cut -f4 > "%s"' % (binned_bedgraph_unsorted_filename,
                                                                           binned_bedgraph_filename,
                                                                           binned_rpm_filename)

            sb.call(cmd2, shell=True)

            if not keep_intermediate_files:
                info('Deleting %s' % binned_bedgraph_unsorted_filename)
                try:
                    os.remove(binned_bedgraph_unsorted_filename)
                except:
                    pass

    return binned_rpm_filenames
#######
# convert bam files to genome-wide rpm tracks

def to_filtered_deduped_bams(bam_filenames,
                             output_directory,
                             n_processes):

    filtered_bam_directory = os.path.join(output_directory, 'FILTERED_BAMS')

    if not os.path.exists(filtered_bam_directory):
        os.makedirs(filtered_bam_directory)

    bam_filtered_nodup_filenames = [os.path.join(
        filtered_bam_directory,
        '%s.filtered.nodup%s' % (os.path.splitext(os.path.basename(bam_filename))))
        for bam_filename in bam_filenames]

    for bam_filename, bam_filtered_nodup_filename in zip(bam_filenames,
                                                         bam_filtered_nodup_filenames):

        if not (os.path.exists(bam_filtered_nodup_filename) and do_not_recompute):

            info('Processing:%s' % bam_filename)
            bam_temp_filename = os.path.join(os.path.dirname(bam_filtered_nodup_filename),
                                             '%s.temp%s' % os.path.splitext(
                                                 os.path.basename(bam_filtered_nodup_filename)))
            info('Removing  unmapped, mate unmapped, not primary alignment,'
                 ' low MAPQ reads, and reads failing qc')

            cmd = 'sambamba view -f bam -l 0 -t %d -F ' \
                  '"not (unmapped or mate_is_unmapped or' \
                  ' failed_quality_control or duplicate' \
                  ' or secondary_alignment)and mapping_quality >= 30"' \
                  ' "%s"  -o "%s"' % (n_processes,
                                      bam_filename,
                                      bam_temp_filename)

            # cmd = 'sambamba view -f bam -l 0 -t %d -F "not failed_quality_control" "%s"  -o "%s"' % (n_processes,
            #                                                                                          bam_filename,
            #                                                                                          bam_temp_filename)
            sb.call(cmd, shell=True)
            info('Removing  optical duplicates')
            cmd = 'sambamba markdup  -l 5 -t %d  "%s" "%s" ' % (
                n_processes,
                bam_temp_filename,
                bam_filtered_nodup_filename)
            sb.call(cmd, shell=True)

            try:
                os.remove(bam_temp_filename)
                os.remove(bam_temp_filename + '.bai')
            except:
                pass

    return bam_filtered_nodup_filenames


def to_normalized_extended_reads_tracks(bam_filenames,
                                        sample_names,
                                        tracks_directory,
                                        chr_len_filename,
                                        read_ext):

    bedgraph_filenames = [os.path.join(tracks_directory, '%s.bedgraph' % sample_name)
                          for sample_name in sample_names]
    bigwig_filenames = [filename.replace('.bedgraph', '.bw')
                        for filename in bedgraph_filenames]

    for bam_filename, bedgraph_filename, bigwig_filename in zip(bam_filenames,
                                                                bedgraph_filenames,
                                                                bigwig_filenames):

        if not (os.path.exists(bedgraph_filename) and do_not_recompute):
            info('Processing:%s' % bam_filename)

            info('Computing Scaling Factor...')
            scaling_factor = get_scaling_factor(bam_filename)
            info('Scaling Factor: %e' % scaling_factor)
            info('Converting %s to bed and extending read length...' %bam_filename)
            info('Normalizing counts by scaling factor...')


            cmd = 'sambamba view -f bam "%s" | bamToBed | slopBed  -r "%s" -l 0 -s -i stdin -g "%s" | ' \
                  'genomeCoverageBed -g  "%s" -i stdin -bg -scale %.64f| sort -k1,1 -k2,2n  > "%s"' % (
            bam_filename, read_ext, chr_len_filename, chr_len_filename, scaling_factor, bedgraph_filename)
            sb.call(cmd, shell=True)

            if not keep_intermediate_files:
                info('Deleting %s' % bam_filename)
                try:
                    os.remove(bam_filename)
                except:
                    pass

        if keep_intermediate_files:
            if not (os.path.exists(bigwig_filename) and do_not_recompute):
                info('Converting BedGraph to BigWig')
                cmd = 'bedGraphToBigWig "%s" "%s" "%s"' % (bedgraph_filename,
                                                           chr_len_filename,
                                                           bigwig_filename)
                sb.call(cmd, shell=True)

                info('Converted %s to bigwig' % bedgraph_filename)

    return bedgraph_filenames, bigwig_filenames

def to_binned_tracks(bedgraph_filenames,
                     binned_sample_names,
                     tracks_directory,
                     intermediate_directory,
                     chr_len_filename,
                     genome_sorted_bins_file):

    bedgraph_binned_filenames = [os.path.join(tracks_directory,
                                              '%s.bedgraph' % binned_sample_name)
                                 for binned_sample_name in binned_sample_names]

    binned_rpm_filenames = [os.path.join(intermediate_directory,
                                         '%s.rpm' % binned_sample_name)
                            for binned_sample_name in binned_sample_names]

    bigwig_binned_filenames = [filename.replace('.bedgraph', '.bw')
                               for filename in bedgraph_binned_filenames]

    for bedgraph_filename, bedgraph_binned_filename,\
        binned_rpm_filename, bigwig_binned_filename in zip(bedgraph_filenames,
                                                           bedgraph_binned_filenames,
                                                           binned_rpm_filenames,
                                                           bigwig_binned_filenames):

        if not (os.path.exists(binned_rpm_filename) and do_not_recompute):
            info('Making constant binned rpm values file: %s' % binned_rpm_filename)

            cmd = 'bedtools map -a "%s" -b "%s" -c 4 -o mean -null 0.0 > "%s"' % (genome_sorted_bins_file,
                                                                                       bedgraph_filename,
                                                                                       bedgraph_binned_filename)
            sb.call(cmd, shell=True)

            cmd = 'cut -f4 "%s"  > "%s" ' % (bedgraph_binned_filename, binned_rpm_filename)
            sb.call(cmd, shell=True)

            info('Binned Bedgraph saved...')

        if not (os.path.exists(bigwig_binned_filename) and do_not_recompute):
            info('Converting BedGraph to BigWig')
            cmd = 'bedGraphToBigWig "%s" "%s" "%s"' % (bedgraph_binned_filename,
                                                       chr_len_filename,
                                                       bigwig_binned_filename)
            sb.call(cmd, shell=True)
            info('Converted %s to bigwig' % bedgraph_binned_filename)
            if not keep_intermediate_files:
                info('Deleting %s' % bedgraph_binned_filename)
                try:
                    os.remove(bedgraph_binned_filename)
                except:
                    pass

    return binned_rpm_filenames

def load_binned_rpm_tracks(binned_sample_names, binned_rpm_filenames):
    # load all the tracks
    info('Loading the processed tracks')
    df_chip = {}
    for binned_sample_name, binned_rpm_filename in zip(binned_sample_names,
                                                       binned_rpm_filenames):

        df_chip[binned_sample_name] = pd.read_csv(binned_rpm_filename,
                                                  squeeze=True,
                                                  header=None)

        info('Loading %s from file %s' % (binned_sample_name,
                                          binned_rpm_filename))

    df_chip = pd.DataFrame(df_chip)

    return df_chip


def to_binned_normalized_tracks(df_chip,
                                coordinates_bin,
                                binned_sample_names,
                                chr_len_filename,
                                tracks_directory):

    df_chip_normalized = pd.DataFrame(quantile_normalization(df_chip.values),
                                      columns=df_chip.columns,
                                      index=df_chip.index)

    bedgraph_binned_normalized_filenames = [os.path.join(tracks_directory,
                                                         '%s_quantile_normalized.bedgraph' % binned_sample_name)
                                            for binned_sample_name in binned_sample_names]
    bigwig_binned_normalized_filenames = [filename.replace('.bedgraph', '.bw')
                                          for filename in bedgraph_binned_normalized_filenames]
    # write quantile normalized tracks
    for binned_sample_name, bedgraph_binned_normalized_filename, bigwig_binned_normalized_filename in zip(
            binned_sample_names,
            bedgraph_binned_normalized_filenames,
            bigwig_binned_normalized_filenames):

        if not (os.path.exists(bedgraph_binned_normalized_filename) and do_not_recompute):
            info('Writing binned track: %s' % bigwig_binned_normalized_filename)
            joined_df = pd.DataFrame.join(coordinates_bin, df_chip_normalized[binned_sample_name])
            joined_df.to_csv(bedgraph_binned_normalized_filename,
                             sep='\t',
                             header=False,
                             index=False)

        if not (os.path.exists(bigwig_binned_normalized_filename) and do_not_recompute):
            cmd = 'bedGraphToBigWig "%s" "%s" "%s"' % (bedgraph_binned_normalized_filename,
                                                       chr_len_filename,
                                                       bigwig_binned_normalized_filename)
            sb.call(cmd, shell=True)
            info('Converted %s to bigwig' % bedgraph_binned_normalized_filename)
            if not keep_intermediate_files:
                info('Deleting %s' % bedgraph_binned_normalized_filename)
                try:
                    os.remove(bedgraph_binned_normalized_filename)
                except:
                    pass

    return df_chip_normalized, bigwig_binned_normalized_filenames

def find_hpr_coordinates(df_chip,
                         coordinates_bin,
                         th_rpm,
                         transformation,
                         max_regions_percentage,
                         output_directory):

    from scipy.stats import zscore
    import matplotlib as mpl
    mpl.use('Agg')
    import pylab as pl

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

    df_chip_hpr_zscore = df_chip_not_empty.loc[hpr_idxs, :].apply(zscore, axis=1,
                                                                  result_type ='broadcast')


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
    pl.savefig(os.path.join(output_directory,
                            'SELECTION_OF_VARIABILITY_HOTSPOT.pdf'))
    pl.close()

    return hpr_idxs, coordinates_bin, df_chip_hpr_zscore, hpr_iod_scores


def hpr_to_bigwig(coordinates_bin,
                  tracks_directory,
                  chr_len_filename):

    bedgraph_iod_track_filename = os.path.join(tracks_directory,
                                               'VARIABILITY.bedgraph')

    bw_iod_track_filename = os.path.join(tracks_directory,
                                         'VARIABILITY.bw')

    # create a track for IGV

    if not (os.path.exists(bw_iod_track_filename) and do_not_recompute):

        info('Generating variability track in bigwig format in:%s'
             % bw_iod_track_filename)

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
    return bw_iod_track_filename


def hpr_to_bedgraph(hpr_idxs,
                    coordinates_bin,
                    tracks_directory,
                    output_directory):

    bedgraph_hpr_filename = os.path.join(tracks_directory,
                                         'SELECTED_VARIABILITY_HOTSPOT.bedgraph')

    bed_hpr_filename = os.path.join(output_directory,
                                    'SELECTED_VARIABILITY_HOTSPOT.bed')

    to_write = coordinates_bin.loc[hpr_idxs[hpr_idxs].index]
    to_write.dropna(inplace=True)
    to_write['bpstart'] = to_write['bpstart'].astype(int)
    to_write['bpend'] = to_write['bpend'].astype(int)

    to_write.to_csv(bedgraph_hpr_filename,
                    sep='\t',
                    header=False,
                    index=False)

    if not (os.path.exists(bed_hpr_filename) and do_not_recompute):

        info('Writing the HPRs in: "%s"' % bed_hpr_filename)
        sb.call(' bedtools merge -i "%s" >  "%s"' % (bedgraph_hpr_filename,
                                                      bed_hpr_filename),
                shell=True)

    return bed_hpr_filename

def write_specific_regions(coordinates_bin,
                           df_chip_hpr_zscore,
                           specific_regions_directory,
                           depleted,
                           z_score_low,
                           z_score_high):
    # write target
    info('Writing Specific Regions for each cell line...')
    coord_zscore = coordinates_bin.copy()
    for col in df_chip_hpr_zscore:
        regions_specific_filename = 'Regions_specific_for_%s_z_%.2f.bedgraph' % (
            os.path.basename(col).replace('.rpm', ''), z_score_high)
        specific_output_filename = os.path.join(specific_regions_directory,
                                                regions_specific_filename)
        specific_output_bed_filename = specific_output_filename.replace('.bedgraph', '.bed')

        if not (os.path.exists(specific_output_bed_filename) and do_not_recompute):
            if depleted:
                z_score_high = -z_score_high
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
            sb.call(' bedtools merge -i  "%s" >  "%s"' % (specific_output_filename,
                                                          specific_output_bed_filename),
                    shell=True)
    # write background
    info('Writing Background Regions for each cell line...')
    coord_zscore = coordinates_bin.copy()
    for col in df_chip_hpr_zscore:

        bg_output_filename = os.path.join(specific_regions_directory,
                                          'Background_for_%s_z_%.2f.bedgraph' % (
                                              os.path.basename(col).replace('.rpm', ''),
                                              z_score_low))

        bg_output_bed_filename = bg_output_filename.replace('.bedgraph', '.bed')

        if not (os.path.exists(bg_output_bed_filename) and do_not_recompute):

            if depleted:
                z_score_low = -z_score_low
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
                ' bedtools merge -i "%s" >  "%s"' % (bg_output_filename,
                                                      bg_output_bed_filename),
                shell=True)


def create_igv_track_file(hpr_iod_scores,
                          bed_hpr_filename,
                          bw_iod_track_filename,
                          genome_name,
                          output_directory,
                          binned_sample_names,
                          sample_names,
                          disable_quantile_normalization):

    import xml.etree.cElementTree as ET
    import glob

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
    for binned_sample_name in binned_sample_names:
        if disable_quantile_normalization:
            track_full_path = os.path.join(output_directory, 'TRACKS', '%s.bw' % binned_sample_name)
        else:
            track_full_path = os.path.join(output_directory, 'TRACKS',
                                           '%s_quantile_normalized.bw' % binned_sample_name)

        track_filename = rem_base_path(track_full_path, output_directory)

        if os.path.exists(track_full_path):
            resource_items.append(ET.SubElement(resources, "Resource"))
            resource_items[-1].set("path", track_filename)
            track_items.append(ET.SubElement(panel, "Track"))
            track_items[-1].set('color', "0,0,178")
            track_items[-1].set('id', track_filename)
            track_items[-1].set("name", binned_sample_name)

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
    resource_items[-1].set("path", rem_base_path(bed_hpr_filename, output_directory))
    track_items.append(ET.SubElement(panel, "Track"))
    track_items[-1].set('color', "178,0,0")
    track_items[-1].set('id', rem_base_path(bed_hpr_filename, output_directory))
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


def main(input_args=None):

    # input_args = ["/home/rfarouni/Documents/haystack_bio/haystack/haystack_data/test_data/samples_names_hotspots.txt",
    #               "hg19",
    #               '--blacklist',
    #               "hg19",
    #               '--output_directory',
    #               "/home/rfarouni/haystack_test_output/HAYSTACK_PIPELINE_RESULTS2",
    #               "--chrom_exclude",
    #               'chr(?!21)']
    # input_args.append('--keep_intermediate_files')

    print '\n[H A Y S T A C K   H O T S P O T]'
    print('\n-SELECTION OF VARIABLE REGIONS-\n')
    print 'Version %s\n' % HAYSTACK_VERSION

    # step 1
    check_required_packages()
    # step 2
    parser = get_args()
    args = parser.parse_args(input_args)

    args.n_processes = max(1, args.n_processes - 1)

    info(vars(args))
    global do_not_recompute
    global keep_intermediate_files
    do_not_recompute = args.do_not_recompute
    keep_intermediate_files= args.keep_intermediate_files
    # step 3: create directories
    if args.name:
        directory_name = 'HAYSTACK_HOTSPOTS_on_%s' % args.name
    else:
        directory_name = 'HAYSTACK_HOTSPOTS'
    if args.output_directory:
        output_directory = os.path.join(args.output_directory,
                                        directory_name)
    else:
        output_directory = directory_name
    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    tracks_directory = os.path.join(output_directory,
                                    'TRACKS')
    if not os.path.exists(tracks_directory):
        os.makedirs(tracks_directory)
    intermediate_directory = os.path.join(output_directory,
                                          'INTERMEDIATE')
    if not os.path.exists(intermediate_directory):
        os.makedirs(intermediate_directory)
    specific_regions_directory = os.path.join(output_directory,
                                              'SPECIFIC_REGIONS')
    if not os.path.exists(specific_regions_directory):
        os.makedirs(specific_regions_directory)

    # step 4: get filepaths
    sample_names, data_filenames = get_data_filepaths(args.samples_filename_or_bam_folder,
                                                      args.input_is_bigwig)
    binned_sample_names = ['%s.%dbp' % (sample_name, args.bin_size)
                           for sample_name in sample_names]
    # step 5: get genome data
    _, chr_len_filename, _= initialize_genome(args.genome_name)

    # step 6: create tiled genome
    genome_sorted_bins_file = create_tiled_genome(args.genome_name,
                                                  output_directory,
                                                  chr_len_filename,
                                                  args.bin_size,
                                                  args.chrom_exclude,
                                                  args.blacklist)
    # step 7:Convert files to genome-wide rpm tracks
    if args.input_is_bigwig:

        genome_sorted_bins_file = create_tiled_genome_with_id(genome_sorted_bins_file)



        binned_rpm_filenames = to_binned_tracks_if_bigwigs(data_filenames,
                                                           intermediate_directory,
                                                           binned_sample_names,
                                                           genome_sorted_bins_file)
    else:

        if args.do_not_filter_bams:
            bam_filtered_nodup_filenames= data_filenames
        else:
            bam_filtered_nodup_filenames=to_filtered_deduped_bams(data_filenames,
                                                              output_directory,
                                                              args.n_processes)

        bedgraph_filenames, bigwig_filenames=\
            to_normalized_extended_reads_tracks(bam_filtered_nodup_filenames,
                                                sample_names,
                                                tracks_directory,
                                                chr_len_filename,
                                                args.read_ext)



        binned_rpm_filenames = to_binned_tracks(bedgraph_filenames,
                                                binned_sample_names,
                                                tracks_directory,
                                                intermediate_directory,
                                                chr_len_filename,
                                                genome_sorted_bins_file)


    # step 8: quantile normalize the data
    df_chip = load_binned_rpm_tracks(binned_sample_names,
                                     binned_rpm_filenames)

    coordinates_bin = pd.read_csv(genome_sorted_bins_file,
                                  names=['chr_id',
                                         'bpstart',
                                         'bpend'],
                                  sep='\t',
                                  header=None,
                                  usecols=[0, 1, 2])

    if not args.disable_quantile_normalization:
        info('Normalizing the data...')

        df_chip, bigwig_binned_normalized_filenames = \
            to_binned_normalized_tracks(df_chip,
                                        coordinates_bin,
                                        binned_sample_names,
                                        chr_len_filename,
                                        tracks_directory)
# step 9
    info('Determine High Plastic Regions (HPR)')
    hpr_idxs, coordinates_bin, df_chip_hpr_zscore, hpr_iod_scores =\
        find_hpr_coordinates(df_chip,
                             coordinates_bin,
                             args.th_rpm,
                             args.transformation,
                             args.max_regions_percentage,
                             output_directory)
    info('HPR to bigwig')
    bw_iod_track_filename = hpr_to_bigwig(coordinates_bin,
                                          tracks_directory,
                                          chr_len_filename)
    info('HPR to bedgraph')
    bed_hpr_filename = hpr_to_bedgraph(hpr_idxs,
                                       coordinates_bin,
                                       tracks_directory,
                                       output_directory)
    info('Save files')
    write_specific_regions(coordinates_bin,
                           df_chip_hpr_zscore,
                           specific_regions_directory,
                           args.depleted,
                           args.z_score_low,
                           args.z_score_high)
    create_igv_track_file(hpr_iod_scores,
                          bed_hpr_filename,
                          bw_iod_track_filename,
                          args.genome_name,
                          output_directory,
                          binned_sample_names,
                          sample_names,
                          args.disable_quantile_normalization)

    info('All done! Ciao!')

if __name__ == '__main__':
    main()
    sys.exit(0)
