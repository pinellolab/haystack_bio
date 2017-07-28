import os
from distutils.dir_util import copy_tree
from haystack_common import determine_path

def copy_haystack_data():

    data_root = determine_path()
    d_path = lambda x: (x, os.path.join(data_root, x))
    try:
        os.chdir('haystack_data')
        copy_tree(*d_path('test_data'))
        copy_tree(*d_path('extra'))
        copy_tree(*d_path('genomes'))
        copy_tree(*d_path('gene_annotations'))
        copy_tree(*d_path('motif_databases'))
    except:
       print "Cannot move data"

    print(os.listdir(os.path.join(data_root)))

    print(os.getcwd())

def main():
    print '\n[copy data]\n'
    copy_haystack_data()