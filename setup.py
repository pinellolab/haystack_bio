#!/usr/bin/env python
"""Description: 
Setup script for Haystack -- Epigenetic Variability and Transcription Factor Motifs Analysis Pipeline
@status:  beta
@version: $Revision$
@author:  Luca Pinello
@contact: lpinello@jimmy.harvard.edu
"""
from __future__ import division, print_function
from setuptools import setup
from distutils.dir_util import copy_tree
import os
import subprocess as sb
import sys

def main():
    setup(
        version="0.5.0",
        name="haystack_bio",
        include_package_data=True,
        packages=["haystack"],
        package_dir={'haystack': 'haystack'},
        package_data={'haystack': ['./*']},
        entry_points={
            "console_scripts": ['haystack_pipeline = haystack.haystack_pipeline_CORE:main',
                                'haystack_hotspots =  haystack.haystack_hotspots:main',
                                'haystack_motifs = haystack.haystack_motifs_CORE:main',
                                'haystack_tf_activity_plane = haystack.haystack_tf_activity_plane_CORE:main',
                                'haystack_download_genome = haystack.haystack_download_genome_CORE:main', ]
        },
        description="Epigenetic Variability and Transcription Factor Motifs Analysis Pipeline",
        author='Luca Pinello',
        author_email='lpinello@jimmy.harvard.edu',
        url='http://github.com/lucapinello/Haystack',
        classifiers=[
            'Development Status :: 4 - Beta',
            'Environment :: Console',
            'Intended Audience :: Developers',
            'Intended Audience :: Science/Research',
            'License :: OSI Approved :: BSD License',
            'Operating System :: MacOS :: MacOS X',
            'Operating System :: POSIX',
            'Topic :: Scientific/Engineering :: Bio-Informatics',
            'Programming Language :: Python',
        ],
        install_requires=[]
    )

# TO INSTALL HAYSTACK DEPENDECIENS IN A CUSTOM LOCATION SET THE ENV VARIABLE: HAYSTACK_DEPENDENCIES_FOLDER
sys.stdout.write('\n\nInstalling dependencies...')

HAYSTACK_DEPENDENCIES_FOLDER = '%s/Haystack_dependencies' % os.environ['HOME']

BIN_FOLDER = os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, 'bin')

if not os.path.exists(HAYSTACK_DEPENDENCIES_FOLDER):
    sys.stdout.write('OK, creating the folder:%s' % HAYSTACK_DEPENDENCIES_FOLDER)
    os.makedirs(HAYSTACK_DEPENDENCIES_FOLDER)
    os.makedirs(BIN_FOLDER)
else:
    sys.stdout.write('\nI cannot create the folder!\nThe folder %s is not empty!' % HAYSTACK_DEPENDENCIES_FOLDER)

    try:
        os.makedirs(BIN_FOLDER)
    except:
        pass

d_path = lambda x: (x, os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, x))

copy_tree(*d_path('genomes'))
copy_tree(*d_path('gene_annotations'))
copy_tree(*d_path('motif_databases'))
copy_tree(*d_path('extra'))
copy_tree(*d_path('test_data'))
# fix permission so people can write in haystack dep folders

sb.call('chmod -R 777 %s' % os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, 'genomes'), shell=True)
sb.call('chmod -R 777 %s' % os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, 'gene_annotations'), shell=True)
sb.call('chmod -R 777 %s' % os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, 'motif_databases'), shell=True)
sb.call('chmod -R 777 %s' % os.path.join(HAYSTACK_DEPENDENCIES_FOLDER, 'test_data'), shell=True)

if __name__ == '__main__':
    main()

    sys.stdout.write('\n\nINSTALLATION COMPLETED, open a NEW terminal and enjoy HAYSTACK!')
