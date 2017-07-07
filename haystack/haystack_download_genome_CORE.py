import os
import sys
import  urllib2
import shutil as sh
from bioutilities import Genome_2bit
from haystack_common import determine_path, HAYSTACK_VERSION


def download_genome(name, output_directory=None):

    urlpath = "http://hgdownload.cse.ucsc.edu/goldenPath/%s/bigZips/%s.2bit" % (name, name)
    genome_url_origin = urllib2.urlopen(urlpath)

    genome_filename=os.path.join(output_directory, "%s.2bit" % name)

    print 'Downloding %s in %s...' %(urlpath,genome_filename)
    
    if os.path.exists(genome_filename):
        print 'File %s exists, skipping download' % genome_filename
    else:

        with open(genome_filename, 'wb') as genome_file_destination:
            sh.copyfileobj(genome_url_origin, genome_file_destination)

        print 'Downloded %s in %s:' %(urlpath,genome_filename)

    g=Genome_2bit(genome_filename,verbose=True)

    chr_len_filename=os.path.join(output_directory, "%s_chr_lengths.txt" % name)
    if not os.path.exists(chr_len_filename):
        print 'Extracting chromosome lengths'
        g.write_chr_len(chr_len_filename)
        print 'Done!'
    else:
        print 'File %s exists, skipping generation' % chr_len_filename

    meme_bg_filename=os.path.join(output_directory, "%s_meme_bg" % name)
    if not os.path.exists(meme_bg_filename):
        print 'Calculating nucleotide frequencies....'
        g.write_meme_background(meme_bg_filename)
        print 'Done!'
    else:
        print 'File %s exists, skipping generation' % meme_bg_filename

    

def main():
    print '\n[H A Y S T A C K   G E N O M E   D O W L O A D E R]\n'
    print 'Version %s\n' % HAYSTACK_VERSION

    if len(sys.argv) < 2:
        sys.exit('Example: haystack_download_genome hg19\n')
    else:
        download_path=determine_path('genomes')
        download_genome(sys.argv[1],download_path)
        