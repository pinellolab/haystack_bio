import os
import sys
import urllib
from tqdm import tqdm
from bioutilities import Genome_2bit
from haystack_common import determine_path, query_yes_no
import argparse

HAYSTACK_VERSION = "0.5.0"

def get_args_download_genome():
    # mandatory
    parser = argparse.ArgumentParser(description='download_genome parameters')
    parser.add_argument('name', type=str,
                        help='genome name. Example: haystack_download_genome hg19.')

    # optional
    parser.add_argument('--yes', help='answer yes to download prompt', action='store_true')

    return parser

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


def download_genome(name, answer):
    if answer or query_yes_no('Should I download it for you?'):
        output_directory= determine_path('genomes')
        print 'genome_directory: %s' % output_directory
        try:
            genome_filename = os.path.join(output_directory, "%s.2bit" % name)
            if os.path.exists(genome_filename):
                print 'File %s exists, skipping download' % genome_filename
            else:
                urlpath = "http://hgdownload.cse.ucsc.edu/goldenPath/%s/bigZips/%s.2bit" % (name, name)
                print 'Downloading %s in %s...' % (urlpath, genome_filename)

                with TqdmUpTo(unit='B', unit_scale=True, mininterval=30, miniters=1, desc=url.split('/')[-1]) as t:
                    urllib.urlretrieve(url,
                                       filename=genome_filename,
                                       reporthook=t.update_to,
                                       data=None)

                print 'Downloaded %s in %s:' % (urlpath, genome_filename)

        except IOError, e:
                print "Can't retrieve %r to %r: %s" % (urlpath, genome_filename, e)
                return
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
    else:
        print('Sorry I need the genome file to perform the analysis. Exiting...')
        sys.exit(1)

def main(input_args=None):

    print '\n[H A Y S T A C K   G E N O M E   D O W L O A D E R]\n'
    print 'Version %s\n' % HAYSTACK_VERSION

    parser = get_args_download_genome()
    args = parser.parse_args(input_args)
    download_genome(args.name, args.yes)

if __name__ == '__main__':
    main()
    sys.exit(0)