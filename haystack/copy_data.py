import os
from distutils.dir_util import copy_tree


def copy_haystack_data():

    data_root = os.environ['HOME']

    d_path = lambda x: (x, os.path.join(data_root, x))
    try:
       copy_tree(*d_path('haystack_data'))
    except:
       print "Cannot move data"

    print(os.listdir(os.path.join(data_root,'haystack_data')))

def main():
    print '\n[copy data]\n'
    copy_haystack_data()