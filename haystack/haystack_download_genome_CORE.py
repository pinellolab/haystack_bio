import os
import sys
import urllib2
import shutil as sh
from bioutilities import Genome_2bit
from haystack_common import determine_path
import tarfile

HAYSTACK_VERSION = "0.5.0"


def copy_haystack_data():

    HAYSTACK_DEPENDENCIES_FOLDER = '%s/haystack_data' % os.environ['HOME']

    if not os.path.exists(HAYSTACK_DEPENDENCIES_FOLDER):
        sys.stdout.write('OK, creating the folder:%s' % HAYSTACK_DEPENDENCIES_FOLDER)
        os.makedirs(HAYSTACK_DEPENDENCIES_FOLDER)
    else:
        sys.stdout.write('\nI cannot create the folder!\nThe folder %s is not empty!' % HAYSTACK_DEPENDENCIES_FOLDER)

    d_path = lambda x: (x, os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, x))

    copy_tree(*d_path('haystack_data'))




def download_haystack_data(haystack_data_dir=None):

    if not haystack_data_dir:

        haystack_data_dir =os.environ['HOME']
    else:
        haystack_data_dir =haystack_data_dir

    try:
        url_path = "https://github.com/rfarouni/Haystack/archive/v0.5.0.tar.gz"
        data_url_origin = urllib2.urlopen(url_path)
        file_destination = os.path.join(haystack_data_dir,
                                        'haystack.tar.gz')

        with open(file_destination, 'wb') as file:
            sh.copyfileobj(data_url_origin, file)

        print 'Downloaded %s in %s:' % (url_path,
                                        filename)
    except IOError, e:
        print "Can't retrieve %r to %r: %s" % (url_path,
                                               filename, e)
        #return

    try:
        if (file_destination.endswith("tar.gz")):
            tar = tarfile.open(file_destination, "r:gz")
            subdir_and_files = [
                tarinfo for tarinfo in tar.getmembers()
                if tarinfo.name.startswith("Haystack-0.5.0/motif_databases") ]
            tar.extractall(haystack_data_dir,members=subdir_and_files)
            tar.close()
        print 'Extracted %s to %s:' % (file_destination, haystack_data_dir)
        try:
            os.remove(file_destination)
            os.rename(os.path.join(haystack_data_dir,'Haystack-0.5.0'),
                      os.path.join(haystack_data_dir,'haystack_data'))
        except:
            print "Cannot remove %s" % (file_destination)
    except:
        print "Bad tar file (from %r)" % (file_destination)
        #return


def download_genome(name):

    output_directory= determine_path('genomes')
    try:
        urlpath = "http://hgdownload.cse.ucsc.edu/goldenPath/%s/bigZips/%s.2bit" % (name, name)
        genome_url_origin = urllib2.urlopen(urlpath)
        genome_filename = os.path.join(output_directory, "%s.2bit" % name)

        if os.path.exists(genome_filename):
            print 'File %s exists, skipping download' % genome_filename
        else:
            print 'Downloding %s in %s...' % (urlpath, genome_filename)
            with open(genome_filename, 'wb') as genome_file_destination:
                sh.copyfileobj(genome_url_origin, genome_file_destination)

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


def main():
    print '\n[H A Y S T A C K   G E N O M E   D O W L O A D E R]\n'
    print 'Version %s\n' % HAYSTACK_VERSION

    if len(sys.argv) < 2:
        sys.exit('Example: haystack_download_genome hg19\n')
    else:
        download_genome(sys.argv[1])

