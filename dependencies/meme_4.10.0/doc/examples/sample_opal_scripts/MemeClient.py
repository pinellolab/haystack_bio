# This script submits a FASTA file to the MEME web service at NBCR
# and downloads the resulting output to the directory meme_out.
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
serviceURL = "http://nbcr-222.ucsd.edu/opal2/services/MEME_4.9.1"

# Instantiate a new service object to interact with.  Pass it the service location
appLocator = AppServiceLocator()
appServicePort = appLocator.getAppServicePort(serviceURL)
    
# Instantiate a new non-blocking job request
req = launchJobRequest()

# Set the command-line arguments for the job.
#
#meme_webservice [options] <sequences>
#
#  Options: 
#    -alpha [dna|protein]      The alphabet of the sequences. Default: dna
#    -mod [oops|zoops|anr]     The expected number of motif repeats per sequence.
#                              Default: zoops
#    -nmotifs <count>          The number of motif to find. Default: 3
#    -minw <width>             The minimum width of the motif. Default: 6
#    -maxw <width>             The maximum width of the motif. Default: 50
#    -minsites <num>           The minimum number of sites per motif.
#    -maxsites <num>           The maximum number of sites per motif.
#    -bfile <file>             A background file.
#    -neg <file>               A negative sequences set, for generating PSPs.
#    -norevcomp                Restrict sites to only given strand.
#    -pal                      Only find palindromes.
#    -help                     Show this brief help message.
#
# Use DNA alphabet, and find 3 motifs.
req._argList = "-alpha dna -nmotifs 3 -mod zoops crp0.fasta";

# Get contents of local file to be used as input
fastaFile = open("./crp0.fasta", "r")
fastaFileContents = fastaFile.read()
fastaFile.close()

# Set name and content of remote input file.
# Only one input file is used, the FASTA file.
inputFiles = []
inputFile = ns0.InputFileType_Def('inputFile')
inputFile._name = 'crp0.fasta'
inputFile._contents = fastaFileContents
inputFiles.append(inputFile)
req._inputFile = inputFiles

# Launch non-blocking job and retrieve job ID
print "Launching remote MEME job"
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
  output_dir = 'meme_out'
  dir_count = sum(1 for _ in filter(None, urlparse(status._baseURL).path.split("/")))
  index_url = status._baseURL + '/index.html'
  call(["wget", "-r", "-nc", "-p", "-np", "-nH", "--cut-dirs=" + str(dir_count), "-P",  output_dir, index_url])

print "\nJob Complete\n\n";
