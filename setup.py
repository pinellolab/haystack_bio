#!/usr/bin/env python
"""Description: 
Setup script for haystack_bio -- Epigenetic Variability and Transcription Factor Motifs Analysis Pipeline
@status:  beta
@version: $Revision$
@author:  Luca Pinello, Rick Farouni
@contact: lpinello@mgh.harvard.edu
"""
from setuptools import setup

from haystack.haystack_common import check_required_packages


def main():

    setup(
        version="0.5.3",
        name="haystack_bio",
        include_package_data=True,
        packages=["haystack"],
        package_dir={'haystack': 'haystack'},
        entry_points={
            "console_scripts": ['haystack_pipeline = haystack.run_pipeline:main',
                                'haystack_hotspots =  haystack.find_hotspots:main',
                                'haystack_motifs = haystack.find_motifs:main',
                                'haystack_tf_activity_plane = haystack.generate_tf_activity_plane:main',
                                'haystack_download_genome = haystack.download_genome:main',
                                'haystack_run_test = haystack.haystack_common:run_testdata']
        },
        description="Epigenetic Variability and Transcription Factor Motifs Analysis Pipeline",
        maintainer= 'Luca Pinello , Rick Farouni',
        maintainer_email='lpinello@mgh.harvard.edu, tfarouni@mgh.harvard.edu',
        author='Luca Pinello, Rick Farouni',
        author_email='lpinello@mgh.harvard.edu, tfarouni@mgh.harvard.edu',
        url='https://github.com/pinellolab/haystack_bio',
        classifiers=[
            'Development Status :: 5 - Stable',
            'Environment :: Console',
            'Intended Audience :: Developers',
            'Intended Audience :: Science/Research',
            'License :: OSI Approved :: GNU Affero General Public License v3',
            'Operating System :: MacOS :: MacOS X',
            'Operating System :: POSIX:: Linux',
            'Topic :: Scientific/Engineering :: Bio-Informatics',
            'Programming Language :: Python:: 2.7',
        ],
        install_requires=[
            'numpy>=1.13.3',
            'pandas>=0.21.0',
            'matplotlib>=2.1.0',
            'scipy>=1.0.0',
            'jinja2>=2.9.6',
            'tqdm>=4.19.4',
            'weblogo>=3.5.0',
            'bx-python>=0.7.3']
    )

if __name__ == '__main__':
    print("Checking Required Packages")
    check_required_packages()
    print("Installing Package")
    main()
