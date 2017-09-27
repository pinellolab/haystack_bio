from __future__ import division
import os
import sys
import glob
import shutil
import argparse
import multiprocessing
import find_hotspots as hotspots
import find_motifs as motifs
import generate_tf_activity_plane as tf_activity_plane
# commmon functions
from haystack_common import check_file
import logging
logging.basicConfig(level=logging.INFO,
                    format='%(levelname)-5s @ %(asctime)s:\n\t %(message)s \n',
                    datefmt='%a, %d %b %Y %H:%M:%S',
                    stream=sys.stderr,
                    filemode="w"
                    )
error = logging.critical
warn = logging.warning
debug = logging.debug
info = logging.info

HAYSTACK_VERSION = "0.5.2"

def get_args_pipeline():
    # mandatory
    parser = argparse.ArgumentParser(description='HAYSTACK Parameters')
    parser.add_argument('samples_filename_or_bam_folder', type=str,
                        help='A tab delimeted file with in each row (1) a sample name, (2) the path to the corresponding bam filename, (3 optional) the path to the corresponding gene expression filename.')
    parser.add_argument('genome_name', type=str, help='Genome assembly to use from UCSC (for example hg19, mm9, etc.)')

    # optional
    parser.add_argument('--name', help='Define a custom output filename for the report', default='')
    parser.add_argument('--output_directory', type=str, help='Output directory (default: current directory)',
                        default='')
    parser.add_argument('--bin_size', type=int, help='bin size to use(default: 500bp)', default=500)
    parser.add_argument('--do_not_recompute',
                        help='Keep any file previously precalculated',
                        action='store_true')
    parser.add_argument('--do_not_filter_bams',
                        help='Use BAM files as provided. Do not remove reads that are unmapped, mate unmapped,'
                             ' not primary aligned or low MAPQ reads, reads failing qc and optical duplicates',
                        action='store_true')
    parser.add_argument('--depleted',
                        help='Look for cell type specific regions with depletion of signal instead of enrichment',
                        action='store_true')
    parser.add_argument('--input_is_bigwig',
                        help='Use the bigwig format instead of the bam format for the input. Note: The files must have extension .bw',
                        action='store_true')
    parser.add_argument('--disable_quantile_normalization', help='Disable quantile normalization (default: False)',
                        action='store_true')
    parser.add_argument('--transformation', type=str,
                        help='Variance stabilizing transformation among: none, log2, angle (default: angle)',
                        default='angle', choices=['angle', 'log2', 'none'])
    parser.add_argument('--z_score_high', type=float, help='z-score value to select the specific regions(default: 1.5)',
                        default=1.5)
    parser.add_argument('--z_score_low', type=float,
                        help='z-score value to select the not specific regions(default: 0.25)', default=0.25)
    parser.add_argument('--th_rpm', type=float,
                        help='Percentile on the signal intensity to consider for the hotspots (default: 99)',
                        default=99)
    parser.add_argument('--meme_motifs_filename', type=str,
                        help='Motifs database in MEME format (default JASPAR CORE 2016)')
    parser.add_argument('--motif_mapping_filename', type=str,
                        help='Custom motif to gene mapping file (the default is for JASPAR CORE 2016 database)')
    parser.add_argument('--plot_all',
                        help='Disable the filter on the TF activity and correlation (default z-score TF>0 and rho>0.3)',
                        action='store_true')
    parser.add_argument('--keep_intermediate_files',
                        help='keep intermediate bedgraph files ',
                        action='store_true')
    parser.add_argument('--n_processes', type=int,
                        help='Specify the number of processes to use. The default is #cores available.',
                        default=min(4, multiprocessing.cpu_count()))
    parser.add_argument('--blacklist',
                        type=str,
                        help='Exclude blacklisted regions. Blacklisted regions are not excluded by default. '
                             'Use hg19 to blacklist regions for the human genome 19, '
                             'otherwise provide the filepath for a bed file with blacklisted regions.',
                        default='')
    parser.add_argument('--chrom_exclude',
                        type=str,
                        help='Exclude chromosomes that contain given (regex) string. For example _random|chrX|chrY excludes  random, X, and Y chromosome regions',
                        default='_|chrX|chrY')
    parser.add_argument('--read_ext', type=int, help='Read extension in bps (default: 200)', default='200')
    parser.add_argument('--temp_directory', help='Directory to store temporary files  (default: /tmp)', default='/tmp')
    parser.add_argument('--version', help='Print version and exit.', action='version',
                        version='Version %s' % HAYSTACK_VERSION)

    return parser

def main(input_args=None):


    # input_args = ["/mnt/hd2/DATA_PINELLO//PROJECTS/2017_06_HAYSTACK/precomputed_tracks/H3K27ac/sample_names_H3K27ac.txt",
    #               "hg19",
    #               '--blacklist',
    #               "hg19",
    #               '--output_directory',
    #               "/mnt/hd2/DATA_PINELLO/PROJECTS/2017_06_HAYSTACK/precomputed_tracks/H3K27ac/th_rpm99_zscore1_try2_ml002"]


    #output_directory= "/mnt/hd2/DATA_PINELLO/PROJECTS/2017_06_HAYSTACK/precomputed_tracks/H3K27ac/th_rpm99_zscore1_try2_ml002"

    #samples_filename_or_bam_folder= "/mnt/hd2/DATA_PINELLO//PROJECTS/2017_06_HAYSTACK/precomputed_tracks/H3K27ac/sample_names_H3K27ac.txt"


    print '\n[H A Y S T A C K   P I P E L I N E]'
    print('\n-SELECTION OF HOTSPOTS OF VARIABILITY AND ENRICHED MOTIFS-\n')
    print 'Version %s\n' % HAYSTACK_VERSION
    parser = get_args_pipeline()
    args = parser.parse_args(input_args)

    args_dict = vars(args)
    for key, value in args_dict.items():
        exec ('%s=%s' % (key, repr(value)))

    if meme_motifs_filename:
        check_file(meme_motifs_filename)

    if motif_mapping_filename:
        check_file(motif_mapping_filename)

    if not os.path.exists(temp_directory):
        error('The folder specified with --temp_directory: %s does not exist!' % temp_directory)
        sys.exit(1)

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
    if not os.path.exists(samples_filename_or_bam_folder):
        error("The file or folder %s doesn't exist. Exiting." %
              samples_filename_or_bam_folder)
        sys.exit(1)

    if os.path.isfile(samples_filename_or_bam_folder):
        BAM_FOLDER = False
        data_filenames = []
        gene_expression_filenames = []
        sample_names = []

        with open(samples_filename_or_bam_folder) as infile:
            for line in infile:

                if not line.strip():
                    continue

                if line.startswith('#'):  # skip optional header line or empty lines
                    info('Skipping header/comment line:%s' % line)
                    continue

                fields = line.strip().split()
                n_fields = len(fields)

                if n_fields == 2:

                    USE_GENE_EXPRESSION = False

                    sample_names.append(fields[0])
                    data_filenames.append(fields[1])

                elif n_fields == 3:

                    USE_GENE_EXPRESSION = USE_GENE_EXPRESSION and True
                    sample_names.append(fields[0])
                    data_filenames.append(fields[1])
                    gene_expression_filenames.append(fields[2])
                else:
                    error('The samples file format is wrong!')
                    sys.exit(1)
    else:
        if os.path.exists(samples_filename_or_bam_folder):
            BAM_FOLDER = True
            USE_GENE_EXPRESSION = False
            data_filenames = glob.glob(os.path.join(samples_filename_or_bam_folder, '*' + extension_to_check))

            if not data_filenames:
                error('No bam/bigwig  files to analyze in %s. Exiting.' % samples_filename_or_bam_folder)
                sys.exit(1)

            sample_names = [os.path.basename(data_filename).replace(extension_to_check, '') for data_filename in
                            data_filenames]
        else:
            error("The file or folder %s doesn't exist. Exiting." % samples_filename_or_bam_folder)
            sys.exit(1)

    # dir_path = os.path.dirname(os.path.realpath(samples_filename_or_bam_folder))
    # data_filenames = [os.path.join(dir_path, filename)
    #                   for filename in data_filenames]
    # gene_expression_filenames = [os.path.join(dir_path, filename)
    #                   for filename in gene_expression_filenames]

    # check all the files before starting
    info('Checking samples files location...')
    for data_filename in data_filenames:
        check_file(data_filename)

    if USE_GENE_EXPRESSION:
        for gene_expression_filename in gene_expression_filenames:
            check_file(gene_expression_filename)

    if not os.path.exists(output_directory):
        os.makedirs(output_directory)

    # copy back the file used
    if not BAM_FOLDER:
        shutil.copy2(samples_filename_or_bam_folder, output_directory)

    # write hotspots conf files
    sample_names_hotspots_filename = os.path.join(output_directory,
                                                  'sample_names_hotspots.txt')

    with open(sample_names_hotspots_filename, 'w+') as outfile:
        for sample_name, data_filename in zip(sample_names, data_filenames):
            outfile.write('%s\t%s\n' % (sample_name, data_filename))


    # CALL HAYSTACK HOTSPOTS
    input_args=[sample_names_hotspots_filename,
                genome_name,
                '--blacklist',
                blacklist,
                '--output_directory',
                output_directory,
                '--bin_size',
                '{:d}'.format(bin_size),
                '--chrom_exclude',
                chrom_exclude,
                '--th_rpm',
                '{:f}'.format(th_rpm),
                '--transformation',
                transformation,
                '--z_score_high',
                '{:f}'.format(z_score_high),
                '--z_score_low',
                '{:f}'.format(z_score_low),
                '--read_ext',
                '{:d}'.format(read_ext)]
    if do_not_recompute:
        input_args.append('--do_not_recompute')
    if do_not_filter_bams:
        input_args.append('--do_not_filter_bams')
    if depleted:
        input_args.append('--depleted')
    if input_is_bigwig:
        input_args.append('--input_is_bigwig')
    if disable_quantile_normalization:
        input_args.append('--disable_quantile_normalization')
    if keep_intermediate_files:
        input_args.append('--keep_intermediate_files')

    hotspots.main(input_args=input_args)


    #
    # # write motifs conf files
    # target_motifs_filepaths_file = os.path.join(output_directory,
    #                                               'target_motifs_bed_filepaths.txt')
    #
    #
    # with open(target_motifs_filepaths_file, 'w+') as outfile:
    #
    #     for sample_name in sample_names:
    #         specific_regions_filename = glob.glob(os.path.join(output_directory,
    #                                                            'HAYSTACK_HOTSPOTS',
    #                                                            'SPECIFIC_REGIONS',
    #                                                            'Regions_specific_for_%s*.bed' % sample_name))[0]
    #
    #         bg_regions_filename = glob.glob(os.path.join(output_directory,
    #                                                      'HAYSTACK_HOTSPOTS',
    #                                                      'SPECIFIC_REGIONS',
    #                                                      'Background_for_%s*.bed' % sample_name))[0]
    #
    #         outfile.write('%s\t%s\t%s\n' % (sample_name, specific_regions_filename, bg_regions_filename))



    # CALL HAYSTACK MOTIFS

    motif_directory = os.path.join(output_directory,
                                   'HAYSTACK_MOTIFS')


    for sample_name in sample_names:
        specific_regions_filename = os.path.join(output_directory, 'HAYSTACK_HOTSPOTS', 'SPECIFIC_REGIONS',
                                                 'Regions_specific_for_%s*.bed' % sample_name)
        bg_regions_filename = glob.glob(os.path.join(output_directory, 'HAYSTACK_HOTSPOTS', 'SPECIFIC_REGIONS',
                                                     'Background_for_%s*.bed' % sample_name))[0]
        # bg_regions_filename=glob.glob(specific_regions_filename.replace('Regions_specific','Background')[:-11]+'*.bed')[0] #lo zscore e' diverso...
        # print specific_regions_filename,bg_regions_filename
        cmd_to_run = 'haystack_motifs %s %s --bed_bg_filename %s --output_directory %s --name %s' % (
        specific_regions_filename, genome_name, bg_regions_filename, motif_directory, sample_name)

        if meme_motifs_filename:
            cmd_to_run += ' --meme_motifs_filename %s' % meme_motifs_filename

        if n_processes:
            cmd_to_run += ' --n_processes %d' % n_processes

        if temp_directory:
            cmd_to_run += ' --temp_directory %s' % temp_directory

        print cmd_to_run


        sb.call(cmd_to_run, shell=True)

    # input_args_motif = [target_motifs_filepaths_file,
    #                     genome_name,
    #                     '--output_directory',
    #                     motif_directory]
    #
    # if meme_motifs_filename:
    #     input_args_motif.extend(['--meme_motifs_filename', meme_motifs_filename])
    # if n_processes:
    #     input_args_motif.extend(['--n_processes', '{:d}'.format(n_processes)])
    # if temp_directory:
    #     input_args_motif.extend(['--temp_directory', temp_directory])
    #
    # motifs.main(input_args_motif)

    # CALL HAYSTACK MOTIFS


    # Ignore
    #
    # for sample_name in sample_names:
    #
    #     specific_regions_filename = glob.glob(os.path.join(output_directory,
    #                                              'HAYSTACK_HOTSPOTS',
    #                                              'SPECIFIC_REGIONS',
    #                                              'Regions_specific_for_%s*.bed' % sample_name))[0]
    #
    #     bg_regions_filename = glob.glob(os.path.join(output_directory,
    #                                                  'HAYSTACK_HOTSPOTS',
    #                                                  'SPECIFIC_REGIONS',
    #                                                  'Background_for_%s*.bed' % sample_name))[0]
    #
    #
    #     input_args_motif = [specific_regions_filename,
    #                         genome_name,
    #                         '--bed_bg_filename',
    #                         bg_regions_filename,
    #                         '--output_directory',
    #                         motif_directory,
    #                         '--name',
    #                         sample_name]
    #
    #
    #     input_args_motif = [genome_name,
    #                         '--output_directory',
    #                         motif_directory]
    #
    #     if meme_motifs_filename:
    #         input_args_motif.extend(['--meme_motifs_filename', meme_motifs_filename])
    #     if n_processes:
    #         input_args_motif.extend(['--n_processes', '{:d}'.format(n_processes)])
    #     if temp_directory:
    #         input_args_motif.extend(['--temp_directory', temp_directory])
    #     motifs.main(input_args_motif)

    if USE_GENE_EXPRESSION:

        sample_names_tf_activity_filename = os.path.join(output_directory,
                                                         'sample_names_tf_activity.txt')

        with open(sample_names_tf_activity_filename, 'w+') as outfile:
            for sample_name, gene_expression_filename in zip(sample_names,
                                                             gene_expression_filenames):
                outfile.write('%s\t%s\n' % (sample_name,
                                            gene_expression_filename))

        tf_activity_directory = os.path.join(output_directory,
                                             'HAYSTACK_TFs_ACTIVITY_PLANES')

        for sample_name in sample_names:

            # write tf activity  conf files


            # CALL HAYSTACK TF ACTIVITY
            motifs_output_folder = os.path.join(motif_directory,
                                                'HAYSTACK_MOTIFS_on_%s' % sample_name)

            if os.path.exists(motifs_output_folder):

                input_args_activity= [motifs_output_folder,
                                      sample_names_tf_activity_filename,
                                      sample_name,
                                      '--output_directory',
                                      tf_activity_directory]
                if motif_mapping_filename:
                    input_args_activity.extend(['--motif_mapping_filename',
                                                motif_mapping_filename])
                if plot_all:
                    input_args_activity.append(['--plot_all'])

                tf_activity_plane.main(input_args_activity)

if __name__ == '__main__':
    main()
