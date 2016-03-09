import os
import time
import pickle as cp
import glob
import subprocess as sb
import sys
import platform 
import shutil
from distutils.dir_util import copy_tree
from os.path import expanduser


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

def check_installation(filename,tool_name,verbose=False):
    if os.path.isfile(filename):
        if verbose:
            print('%s is/was succesfully installed ' % tool_name)
        return True
    else:
        if verbose:
            print 'Sorry I cannot install %s for you, check you have all the required dependencies and and try again to install HAYSTACK.' % tool_name
        return False
    


def check_fimo():
            if not check_installation(os.path.join(BIN_FOLDER,'fimo'),'FIMO',verbose=False):
                print '\nHAYSTACK requires to install FIMO from the MEME suite(4.9.1): http://ebi.edu.au/ftp/software/MEME/index.html'
                print('Please be patient!')
                os.chdir('dependencies/meme_4.9.1/')

                cmd_cfg='./configure --prefix=%s --enable-build-libxml2 --enable-build-libxslt ' % INSTALLATION_PATH
                if CURRENT_PLATFORM=='CYGWIN':
                    cmd_cfg+=' --build=x86_64-cygwin'

                print cmd_cfg    
                sb.call(cmd_cfg,shell=True)
                #sb.call('make clean',shell=True)
                sb.call('make && make install',shell=True)
                sb.call('make clean',shell=True)
                os.chdir('..')
                os.chdir('..')   
                #installa fimo
                print('FIMO should be installed (please check the output)')
            
    	    if not check_installation(os.path.join(BIN_FOLDER,'fimo'),'FIMO'):
	    	sys.exit(1)

            
def check_samtools():
            if not check_installation(os.path.join(BIN_FOLDER,'samtools'),'SAMTOOLS',verbose=False):
                print '\nHAYSTACK requires to install SAMTOOLS 0.1.19:http://sourceforge.net/projects/samtools/files/samtools/0.1.19/'
                print('Please be patient!')
                os.chdir('dependencies/samtools-0.1.19/')
                sb.call('make clean',shell=True)
                if CURRENT_PLATFORM=='CYGWIN':
                    sb.call('make --makefile=Makefile.cygwin',shell=True)
                else:
                    sb.call('make',shell=True)
                try:
                    shutil.copy2('samtools',BIN_FOLDER)
                except:
                    pass
                sb.call('make clean',shell=True)
                os.chdir('..')
                os.chdir('..')         
                
            if not check_installation(os.path.join(BIN_FOLDER,'samtools'),'SAMTOOLS'):
            	sys.exit(1)
            
                
def check_bedtools():
        
        if not check_installation(os.path.join(BIN_FOLDER,'bedtools'),'BEDTOOLS',verbose=False):
            print '\nHAYSTACK requires to install BEDTOOLS:https://github.com/arq5x/bedtools2/releases/tag/v2.20.1'
            print('Please be patient!')
            os.chdir('dependencies/bedtools2-2.20.1/')
            sb.call('make clean',shell=True)
            sb.call('make',shell=True)
            for filename in glob.glob(os.path.join('bin', '*')):
                shutil.copy(filename, BIN_FOLDER)
            sb.call('make clean',shell=True)    
            os.chdir('..')
            os.chdir('..')
            
        if not check_installation(os.path.join(BIN_FOLDER,'bedtools'),'BEDTOOLS'):
            sys.exit(1)

            
def check_ghostscript():
        

        if (not check_installation(os.path.join(BIN_FOLDER,'gs'),'Ghostscript',verbose=False) and CURRENT_PLATFORM=='Linux') or ( CURRENT_PLATFORM=='Darwin' and which('gs') is None):
            print '\nHAYSTACK requires to install Ghostscript 9.14'
            
            if CURRENT_PLATFORM=='Linux':
                print('Ok be patient!')
                shutil.copy2('dependencies/Linux/gs',BIN_FOLDER)
                time.sleep(3) # we need some time to allow the NFS to sync the file... 		
		sb.call('chmod +x %s' % os.path.join(BIN_FOLDER,'gs'),shell=True)
               
                #if not check_installation(os.path.join(BIN_FOLDER,'gs'),'Ghostscript'):
                #    sys.exit(1)
 
            elif CURRENT_PLATFORM=='Darwin': 
                print('Ok launching the installer for you!')
                print 'To install Ghostscript I need admin privileges.'
                sb.call('sudo installer -pkg dependencies/Darwin/Ghostscript-9.14.pkg -target /',shell=True)

            elif CURRENT_PLATFORM=='CYGWIN':
               print 'You need to install manually Ghostscript!' 
        
CURRENT_PLATFORM=platform.system().split('_')[0]

if CURRENT_PLATFORM not in  ['Linux','Darwin','CYGWIN'] and platform.architecture()!='64bit':
    print 'Sorry your platform is not supported\n Haystack is supported only on 64bit versions of Linux or OSX '
    sys.exit(1)
    
try: 
    INSTALLATION_PATH=sys.argv[1]
except:
    INSTALLATION_PATH='%s/HAYSTACK' % os.environ['HOME']

BIN_FOLDER=os.path.join(INSTALLATION_PATH,'bin')
    
if query_yes_no('I will install HAYSTACK in:%s \n\nIs that ok?' % INSTALLATION_PATH):    
   
    if not os.path.exists(INSTALLATION_PATH):
        print 'OK, creating the folder:%s' % INSTALLATION_PATH
        os.makedirs(INSTALLATION_PATH)
        os.makedirs(BIN_FOLDER)
    else:
        print '\nI cannot create the folder!\nThe folder %s is not empty!' % INSTALLATION_PATH
        if query_yes_no('\nCan I overwrite its content? \nWARNING: all the files inside will be overwritten!'):
            #shutil.rmtree(INSTALLATION_PATH)
            #os.makedirs(INSTALLATION_PATH)
            try:
                os.makedirs(BIN_FOLDER)
            except:
                pass
        else:
            print '\nOK, install HAYSTACK in a different PATH running again this script with: \n\npython setup.py YOUR_PATH'
            sys.exit(1)
        
else:
    print '\nOK, to install HAYSTACK in a different PATH just run this script again with: \n\npython setup.py YOUR_PATH'
    sys.exit(1)

print 'CHECKING DEPENDENCIES...'
check_ghostscript()
check_fimo()
check_bedtools()
check_samtools()

#coping data in place
print '\nCopying data in place...'
d_path = lambda x: (x,os.path.join(INSTALLATION_PATH,x))

s_path=lambda x: os.path.join(INSTALLATION_PATH,x)

copy_tree(*d_path('genomes'))
copy_tree(*d_path('gene_annotations'))
copy_tree(*d_path('motif_databases'))
copy_tree(*d_path('extra'))


if CURRENT_PLATFORM=='Linux':
    src='precompiled_binary/Linux/'

elif CURRENT_PLATFORM=='CYGWIN':
    src='precompiled_binary/Cygwin/'

elif CURRENT_PLATFORM=='Darwin':
    src='precompiled_binary/Darwin/'
    haystack_hotspot_src_folder=os.path.join(src,'haystack_hotspots_core')
    haystack_hotspot_dst_folder=os.path.join(BIN_FOLDER,'haystack_hotspots_core')
    haystack_hotspot_binary=os.path.join(haystack_hotspot_dst_folder,'haystack_hotspots')
    
    
    haystack_tf_src_folder=os.path.join(src,'haystack_tf_activity_plane_core')
    haystack_tf_dst_folder=os.path.join(BIN_FOLDER,'haystack_tf_actvity_plane_core')
    haystack_tf_binary=os.path.join(haystack_tf_dst_folder,'haystack_tf_activity_plane')
    
    try:
        shutil.rmtree(haystack_hotspot_dst_folder)
    except:
        pass
    
    try:
        shutil.rmtree(haystack_tf_dst_folder)
    except:
        pass
    
    shutil.copytree(haystack_hotspot_src_folder, haystack_hotspot_dst_folder)
    link_cmd='ln -s %s %s' % (haystack_hotspot_binary, os.path.join(BIN_FOLDER,'haystack_hotspots'))
    #print link_cmd
    sb.call(link_cmd,shell=True)
    
    shutil.copytree(haystack_tf_src_folder, haystack_tf_dst_folder)
    link_cmd='ln -s %s %s' % (haystack_tf_binary, os.path.join(BIN_FOLDER,'haystack_tf_activity_plane'))
    #print link_cmd
    sb.call(link_cmd,shell=True)

dest=BIN_FOLDER
src_files = os.listdir(src)
for file_name in src_files:
    full_file_name = os.path.join(src, file_name)
    if (os.path.isfile(full_file_name)):
        shutil.copy(full_file_name, dest)    

#ADD HAYSTACK TO PATH
home = expanduser("~")
shell=os.environ["SHELL"].split('/')[-1]

shell_profile=None
line_to_add=None    

if shell=='bash':
    if CURRENT_PLATFORM=='Darwin': 
        shell_profile='.bash_profile'
    else:
        shell_profile='.bashrc'
        
elif shell=='sh' or shell=='ksh':
    shell_profile='.profile'
    
elif shell=='tcsh':
    shell_profile='.tcshrc'

elif shell=='csh':
    shell_profile='.cshrc'
    
else:
	shell='bash'

if shell in ['bash', 'sh','ksh']:
    line_to_add='export PATH=$PATH:%s' % BIN_FOLDER
elif shell in ['tcsh','csh']:
    line_to_add= 'set path = ($path %s )' % BIN_FOLDER
    
cmd_add_path="echo '%s'  >> ~/%s" % (line_to_add,shell_profile)

if not os.path.exists(os.path.join(home,shell_profile)) or not line_to_add in  open(os.path.join(home,shell_profile)).read():
    if query_yes_no('You need to add  %s to your PATH variable.\n\nShould I do for you?' % BIN_FOLDER):
        if shell_profile is None:
            print 'I cannot determine automatically the shell you are using. Please add the folder %s to your PATH manually!' % BIN_FOLDER
        print '\nExecuting:%s' % cmd_add_path
        sb.call(cmd_add_path,shell=True)
        sb.call(line_to_add,shell=True)
        print '\n\nINSTALLATION COMPLETED, open a NEW terminal and enjoy HAYSTACK!'
    else:
        print '\nOK.\n\nNOTE: to run HAYSTACK all the files in %s should be in your PATH' % BIN_FOLDER
else:
    print '\n\nINSTALLATION COMPLETED, open a NEW terminal and enjoy HAYSTACK!'    

#COPY current env for subprocess
os.environ['PATH']=('%s:' % BIN_FOLDER) +os.environ['PATH'] #here the order is important
system_env=os.environ
cp.dump(system_env,open(os.path.join(dest,'system_env.pickle'),'w+'))

            
