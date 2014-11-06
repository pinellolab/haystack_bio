#!@WHICHPERL@

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI;
use Fcntl;
use File::Basename qw(fileparse);
use HTML::Template;

use CatList qw(open_entries_iterator load_entry DB_ROOT DB_NAME);
use MemeWebUtils qw(getnum get_safe_name get_safe_file_name loggable_date);
use MotifUtils qw(seq_to_intern matrix_to_intern intern_to_iupac intern_to_meme motif_id motif_width);
use OpalServices;
use OpalTypes;

# Setup logging
my $logger = undef;
eval {
  require Log::Log4perl;
  Log::Log4perl->import();
};
unless ($@) {
  Log::Log4perl::init('@APPCONFIGDIR@/logging.conf');
  $logger = Log::Log4perl->get_logger('meme.cgi.spamo');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting SpaMo CGI") if $logger;

#constants
$CGI::POST_MAX = 1024 * 1024 * 20; # 20Mb upload limit
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads

#defaults
my $MOTIFS_DB_INDEX = '@MEME_DIR@/etc/motif_db.index';
my $VERSION = '@S_VERSION@';
my $PROGRAM = 'SPAMO';
my $HELP_EMAIL = '@contact@';
my $BIN_DIR = '@MEME_DIR@/bin';
my $SPAMO_SERVICE_URL = "@OPAL@/SPAMO_@S_VERSION@";

&process_request();
exit(0);

#
# Checks for the existance of the cgi parameter search
# and invokes the approprate response
#
sub process_request {
  my $q = CGI->new;
  my $utils = new MemeWebUtils($PROGRAM, $BIN_DIR);
  #decide what to do
  my $search = $q->param('search'); #ignore taint
  
  if (not defined $search) {
    display_form($q, $utils);
  } else {
    my $data = check_params($q, $utils);
    if ($utils->has_errors()) {
      $utils->print_tailers();
    } else {
      submit_to_opal($q, $utils, $data);
    }
  }
}


#
# display_form
#
# Displays the spamo page.
#
sub display_form {
  my $q = shift;
  die("Expected CGI object") unless ref($q) eq 'CGI';
  my $utils = shift;
  die("Expected MemeWebUtils object") unless ref($utils) eq 'MemeWebUtils';
  
  #open the template
  my $template = HTML::Template->new(filename => 'spamo.tmpl');

  #fill in parameters
  $template->param(version => $VERSION);
  $template->param(help_email => $HELP_EMAIL);

  #create the parameter for the database selection
  my $iter = open_entries_iterator($MOTIFS_DB_INDEX, 0); #load the first category
  my $first = 1; #TRUE;
  my @dbs_param = ();
  while ($iter->load_next()) { #auto closes
    my %row_data = (db_id => $iter->get_index(), 
      db_name => ($iter->get_entry())[DB_NAME],
      selected => $first);
    push(@dbs_param, \%row_data);
    $first = 0; #FALSE;
  }
  $template->param(dbs => \@dbs_param);
  
  if ($q->param('inline_motifs')) {
    my $data = $q->param('inline_motifs');
    my $name = $q->param('inline_name');
    my $selected = $q->param('inline_selected');
    my (undef, undef, $count, undef) =
    $utils->check_meme_motifs($data, NAME => 'inline motifs', ALPHA => 'DNA');
    $template->param(inline_name => $name);
    $template->param(inline_motifs => $data);
    $template->param(inline_count => $count);
  }

  if ($utils->has_errors()) {
    #finish off the errors
    $utils->print_tailers();
  } else {
    #output the html
    print $q->header, $template->output;
  }
}

#
# check_params
#
# Reads and checks the parameters in preperation for calling spamo
#
sub check_params {
  my ($q, $utils) = @_;
  my %d = ();
  # get the input sequences
  my $seqsfh = $q->upload('sequences');
  my $seqstxt = $q->param('pasted_sequences');
  my $pasted = $utils->param_bool($q, 'use_pasted');
  ($d{SEQ_DATA}, undef, $d{SEQ_COUNT}, $d{SEQ_MIN}, $d{SEQ_MAX}, $d{SEQ_AVG}, 
    $d{SEQ_TOTAL}) = 
  $utils->get_sequence_data($seqstxt, $seqsfh, PASTE => $pasted);
  # get the input sequences name
  $d{SEQ_FILE} = ($pasted 
    ? 'sequences' 
    : get_safe_file_name(scalar $q->param('sequences'), 'sequences', 2));
  # get the primary motif
  my $motfh = $q->upload('primary_motif');
  my $motin = $q->param('inline_motifs');
  my $use_inline = $utils->param_bool($q, 'use_inline');
  $d{PRI_DATA} =
  $utils->upload_motif_file($motfh, $motin, $use_inline);
  $utils->check_meme_motifs($d{PRI_DATA}, NAME => 'primary motif', ALPHA => 'DNA');
  # get the primary motif name
  $d{PRI_FILE} = ($use_inline 
    ? get_safe_name(scalar $q->param('inline_name'), 'primary_motif.meme', 2) 
    : get_safe_file_name(scalar $q->param('primary_motif'), 'primary_motif.meme', 2));
  # get the secondary motifs
  ($d{DBSEC_NAME}, $d{DBSEC_DBS}, $d{UPSEC_ORIG_NAME}, $d{UPSEC_NAME}, 
    $d{UPSEC_DATA}, $d{UPSEC_ALPHA}, $d{UPSEC_COUNT}, $d{UPSEC_COLS}) = 
  $utils->param_motif_db($q);
  # get the dump seqs option
  $d{DUMP_SEQS} = $utils->param_bool($q, 'dumpseqs');
  # get the email address
  $d{EMAIL} = $utils->param_email($q);
  
  return \%d;
}

#
# make the arguments list
#
sub make_arguments {
  $logger->trace("call make_arguments") if $logger;
  my ($data) = @_;
  my @args = ();
  push(@args, '--margin', $data->{MARGIN}) if (defined($data->{MARGIN}));
  push(@args, '--dumpseqs') if ($data->{DUMP_SEQS});
  push(@args, '--uploaded', $data->{UPSEC_NAME}) if (defined($data->{UPSEC_NAME}));
  push(@args, $data->{SEQ_FILE}, $data->{PRI_FILE}, $data->{DBSEC_DBS});

  $logger->info(join(' ', @args)) if $logger;
  return @args;
}


sub make_verification {
  my ($data) = @_;

  #open the template
  my $template = HTML::Template->new(filename => 'spamo_verify.tmpl');

  #fill in parameters
  $template->param(seq_file => $data->{SEQ_FILE});
  $template->param(seq_count => $data->{SEQ_COUNT});
  $template->param(seq_min => $data->{SEQ_MIN});
  $template->param(seq_max => $data->{SEQ_MAX});
  $template->param(seq_avg => $data->{SEQ_AVG});
  $template->param(seq_total => $data->{SEQ_TOTAL});
  $template->param(pri_file => $data->{PRI_FILE});
  if ($data->{UPSEC_NAME}) {
    $template->param(sec_file => $data->{UPSEC_NAME});
    $template->param(sec_count => $data->{UPSEC_COUNT});
    $template->param(sec_columns => $data->{UPSEC_COLS});
  } else {
    $template->param(sec_db => $data->{DBSEC_NAME});
  }
  $template->param(dumpseqs => $data->{DUMP_SEQS});

  return $template->output;
}

#
# submit_to_opal
#
# Submits the job to a queue which will probably run on a cluster.
#
sub submit_to_opal {
  my ($q, $utils, $data) = @_;

  # get a handle to the webservice
  my $service = OpalServices->new(service_url => $SPAMO_SERVICE_URL);

  # create OPAL requst
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  # create list of OPAL file objects
  my @infilelist = ();
  # 1) Sequences file
  push(@infilelist, InputFileType->new($data->{SEQ_FILE}, $data->{SEQ_DATA}));
  # 2) primary motif file
  push(@infilelist, InputFileType->new($data->{PRI_FILE}, $data->{PRI_DATA}));
  # 2.5) Uploaded database
  push(@infilelist, InputFileType->new($data->{UPSEC_NAME}, $data->{UPSEC_DATA})) if $data->{UPSEC_NAME};
  # 3) Email address file (for logging purposes only)
  push(@infilelist, InputFileType->new("address_file", $data->{EMAIL}));
  # 4) Submit time file (for logging purposes only)
  push(@infilelist, InputFileType->new("submit_time_file", loggable_date()));
  # Add file objects to request
  $req->setInputFile(@infilelist);

  # output HTTP header
  print $q->header('text/html');

  # Submit the request to OPAL
  my $result = $service->launchJob($req);

  # Give user the verification form and email message
  my $verify = &make_verification($data);

  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);
}
