#!/usr/bin/env python
"""Description: 
Setup script for Haystack -- Epigenetic Variability and Transcription Factor Motifs Analysis Pipeline
@status:  beta
@version: $Revision$
@author:  Luca Pinello
@contact: lpinello@jimmy.harvard.edu
"""

import re
from setuptools import setup, Extension
import os
import subprocess as sb
import sys
import platform 
import shutil
from distutils.dir_util import copy_tree


#TO INSTALL HAYSTACK DEPENDECIENS IN A CUSTOM LOCATION SET THE ENV VARIABLE: HAYSTACK_DEPENDENCIES_FOLDER
if os.environ.get('HAYSTACK_DEPENDENCIES_FOLDER'):
	INSTALLATION_PATH=os.environ.get('HAYSTACK_DEPENDENCIES_FOLDER')
else:
	INSTALLATION_PATH='%s/Haystack_dependencies' % os.environ['HOME']

BIN_FOLDER=os.path.join(INSTALLATION_PATH,'bin')

def main():

	version = re.search(
    	'^__version__\s*=\s*"(.*)"',
    	open('haystack/haystack_common.py').read(),
    	re.M
    	).group(1)



	setup( 
		   version=version,
          name = "haystack_bio",
          include_package_data = True,
    	   packages = ["haystack"],
    	   package_dir={'haystack': 'haystack'},
          package_data={'haystack': ['./*']},     
    	   entry_points = {
        	"console_scripts": ['haystack_pipeline = haystack.haystack_pipeline_CORE:main',
          'haystack_hotspots =  haystack.haystack_hotspots_CORE:main',
          'haystack_motifs = haystack.haystack_motifs_CORE:main',
          'haystack_tf_activity_plane = haystack.haystack_tf_activity_plane_CORE:main',
          'haystack_download_genome = haystack.haystack_download_genome_CORE:main',]
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
          install_requires=[],

          )


def install_dependencies(CURRENT_PLATFORM):


   if CURRENT_PLATFORM not in  ['Linux','Darwin'] and platform.architecture()!='64bit':
		sys.stdout.write('Sorry your platform is not supported\n Haystack is supported only on 64bit versions of Linux or OSX ')
		sys.exit(1)
   
   if not os.path.exists(INSTALLATION_PATH):
		sys.stdout.write ('OK, creating the folder:%s' % INSTALLATION_PATH)
		os.makedirs(INSTALLATION_PATH)
		os.makedirs(BIN_FOLDER)
   else:
		sys.stdout.write ('\nI cannot create the folder!\nThe folder %s is not empty!' % INSTALLATION_PATH)

		try:
			os.makedirs(BIN_FOLDER)
		except:
			pass

def copy_data(CURRENT_PLATFORM):
    

    #coping data in place
    d_path = lambda x: (x,os.path.join(INSTALLATION_PATH,x))
    
    s_path=lambda x: os.path.join(INSTALLATION_PATH,x)
    
    copy_tree(*d_path('genomes'))
    copy_tree(*d_path('gene_annotations'))
    copy_tree(*d_path('motif_databases'))
    copy_tree(*d_path('extra'))
    
    
    if CURRENT_PLATFORM=='Linux':
        src='precompiled_binary/Linux/'
    
    elif CURRENT_PLATFORM=='Darwin':
        src='precompiled_binary/Darwin/'

    dest=BIN_FOLDER
    src_files = os.listdir(src)
    for file_name in src_files:
        full_file_name = os.path.join(src, file_name)
        if (os.path.isfile(full_file_name)):
            shutil.copy(full_file_name, dest)      
    
    
    #fix permission so people can write in haystack dep folders
    
    sb.call('chmod -R 777 %s' % os.path.join(INSTALLATION_PATH,'genomes'),shell=True)
    sb.call('chmod -R 777 %s' % os.path.join(INSTALLATION_PATH,'gene_annotations'),shell=True)
    sb.call('chmod -R 777 %s' % os.path.join(INSTALLATION_PATH,'motif_databases'),shell=True)

	

if __name__ == '__main__':
    if sys.argv[1]=='install':
        
        CURRENT_PLATFORM=platform.system().split('_')[0]
        
        sys.stdout.write ('\n\nInstalling dependencies...')
        install_dependencies(CURRENT_PLATFORM)
        copy_data(CURRENT_PLATFORM)
        sys.stdout.write ('\nInstalling Python package installed')
    main()
    
    if sys.argv[1]=='install':
        sys.stdout.write ('\n\nINSTALLATION COMPLETED, open a NEW terminal and enjoy HAYSTACK!')







