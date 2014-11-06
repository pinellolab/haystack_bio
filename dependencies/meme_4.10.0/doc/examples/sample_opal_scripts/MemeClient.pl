# This script submits a FASTA file to the MEME web service at NBCR
# and downloads the resulting output to the directory meme_out.
# Based on the script pdb2pqrclient.pl included in the 1.92 Opal-Perl
# toolkit, available at http://opal.nbcr.net/

use strict;
use warnings;

# Import the Opal libraries
use OpalServices;
use OpalTypes;

use Fcntl;
use File::Basename;
use File::Spec::Functions qw(catfile);

my ($location, $memeService, $req, $commandline, 
    $fh, $fasta, $fastaFile, $fastaInputFile, 
    $status, $jobid, $output_dir, $dir_count, $index_url);

# Set the location of our service as well as the name of the FASTA file to input
# A list of NBCR services can be found at http://ws.nbcr.net/opal2/dashboard?command=serviceList
$location = "http://nbcr-222.ucsd.edu/opal2/services/MEME_4.9.1";
$fastaFile = "crp0.fasta";

# Instantiate a new service object to interact with.  Pass it the service location
$memeService = OpalServices->new(service_url => $location);

# Instantiate a new job request
$req = JobInputType->new();

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
$commandline = "-alpha dna -nmotifs 3 -mod zoops $fastaFile";
$req->setArgs($commandline);

# Set name and content of input file.
# Only one input file is used, the FASTA file.
sysopen($fh, $fastaFile, O_RDONLY);
$fasta = do {local $/; <$fh>};

# Attach input file to job.
$fastaInputFile = InputFileType->new($fastaFile, $fasta);
$req->setInputFile($fastaInputFile);

# Launch the job and retrieve job ID
print "Launching non-blocking MEME job\n";
$status = $memeService->launchJob($req);
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
  $status = $memeService->queryStatus($jobid);
}

if ($status->getCode() == 8) { # STATUS_DONE
  # Download the output files for the job.
  print "\nDownloading Outputs (using wget):\n\n\n";
  $output_dir = "meme_out";
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
