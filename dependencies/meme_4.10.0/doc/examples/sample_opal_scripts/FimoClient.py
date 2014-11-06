# This script submits a motif file and a FASTA file to the FIMO web service at NBCR
# and downloads the resulting output to the directory fimo_out.
# Based on the script pdb2pqrclient.pl included in the 2.0.0 Opal-Python
# toolkit, available at http://sourceforge.net/projects/opaltoolkit/ and 
# documented at http://nbcr.ucsd.edu/data/docs/opal/documentation.html

# Import the Opal libraries
from AppService_client import \
     AppServiceLocator, getAppMetadataRequest, launchJobRequest, \
     queryStatusRequest, getOutputsRequest, \
     launchJobBlockingRequest, getOutputAsBase64ByNameRequest
from AppService_types import ns0
from os import mkdir
from subprocess import call
from time import sleep
from urlparse import urlparse
from ZSI.TC import String


# Set the location of our service
# A list of NBCR services can be found at http://nbcr-222.ucsd.edu/opal2/dashboard?command=serviceList
serviceURL = "http://nbcr-222.ucsd.edu/opal2/services/FIMO_4.9.1"

# Instantiate a new service object to interact with.  Pass it the service location
appLocator = AppServiceLocator()
appServicePort = appLocator.getAppServicePort(serviceURL)
    
# Instantiate a new blocking job request
req = launchJobRequest()

# Set the command-line arguments for the job. Note that all
# the MEME Suite programs have a webservice program that invokes them.
#
# fimo_webservice [options] <motifs> <db seqs>
#
#  Options:
#    -upseqs <file>    uploaded sequences
#    -pvthresh <pv>    output p-value threshold
#    -norc             scan given strand only
#    -help             brief help message
# 
req._argList = "--upseqs crp0.fasta crp0.meme.xml"
# Alternatively, use the argList setting below to scan the 
# preloaded Saccaromyces cerevisiae genome.
#req._argList = "crp0.meme.xml saccharomyces_cerevisiae.na"

# Get contents of local files to be used as input
fastaFile = open("./crp0.fasta", "r")
fastaFileContents = fastaFile.read()
fastaFile.close()
motifFile = open("./crp0.meme.xml", "r")
motifFileContents = motifFile.read()
motifFile.close()

# Set name and content of remote input file.
# Two input files are used, the motif file
#and the FASTA file.
inputFiles = []
motifInputFile = ns0.InputFileType_Def('inputFile')
motifInputFile._name = 'crp0.meme.xml'
motifInputFile._contents = motifFileContents
inputFiles.append(motifInputFile)
fastaInputFile = ns0.InputFileType_Def('inputFile')
fastaInputFile._name = 'crp0.fasta'
fastaInputFile._contents = fastaFileContents
inputFiles.append(fastaInputFile)
req._inputFile = inputFiles

# Launch a non-blocking job
print "Launching non-blocking FIMO job"
resp = appServicePort.launchJob(req)
jobID = resp._jobID
print "Received Job ID:", jobID

# Poll for job status
status = resp._status
print "Polling job status"
while 1:
  # print current status
  print "Status:"
  print "\tCode:", status._code
  print "\tMessage:", status._message
  print "\tOutput Base URL:", status._baseURL

  if (status._code == 8) or (status._code == 4): # STATUS_DONE || STATUS_FAILED
    break

  print "Waiting 30 seconds"
  sleep(30)
  
  # Query job status
  status = appServicePort.queryStatus(queryStatusRequest(jobID))

if status._code == 8: # 8 = GramJob.STATUS_DONE
  print "\nDownloading Outputs (using wget):\n\n";
  output_dir = 'fimo_out'
  dir_count = sum(1 for _ in filter(None, urlparse(status._baseURL).path.split("/")))
  index_url = status._baseURL + '/index.html'
  call(["wget", "-r", "-nc", "-p", "-np", "-nH", "--cut-dirs=" + str(dir_count), "-P",  output_dir, index_url])

print "\nJob Complete\n\n";
