import os
import sys
from distutils.dir_util import copy_tree


def copy_haystack_data():

    HAYSTACK_DEPENDENCIES_FOLDER = '%s/haystack_data' % os.environ['HOME']

    print(HAYSTACK_DEPENDENCIES_FOLDER )
    print(os.getcwd())

    if not os.path.exists(HAYSTACK_DEPENDENCIES_FOLDER):
        sys.stdout.write('OK, creating the folder:%s' % HAYSTACK_DEPENDENCIES_FOLDER)
        os.makedirs(HAYSTACK_DEPENDENCIES_FOLDER)
    else:
        sys.stdout.write('\nI cannot create the folder!\nThe folder %s is not empty!' % HAYSTACK_DEPENDENCIES_FOLDER)

    print(os.listdir(HAYSTACK_DEPENDENCIES_FOLDER ))

    d_path = lambda x: (x, os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, x))

    copy_tree(*d_path('haystack_data'))

    print(os.listdir(HAYSTACK_DEPENDENCIES_FOLDER ))


def main():
    print '\n[copy data]\n'
    copy_haystack_data()