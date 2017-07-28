import os
from haystack_common import determine_path
import subprocess as sb

def run_testdata():

    print(os.getcwd())

    test_data_dir= determine_path("test_data")
    os.chdir(test_data_dir)
    print(os.getcwd())

    cmd= "haystack_pipeline samples_names_genes.txt hg19 --output_directory $HOME/OUTPUT --bin_size 200 --chrom_exclude 'chr(?!21)'"

    try:
        sb.call(cmd, shell=True)
    except:
       print "Cannot run test"

def main():
    print '\n[run test]\n'
    run_testdata()