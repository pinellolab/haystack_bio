#!/usr/bin/env python
"""Description: 
Setup script for haystack_bio -- Epigenetic Variability and Transcription Factor Motifs Analysis Pipeline
@status:  beta
@version: $Revision$
@author:  Luca Pinello
@contact: lpinello@jimmy.harvard.edu
"""
from setuptools import setup

from haystack.haystack_common import check_required_packages


def main():

    setup(
        version="0.5.1",
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
        maintainer_email='lpinello@jimmy.harvard.edu, tfarouni@mgh.harvard.edu',
        author='Luca Pinello, Rick Farouni',
        author_email='lpinello@jimmy.harvard.edu, tfarouni@mgh.harvard.edu',
        url='http://github.com/lucapinello/Haystack',
        classifiers=[
            'Development Status :: 5 - Beta',
            'Environment :: Console',
            'Intended Audience :: Developers',
            'Intended Audience :: Science/Research',
            'License :: OSI Approved :: BSD License',
            'Operating System :: MacOS :: MacOS X',
            'Operating System :: POSIX',
            'Topic :: Scientific/Engineering :: Bio-Informatics',
            'Programming Language :: Python',
        ],
        install_requires=[
            'numpy>=1.12.1',
            'pandas>=0.13.1',
            'matplotlib>=1.3.1',
            'scipy>=0.13.3',
            'jinja2>=2.7.3',
            'pybedtools>=0.7.10',
            'tqdm>=4.15.0',
            'weblogo>=3.5.0',
            'bx-python>=0.7.3']
    )

if __name__ == '__main__':
    print("Checking Required Packages")
    check_required_packages()
    print("Installing Package")
    main()
