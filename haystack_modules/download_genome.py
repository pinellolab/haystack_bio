import os
import sys
import  urllib2
import shutil as sh 
from bx.seq.twobit import TwoBitFile
import time


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



class Genome_2bit:

    def __init__(self,genome_2bit_file,verbose=False):
        self.genome=TwoBitFile(open(genome_2bit_file,'rb'))
        self.chr_len=dict()
        self.verbose=verbose

        for chr_id in self.genome.keys():
            self.chr_len[chr_id]=len(self.genome[chr_id])
        if verbose:
            print 'Genome initializated'
        
   
    def estimate_background(self):
        counting={'a':.0,'c':.0,'g':.0,'t':.0}
        all=0.0
        
        for chr_id in self.genome.keys():
            if self.verbose:
                start_time = time.time()
                print 'Counting on:',chr_id

            for nt in counting.keys():
                
                count_nt=self.genome[chr_id][:].lower().count(nt)
                counting[nt]+=count_nt
                all+=count_nt

            print 'elapsed:',time.time() - start_time
        
        if self.verbose:
            print counting

        for nt in counting.keys():
            counting[nt]/=all
        
        return counting


    def write_meme_background(self,filename):
        counting=self.estimate_background()
        with open(filename,'w+') as outfile:
            for nt in counting.keys():
                outfile.write('%s\t%2.4f\n' % (nt.upper(),counting[nt]))

    def write_chr_len(self,filename):
        with open(filename,'w+') as outfile:
            for chr_id in self.genome.keys():
                outfile.write('%s\t%s\n' % (chr_id,self.chr_len[chr_id]) )


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

    

if __name__ == '__main__':
    print '\n[H A Y S T A C K   G E N O M E   D O W L O A D E R]\n'

    if len(sys.argv) < 2:
        sys.exit('Example: download_genome hg19\n')
    else:
        haystack_path=which('haystack_motifs')
        if  haystack_path:
            download_path=os.path.dirname(haystack_path).replace('bin','genomes')
            download_genome(sys.argv[1],download_path)
        else:
            sys.exit('Haystack not properly installed.Try to reinstall it. \n\n')
