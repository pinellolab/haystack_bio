# This script submits a FASTA file to the MEME web service at NBCR
# and downloads the resulting output to the directory meme_out.
# Based on the script pdb2pqrclient.pl included in the 1.92 Opal-Perl
# toolkit, available at http://www.nbcr.net/software/opal/

use strict;
use warnings;

# Import the Opal libraries
use OpalServices;
use OpalTypes;
# Import other libraries
use Fcntl;
use File::Basename;
use File::Spec::Functions;

my ($location, $fimoService, $req, $commandline, $fh, $motif, $fasta,
  $fastaFile, $motifFile, $fastaInputFile, $motifInputFile, $status,
  $jobid, $output_dir, $dir_count, $index_url);
# Set the location of our service as well as the name of the FASTA file to input
# A list of NBCR services can be found at http://ws.nbcr.net/opal2/dashboard?command=serviceList
$location = "http://nbcr-222.ucsd.edu/opal2/services/FIMO_4.9.1";

# Instantiate a new service object to interact with.  Pass it the service location
$fimoService = OpalServices->new(service_url => $location);

# Instantiate a new job request
$req = JobInputType->new();

# inputs
$fastaFile = "crp0.fasta";
$motifFile = "crp0.meme.xml";

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
$commandline = "-upseqs $fastaFile $motifFile";
# 
# Alternatively, use a database like the preloaded
# Saccaromyces cerevisiae genome.
#
#$commandline = "$motifFile saccharomyces_cerevisiae.na"
$req->setArgs($commandline);

# Get contents of local files to be used as input
sysopen($fh, $fastaFile, O_RDONLY);
$fasta = do {local $/; <$fh>}; # slurp file
close $fh;
sysopen($fh, $motifFile, O_RDONLY);
$motif = do {local $/; <$fh>}; # slurp file
close $fh;

# Set name and content of remote input file.
# Two input files are used, the motif file
#and the FASTA file.
$fastaInputFile = InputFileType->new($fastaFile, $fasta);
$motifInputFile = InputFileType->new($motifFile, $motif);
$req->setInputFile($motifInputFile, $fastaInputFile);

# Launch the job and retrieve job ID
print "Launching non-blocking FIMO job\n";
$status = $fimoService->launchJob($req);
$jobid = $status->getJobID();
print "Received Job ID: ", $jobid, "\n";


# Poll for job status
print "Polling job status\n";
while (1) {
  # print current status
  print "Status:\n";
  print "\tCode: ", $status->getCode(), "\n";
  print "\tMessage: ", $status->getMessage(), "\n";
  print "\tOutput Base URL: ", $status->getBaseURL(), "\n";

  last if ($status->getCode() == 8 || $status->getCode() == 4); # STATUS_DONE || STATUS_FAILED

  print "Waiting 30 seconds\n";
  sleep(30);
  
  # Query job status
  $status = $fimoService->queryStatus($jobid);
}

if ($status->getCode() == 8) { # STATUS_DONE
  # Download the output files for the job.
  print "\nDownloading Outputs (using wget):\n\n\n";
  $output_dir = "fimo_out";
  $dir_count = 0;
  if ($status->getBaseURL() =~ m/:\/\/[^\/]+\/(.*)$/) {
    my $path = $1;
    my @path_elems = grep {length($_) > 0} split('/', $path);
    $dir_count = scalar(@path_elems);
  }
  $index_url = $status->getBaseURL() . "/index.html";
  # recursive, no clobber, no host directories, skip creating containing folder,
  # put everying in the $output_dir folder, download $index_url
  `wget -r -nc -p -np -nH --cut-dirs=$dir_count -P $output_dir $index_url`;
}
print "\nJob Complete\n\n\n";
1;
