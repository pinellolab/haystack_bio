# -*- coding: utf-8 -*-

import subprocess as sb
import sys
import logging
from distutils.dir_util import copy_tree
import os
from tqdm import tqdm

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


__version__ = "0.5.3"
HAYSTACK_VERSION=__version__

def check_file(filename):
    try:
        with open(filename): pass
    except IOError:
        error('I cannot open the file:'+filename)
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

def check_required_packages():

    if which('bedtools') is None:
        error('Haystack requires bedtools.'
              ' Please install using bioconda')
        sys.exit(1)

    if which('bedGraphToBigWig') is None:
        info(
            ' Haystack requires bedGraphToBigWig.'
            ' Please install.')
        sys.exit(1)

    if which('sambamba') is None:
        info(
            'Haystack requires sambamba.'
            ' Please install.')
        sys.exit(1)

    if which('bigWigAverageOverBed') is None:
        info(
            'Haystack requires bigWigAverageOverBed. '
            'Please install.')
        sys.exit(1)

    if which('meme') is None:
        info(
            'Haystack requires meme. '
            'Please install.')
        sys.exit(1)

    if which('java') is None:
        info(
            'Haystack requires java. '
            'Please install.')
        sys.exit(1)





def determine_path(folder=''):

    _ROOT = os.path.abspath(os.path.dirname(__file__))
    _ROOT = os.path.join(_ROOT,'haystack_data')

    print(_ROOT)

    # if os.environ.has_key('CONDA_PREFIX'): #we check if we are in an conda env
    # #_ROOT = '%s/haystack_data' % os.environ['HOME']
    #     _ROOT=os.environ['CONDA_PREFIX']
    # else:
    #     _ROOT =which('python').replace( '/bin/python', '') #we are in the main conda env
    #
    # _ROOT=os.path.join(_ROOT,'share/haystack_data')
    return os.path.join(_ROOT,folder)


def run_testdata():

    test_data_dir= determine_path("test_data")
    os.chdir(test_data_dir)

    cmd= "haystack_pipeline samples_names.txt hg19 --output_directory $HOME/haystack_test_output --blacklist hg19 --chrom_exclude 'chr(?!21)'"

    try:
        info("running test")
        sb.call(cmd, shell=True)
        info("Test completed successfully")
    except:
        error("Cannot run test")

def copy_haystack_data():
    info("copying data")
    data_root = determine_path()


    d_path = lambda x: (x, os.path.join(data_root, x))
    try:
        copy_tree(*d_path('test_data'))
        copy_tree(*d_path('extra'))
        copy_tree(*d_path('genomes'))
        copy_tree(*d_path('gene_annotations'))
        copy_tree(*d_path('motif_databases'))
        print(os.listdir(os.path.join(data_root)))
    except:
        info("Cannot move data")



#taken from tqdm website
class TqdmUpTo(tqdm):
    """Provides `update_to(n)` which uses `tqdm.update(delta_n)`."""
    def update_to(self, b=1, bsize=1, tsize=None):
        """
        b  : int, optional
            Number of blocks transferred so far [default: 1].
        bsize  : int, optional
            Size of each block (in tqdm units) [default: 1].
        tsize  : int, optional
            Total size (in tqdm units). If [default: None] remains unchanged.
        """
        if tsize is not None:
            self.total = tsize
        self.update(b * bsize - self.n)  # will also set self.n = b * bsize


def check_md5sum(genome_filename, genome_name):

    import hashlib

    def hash_bytestr_iter(bytesiter, hasher, ashexstr=True):
        # taken from https://stackoverflow.com/a/3431835

        for block in bytesiter:
            hasher.update(block)
        return (hasher.hexdigest() if ashexstr else hasher.digest())

    def file_as_blockiter(afile, blocksize=65536):
        # taken from https://stackoverflow.com/a/3431835

        with afile:
            block = afile.read(blocksize)
            while len(block) > 0:
                yield block
                block = afile.read(blocksize)


    md5_dic = {"hg38": "dcc3ea27079aa6dc3f9deccd7275e0f8",
               "hg19": "bcdbfbe9da62f19bee88b74dabef8cd3",
               "hg18": "05e8d31e39545273914397ad6204448e",
               "mm10": "fcfcc276799031793a513e2e9c07adad",
               "mm9": "e47354d24b9d95e832c337d42b9f8f71",
               "ce10": "3d0bab4bc255fc5b3276a476e13d230c",
               "sacCer3": "880201a7d1ec95c0185b0b4783c80411",
               "sacCer2": "ed3b980b89a22f7d869091bee874d4b5",
               "dm6": "62f44f8cbf76c78ce923cb6d87559963",
               "dm3": "4ec509b470010829be44ed8e7bfd7f57"}


    if genome_name in md5_dic.keys():

        md5_source = md5_dic[genome_name]

        md5_hash_returned = hash_bytestr_iter(file_as_blockiter(open(genome_filename, 'rb')),
                                              hashlib.md5())

        check_flag = (md5_source == md5_hash_returned)

        if check_flag:
            info("MD5 verification Succeeded!")
        else:
            info("MD5 verification failed!.")

    else:
        info( 'Cannot verify MD5 sum. The MD5 hash for %s is not found in the internal saved list' %genome_name )
        info(' '.join(md5_dic.keys()))
        check_flag= True

    return check_flag

def initialize_genome(genome_name):

    from bioutilities import Genome_2bit
    import urllib

    info('Initializing Genome:%s' % genome_name)
    genome_directory = determine_path('genomes')
    info('genome_directory: %s' % genome_directory)
    genome_filename = os.path.join(genome_directory, "%s.2bit" % genome_name)
    chr_len_filename = os.path.join(genome_directory, "%s_chr_lengths.txt" % genome_name)
    meme_bg_filename = os.path.join(genome_directory, "%s_meme_bg" % genome_name)

    download_genome = True
    if os.path.exists(genome_filename):

        try:
            Genome_2bit(genome_filename, verbose=True)

            md5_check_flag = check_md5sum(genome_filename, genome_name)

            if md5_check_flag:
                download_genome = False
                info('File %s exists. Skipping genome download' % genome_filename)

            else:
                download_genome = True
        except:
            download_genome = True
            error("Unable to check MD5 sum. Downloading genome.")


    if download_genome:
        info('Sorry I need the genome file to perform the analysis. Downloading...')
        urlpath = "http://hgdownload.cse.ucsc.edu/goldenPath/%s/bigZips/%s.2bit" % (genome_name, genome_name)
        info('Downloading %s in %s...' % (urlpath, genome_filename))
        try:
            with TqdmUpTo(unit='B', unit_scale=True, mininterval=30, miniters=1, desc=urlpath.split('/')[-1]) as t:
                urllib.urlretrieve(urlpath,
                                   filename=genome_filename,
                                   reporthook=t.update_to,
                                   data=None)

            info('Downloaded %s in %s:' % (urlpath, genome_filename))
        except IOError, e:
                    error("Can't retrieve %r to %r: %s" % (urlpath, genome_filename, e))
                    info('Sorry I need the genome file to perform the analysis. Exiting...')
                    sys.exit(1)

    check_file(genome_filename)
    genome = Genome_2bit(genome_filename, verbose=True)

    if not os.path.exists(chr_len_filename):
            info('Extracting chromosome lengths')
            genome.write_chr_len(chr_len_filename)
            info('Done!')
    else:
        info('File %s exists, skipping generation' % chr_len_filename)

    if not os.path.exists(meme_bg_filename):
        info('Calculating nucleotide frequencies....')
        genome.write_meme_background(meme_bg_filename)
        info('Done!')
    else:
        info('File %s exists, skipping generation' % meme_bg_filename)

    check_file(chr_len_filename)
    check_file(meme_bg_filename)

    info('Sorting chromosome lengths file....')

    cmd = ' sort -k1,1 -k2,2n  "%s" -o  "%s" ' % (chr_len_filename,
                                                  chr_len_filename)

    sb.call(cmd, shell=True)


    return genome, chr_len_filename, meme_bg_filename
