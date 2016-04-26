# -*- coding: utf-8 -*-
"""
Created on Fri Apr 22 15:16:12 2016

@author: lpinello
"""


import logging
import sys
import os
import platform 

__version__ = "0.3.0"
HAYSTACK_VERSION=__version__ 


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


CURRENT_PLATFORM=platform.system().split('_')[0]


import cPickle as cp

#Use the dependencies folder created and update the path
_ROOT = os.path.abspath(os.path.dirname(__file__))
system_env=cp.load( open( os.path.join(_ROOT,'system_env.pickle')))
os.environ['PATH']=system_env['PATH']


def determine_path(folder):
    return os.path.join(system_env['HAYSTACK_DEPENDENCIES_PATH'],folder)