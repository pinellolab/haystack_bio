# -*- coding: utf-8 -*-
"""
Created on Fri Apr 22 15:16:12 2016

@author: lpinello
"""
import subprocess as sb
import sys
import logging
import os
from distutils.dir_util import copy_tree


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

def query_yes_no(question, default="yes"):
    valid = {"yes":True,   "y":True,  "ye":True,
             "no":False,     "n":False}
    if default == None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = raw_input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' "\
                             "(or 'y' or 'n').\n")


def query_yes_no(question, default="yes"):
    valid = {"yes":True,   "y":True,  "ye":True,
             "no":False,     "n":False}
    if default == None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    while True:
        sys.stdout.write(question + prompt)
        choice = raw_input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' "
                             "(or 'y' or 'n').\n")

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

