#!@WHICHPERL@
# Author: Timothy Bailey
# Modified: Philip Machanick (for ChIP-seq)
##
##

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI qw(:standard);
use File::Basename qw(fileparse);
use MIME::Base64;
use Scalar::Util qw(looks_like_number);
use SOAP::Lite;

use CatList qw(open_entries_iterator load_entry DB_ROOT DB_NAME);
use Globals;
use OpalServices;
use OpalTypes;
use MemeWebUtils qw(get_safe_name get_safe_file_name loggable_date);


# Setup logging
my $logger = undef;
eval {
  require Log::Log4perl;
  Log::Log4perl->import();
};
unless ($@) {
  Log::Log4perl::init('@APPCONFIGDIR@/logging.conf');
  $logger = Log::Log4perl->get_logger('meme.cgi.memechip');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting meme-chip CGI") if $logger;

# setup constants
$CGI::POST_MAX = 1024 * 1024 * 55; # Limit post to 55MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $VERSION = '@S_VERSION@';
my $PROGRAM = "MEMECHIP";
my $BIN_DIR = '@MEME_DIR@/bin';
my $SERVICE_URL = '@OPAL@/MEMECHIP_@S_VERSION@';
my $HELP_EMAIL = '@contact@';
my $MOTIFS_DB_INDEX = '@MEME_DIR@/etc/motif_db.index';

&process_request();
exit(0);

sub process_request {
  $logger->trace("call process_request") if $logger;
  my $q = CGI->new;
  my $utils = new MemeWebUtils($PROGRAM, $BIN_DIR);
  
  unless (defined $q->param('search')) { # display the form
    &display_form($utils, $q);
  } else { # try to submit to the webservice
    my $data = &check_parameters($utils, $q);
    if (!$utils->has_errors()) {
      &submit_to_opal($utils, $q, $data);
    } else {
      $utils->print_tailers();
    }
  }
}

#
# display the form
#
sub display_form {
  $logger->trace("call display_form") if $logger;
  my ($utils, $q) = @_;

  #open the template
  my $template = HTML::Template->new(filename => 'meme-chip.tmpl');

  #fill in parameters
  $template->param(version => $VERSION);
  $template->param(help_email => $HELP_EMAIL);

  #create the parameter for the database selection
  my $first_motif_db = 1; #TRUE;
  my $iter = open_entries_iterator($MOTIFS_DB_INDEX, 0); #load the first category
  my @dbs_param = ();
  while ($iter->load_next()) { #auto closes
    my %row_data = (db_id => $iter->get_index(), 
      db_name => ($iter->get_entry())[DB_NAME],
      selected => $first_motif_db);
    push(@dbs_param, \%row_data);
    $first_motif_db = 0; #FALSE;
  }
  $template->param(dbs => \@dbs_param);

  if ($utils->has_errors()) {
    #finish off the errors
    $utils->print_tailers();
  } else {
    #output the html
    print $q->header, $template->output;
  }
}

#
# check the parameters and whine if any are bad
#
sub check_parameters {
  $logger->trace("call check_parameters") if $logger;
  my ($utils, $q) = @_;
  my %d = ();
  # get the input sequences
  my $seqsfh = $q->upload('sequences');
  my $pasted = $utils->param_bool($q, 'use_pasted');
  ($d{SEQ_DATA}, undef, $d{SEQ_COUNT}, $d{SEQ_MIN}, 
    $d{SEQ_MAX}, $d{SEQ_AVG}, $d{SEQ_TOTAL}) = 
      $utils->get_sequence_data(
        scalar $q->param('pasted_sequences'), $seqsfh, 
        PASTE => $pasted, ALPHA => 'DNA');
  # get the input sequences name
  my $name = 'sequences';
  $d{SEQ_ORIG_NAME} = ($pasted ? $name : fileparse($q->param('sequences')));
  $d{SEQ_NAME} = get_safe_name($d{SEQ_ORIG_NAME}, $name, 2);
  # get the motifs
  ($d{DBMOT_NAME}, $d{DBMOT_DBS}, $d{UPMOT_ORIG_NAME}, $d{UPMOT_NAME}, 
    $d{UPMOT_DATA}, undef, $d{UPMOT_COUNT}, $d{UPMOT_COLS}) = 
      $utils->param_motif_db($q, ALPHA => 'DNA');
  # get the email address
  $d{EMAIL} = $utils->param_email($q);
  # get the description
  $d{DESCRIPTION} = $q->param('description');

  # Universal Options
  # get the no-reverse-complement restriction setting
  $d{NORC} = $utils->param_bool($q, 'norc');
  # check for optional background Markov model
  if ($q->param('bfile')) {
    my $file = $q->upload('bfile');
    $d{BFILE_DATA} = do {local $/; <$file>};
    $d{BFILE_DATA} =~ s/\r\n/\n/g; # Windows -> UNIX eol
    $d{BFILE_DATA} =~ s/\r/\n/g; # MacOS -> UNIX eol
    $d{BFILE_ORIG_NAME} = fileparse($q->param('bfile'));
    $d{BFILE_NAME} = get_safe_name($d{BFILE_ORIG_NAME}, 'uploaded_background', 2);
  }
  # MEME Specific Options
  # get the motif distribution mode
  $d{MEME_MODE} = $utils->param_choice($q, 'meme_dist', 'zoops', 'oops', 'anr');
  # get the motif count bound
  $d{MEME_NMOTIFS} = $utils->param_int($q, 'meme_nmotifs', 'MEME maximum motifs', 1, undef, 3);
  # get the motif width bounds
  $d{MEME_MINW} = $utils->param_int($q, 'meme_minw', 'MEME minimum width', $MINW, $MAXW, 6);
  $d{MEME_MAXW} = $utils->param_int($q, 'meme_maxw', 'MEME maximum width', $MINW, $MAXW, 30);
  if ($d{MEME_MINW} > $d{MEME_MAXW}) {
    $utils->whine("The MEME minimum width (" . $d{MEME_MINW} . ") must be less than ".
      "or equal to the MEME maximum width (" . $d{MEME_MAXW} . ").");
  }
  # check for optional site bounds
  unless ($d{MEME_MODE} eq 'oops') {
    if ($utils->param_bool($q, 'meme_minsites_enable')) {
      $d{MEME_MINSITES} = $utils->param_int($q, 'meme_minsites', 
        'minimum sites', $MINSITES, $MAXSITES, 2);
    }
    if ($utils->param_bool($q, 'meme_maxsites_enable')) {
      $d{MEME_MAXSITES} = $utils->param_int($q, 'meme_maxsites', 
        'maximum sites', $MINSITES, $MAXSITES, 600);
    }
    if (defined($d{MEME_MINSITES}) && defined($d{MEME_MAXSITES}) && 
      $d{MEME_MINSITES} > $d{MEME_MAXSITES}) {
      $utils->whine("The MEME minimum sites (" . $d{MEME_MINSITES} . ") must be ".
        "less than or equal to the MEME maximum sites (" . $d{MEME_MAXSITES} . ").");
    }
  }
  # get the palindrome restriction setting
  $d{MEME_PAL} = $utils->param_bool($q, 'meme_pal');
  # DREME Specific Options
  # get the motif E-value cut-off threshold
  $d{DREME_E} = $utils->param_num($q, 'dreme_ethresh', 'DREME E-value threshold', 0, undef, 0.05);
  # get the optional motif count cut-off
  if ($utils->param_bool($q, 'dreme_nmotifs_enable')) {
    $d{DREME_M} = $utils->param_int($q, 'dreme_nmotifs', 
      'DREME maximum motifs', 1, undef, 10);
  }
  # CentriMo Specific Options
  # Get the minimum acceptable site score
  $d{CENTRIMO_SCORE} = $utils->param_num($q, 'centrimo_score', 'CentriMo minimum match score', undef, undef, 5);
  # Get the optional maximum central window
  if ($utils->param_bool($q, 'centrimo_maxreg_enable')) {
    $d{CENTRIMO_MAXREG} = $utils->param_num($q, 'centrimo_maxreg', 'CentriMo maximum central region width', 1, undef, 200);
  }
  # get the maximum central enrichment E-value to report
  $d{CENTRIMO_ETHRESH} = $utils->param_num($q, 'centrimo_ethresh', 'CentriMo E-value threshold', 0, undef, 10);
  # get if the seq IDs should be stored
  $d{CENTRIMO_LOCAL} = $utils->param_bool($q, 'centrimo_local');
  # get if the seq IDs should be stored
  $d{CENTRIMO_STORE_IDS} = $utils->param_bool($q, 'centrimo_store_ids');

  return \%d;
}

#
# make the arguments list
#
sub make_arguments {
  $logger->trace("call make_arguments") if $logger;
  my ($data) = @_;
  my @args = ();

  push(@args, '-norc') if ($data->{NORC});
  push(@args, '-bfile', $data->{BFILE_NAME}) if (defined($data->{BFILE_NAME}));
  push(@args, '-upmotif', $data->{UPMOT_NAME}) if (defined($data->{UPMOT_NAME}));
  # meme options
  push(@args, '-meme-mod', $data->{MEME_MODE});
  push(@args, '-meme-nmotifs', $data->{MEME_NMOTIFS});
  push(@args, '-meme-minw', $data->{MEME_MINW});
  push(@args, '-meme-maxw', $data->{MEME_MAXW});
  push(@args, '-meme-minsites', $data->{MEME_MINSITES}) if (defined($data->{MEME_MINSITES}));
  push(@args, '-meme-maxsites', $data->{MEME_MAXSITES}) if (defined($data->{MEME_MAXSITES}));
  push(@args, '-meme-pal') if ($data->{MEME_PAL});
  # dreme options
  push(@args, '-dreme-e', $data->{DREME_E});
  push(@args, '-dreme-m', $data->{DREME_M}) if (defined($data->{DREME_M}));
  # centrimo options
  push(@args, '-centrimo-score', $data->{CENTRIMO_SCORE});
  push(@args, '-centrimo-maxreg', $data->{CENTRIMO_MAXREG}) if (defined($data->{CENTRIMO_MAXREG}));
  push(@args, '-centrimo-ethresh', $data->{CENTRIMO_ETHRESH});
  push(@args, '-centrimo-local') if $data->{CENTRIMO_LOCAL};
  push(@args, '-centrimo-noseq') unless $data->{CENTRIMO_STORE_IDS};
  # sequences and motif dbs
  push(@args, $data->{SEQ_NAME}, $data->{DBMOT_DBS});

  $logger->info(join(' ', @args)) if $logger;
  return @args;
}

#
# make the verification message in HTML
#
sub make_verification {
  $logger->trace("call make_verification") if $logger;
  my ($data) = @_;

  my %motif_occur = (zoops => 'Zero or one per sequence', 
    oops => 'One per sequence', anr => 'Any number of repetitions');

  #open the template
  my $template = HTML::Template->new(filename => 'meme-chip_verify.tmpl');

  #fill in general parameters
  $template->param(norc => $data->{NORC});
  $template->param(bfile_orig_name => $data->{BFILE_ORIG_NAME});
  $template->param(description => $data->{DESCRIPTION});
  # sequence information:
  $template->param(seq_orig_name => $data->{SEQ_ORIG_NAME});
  $template->param(seq_name => $data->{SEQ_NAME});
  $template->param(seq_count => $data->{SEQ_COUNT});
  $template->param(seq_min => $data->{SEQ_MIN});
  $template->param(seq_max => $data->{SEQ_MAX});
  $template->param(seq_avg => $data->{SEQ_AVG});
  $template->param(seq_total => $data->{SEQ_TOTAL});
  # motif database information
  $template->param(dbmot_name => $data->{DBMOT_NAME});
  # uploaded motif information
  $template->param(upmot_orig_name => $data->{UPMOT_ORIG_NAME});
  $template->param(upmot_name => $data->{UPMOT_NAME});
  $template->param(upmot_count => $data->{UPMOT_COUNT});
  $template->param(upmot_cols => $data->{UPMOT_COLS});
  #fill in MEME specific parameters
  $template->param(meme_motif_occur => $motif_occur{$data->{MEME_MODE}});
  $template->param(meme_nmotifs => $data->{MEME_NMOTIFS});
  $template->param(meme_minsites => $data->{MEME_MINSITES});
  $template->param(meme_maxsites => $data->{MEME_MAXSITES});
  $template->param(meme_minw => $data->{MEME_MINW});
  $template->param(meme_maxw => $data->{MEME_MAXW});
  $template->param(meme_pal => $data->{MEME_PAL});
  #fill in DREME specific parameters
  $template->param(dreme_e => $data->{DREME_E});
  $template->param(dreme_m => $data->{DREME_M});
  #fill in CentriMo specific parameters
  $template->param(centrimo_score => $data->{CENTRIMO_SCORE});
  $template->param(centrimo_maxreg => $data->{CENTRIMO_MAXREG});
  $template->param(centrimo_ethresh => $data->{CENTRIMO_ETHRESH});
  $template->param(centrimo_local => $data->{CENTRIMO_LOCAL});
  $template->param(centrimo_store_ids => $data->{CENTRIMO_STORE_IDS});

  return $template->output;
}

#
# Submit job to webservice via OPAL
#
sub submit_to_opal
{
  $logger->trace("call submit_to_opal") if $logger;
  my ($utils, $q, $data) = @_;

  # make the verification form and email message
  my $verify = &make_verification($data);

  my $service = OpalServices->new(service_url => $SERVICE_URL);

  # start OPAL requst
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  # create list of OPAL file objects
  my @infilelist = ();
  # Sequence file
  push(@infilelist, InputFileType->new($data->{SEQ_NAME}, $data->{SEQ_DATA}));
  # Uploaded Motifs
  if ($data->{UPMOT_NAME}) {
    push(@infilelist, InputFileType->new($data->{UPMOT_NAME}, $data->{UPMOT_DATA}));
  }
  # Background file
  if ($data->{BFILE_NAME}) {
    push(@infilelist, InputFileType->new($data->{BFILE_NAME}, $data->{BFILE_DATA}));
  }
  # Description file
  if ($data->{DESCRIPTION} =~ m/\S/) {
    push(@infilelist, InputFileType->new("description", $data->{DESCRIPTION}));
  }
  # Email address file (for logging purposes only)
  push(@infilelist, InputFileType->new("address_file", $data->{EMAIL}));
  # Submit time file (for logging purposes only)
  push(@infilelist, InputFileType->new("submit_time_file", loggable_date()));

  # Add file objects to request
  $req->setInputFile(@infilelist);

  # Submit the request to OPAL
  $logger->trace("call launchJob") if $logger;
  my $result = $service->launchJob($req);

  # Give user the verification form and email message
  print $q->header;
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);

} # submit_to_opal
