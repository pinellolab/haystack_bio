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
  $logger = Log::Log4perl->get_logger('meme.cgi.centrimo');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting CentriMo CGI") if $logger;


$CGI::POST_MAX = 1024 * 20000; # 20Mb upload limit

#defaults
my $MOTIFS_DB_INDEX = '@MEME_DIR@/etc/motif_db.index';
my $VERSION = '@S_VERSION@';
my $PROGRAM = 'CENTRIMO';
my $HELP_EMAIL = '@contact@';
my $BIN_DIR = '@MEME_DIR@/bin';
my $CENTRIMO_SERVICE_URL = "@OPAL@/CENTRIMO_@S_VERSION@";

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
# Displays the centrimo page.
#
sub display_form {
  my $q = shift;
  die("Expected CGI object") unless ref($q) eq 'CGI';
  my $utils = shift;
  die("Expected MemeWebUtils object") unless ref($utils) eq 'MemeWebUtils';
  
  #open the template
  my $template = HTML::Template->new(filename => 'centrimo.tmpl');

  #fill in parameters
  $template->param(version => $VERSION);
  $template->param(help_email => $HELP_EMAIL);
  
  my $first_motif_db = 1; #TRUE;
  if ($q->param('inline_motifs')) {
    my $data = $q->param('inline_motifs');
    my $name = $q->param('inline_name');
    my (undef, undef, $count, undef) =
    $utils->check_meme_motifs($data, NAME => 'inline motifs', ALPHA => 'DNA');
    $template->param(inline_name => $name);
    $template->param(inline_motifs => $data);
    $template->param(inline_count => $count);
    $first_motif_db = 0; #FALSE
  }

  #create the parameter for the database selection
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
# check_params
#
# Reads and checks the parameters in preperation for calling centrimo
#
sub check_params {
  my ($q, $utils) = @_;
  my %d = ();
  # get the local enrichment option
  $d{LOCAL} = $utils->param_bool($q, 'local');
  # get the search option
  $d{COMPAR} = $utils->param_bool($q, 'compar');
  # get the input sequences
  my $seqsfh = $q->upload('sequences');
  my $pasted = $utils->param_bool($q, 'use_pasted');
  ($d{SEQ_DATA}, undef, $d{SEQ_COUNT}, $d{SEQ_MIN}, $d{SEQ_MAX}, $d{SEQ_AVG}, 
    $d{SEQ_TOTAL}) = 
  $utils->get_sequence_data(scalar $q->param('pasted_sequences'), $seqsfh, PASTE => $pasted);
  # get the input sequences name
  my $name = 'sequences';
  $d{SEQ_ORIG_NAME} = ($pasted ? $name : fileparse($q->param('sequences')));
  $d{SEQ_NAME} = get_safe_name($d{SEQ_ORIG_NAME}, $name, 2);
  # get the comparative sequences
  if ($d{COMPAR}) {
    my $compar_seqs_fh = $q->upload('compar_sequences');
    my $compar_pasted = $utils->param_bool($q, 'use_compar_pasted');
    ( $d{D_SEQ_DATA}, undef, $d{D_SEQ_COUNT}, $d{D_SEQ_MIN}, $d{D_SEQ_MAX},
      $d{D_SEQ_AVG}, $d{D_SEQ_TOTAL}
    ) = $utils->get_sequence_data(scalar $q->param('compar_pasted_sequences'),
          $compar_seqs_fh, PASTE => $compar_pasted);
    # get the input sequences name
    my $compar_name = 'neg_sequences';
    $d{D_SEQ_ORIG_NAME} = ($pasted ? $name : fileparse($q->param('compar_sequences')));
    $d{D_SEQ_NAME} = get_safe_name($d{SEQ_ORIG_NAME}, $compar_name, 2);
    if ($d{D_SEQ_NAME} eq $d{SEQ_NAME}) {
      $d{SEQ_NAME} .= '1';
      $d{D_SEQ_NAME} .= '2';
    }
  }
  # get the motifs
  ($d{DBMOT_NAME}, $d{DBMOT_DBS}, $d{UPMOT_ORIG_NAME}, $d{UPMOT_NAME}, 
    $d{UPMOT_DATA}, $d{UPMOT_ALPHA}, $d{UPMOT_COUNT}, $d{UPMOT_COLS}) = 
  $utils->param_motif_db($q);
  # get the email address
  $d{EMAIL} = $utils->param_email($q);
  # get the description
  $d{DESCRIPTION} = $q->param('description');

  # check for optional background Markov model
  if ($q->param('bfile')) {
    my $file = $q->upload('bfile');
    $d{BFILE_DATA} = do {local $/; <$file>};
    $d{BFILE_DATA} =~ s/\r\n/\n/g; # Windows -> UNIX eol
    $d{BFILE_DATA} =~ s/\r/\n/g; # MacOS -> UNIX eol
    $d{BFILE_ORIG_NAME} = fileparse($q->param('bfile'));
    $d{BFILE_NAME} = get_safe_name($d{BFILE_ORIG_NAME}, 'uploaded_background', 2);
  }

  # advanced settings
  $d{STRANDS} = $utils->param_choice($q, 'strands', 'both', 'both_flip', 'given');
  $d{MIN_SCORE} = $utils->param_num($q, 'min_score', 'minimum acceptable score',
    undef, undef, 5);
  $d{OPT_SCORE} = $utils->param_bool($q, 'opt_score');
  if ($utils->param_bool($q, 'use_max_region')) {
    $d{MAX_REGION} = $utils->param_int($q, 'max_region', 'maximum region', 
      0, undef, 200);
  }
  $d{ETHRESH} = $utils->param_num($q, 'ethresh', 'evalue threshold', 
    0, undef, 10);
  # note that negs_always is checked before the sequences (hence why it is not here).
  $d{STORE_IDS} = $utils->param_bool($q, 'store_ids');

  return \%d;
}

#
# make the arguments list
#
sub make_arguments {
  $logger->trace("call make_arguments") if $logger;
  my ($data) = @_;
  my @args = ();
  push(@args, '--local') if ($data->{LOCAL});
  push(@args, '--score', $data->{MIN_SCORE}) if (defined($data->{MIN_SCORE}));
  push(@args, '--optsc') if ($data->{OPT_SCORE});
  push(@args, '--ethresh', $data->{ETHRESH}) if (defined($data->{ETHRESH}));
  push(@args, '--maxreg', $data->{MAX_REGION}) if (defined($data->{MAX_REGION}));
  push(@args, '--neg', $data->{D_SEQ_NAME}) if (defined($data->{D_SEQ_NAME}));
  push(@args, '--upmotifs', $data->{UPMOT_NAME}) if (defined($data->{UPMOT_NAME}));
  push(@args, '--bfile', $data->{BFILE_NAME}) if (defined($data->{BFILE_NAME}));
  push(@args, '--norc') if ($data->{STRANDS} eq 'given');
  push(@args, '--flip') if ($data->{STRANDS} eq 'both_flip');
  push(@args, '--noseq') unless ($data->{STORE_IDS});
  push(@args, $data->{SEQ_NAME}, $data->{DBMOT_DBS});

  $logger->info(join(' ', @args)) if $logger;
  return @args;
}


sub make_verification {
  my ($data) = @_;

  #open the template
  my $template = HTML::Template->new(filename => 'centrimo_verify.tmpl');

  #fill in parameters
  $template->param(description => $data->{DESCRIPTION});
  $template->param(local => $data->{LOCAL});
  $template->param(compar => $data->{COMPAR});
  $template->param(norc => $data->{STRANDS} eq 'given');
  $template->param(flip => $data->{STRANDS} eq 'both_flip');
  $template->param(min_score => $data->{MIN_SCORE});
  $template->param(opt_score => $data->{OPT_SCORE});
  $template->param(ethresh => $data->{ETHRESH});
  $template->param(max_region => $data->{MAX_REGION});
  $template->param(store_ids => $data->{STORE_IDS});

  # sequence information:
  $template->param(seq_orig_name => $data->{SEQ_ORIG_NAME});
  $template->param(seq_name => $data->{SEQ_NAME});
  $template->param(seq_count => $data->{SEQ_COUNT});
  $template->param(seq_min => $data->{SEQ_MIN});
  $template->param(seq_max => $data->{SEQ_MAX});
  $template->param(seq_avg => $data->{SEQ_AVG});
  $template->param(seq_total => $data->{SEQ_TOTAL});

  # comparative sequence information:
  if (defined($data->{D_SEQ_NAME})) {
    $template->param(d_seq_orig_name => $data->{D_SEQ_ORIG_NAME});
    $template->param(d_seq_name => $data->{D_SEQ_NAME});
    $template->param(d_seq_count => $data->{D_SEQ_COUNT});
    $template->param(d_seq_min => $data->{D_SEQ_MIN});
    $template->param(d_seq_max => $data->{D_SEQ_MAX});
    $template->param(d_seq_avg => $data->{D_SEQ_AVG});
    $template->param(d_seq_total => $data->{D_SEQ_TOTAL});
  }

  # motif database information
  $template->param(dbmot_name => $data->{DBMOT_NAME});

  # uploaded motif information
  $template->param(upmot_orig_name => $data->{UPMOT_ORIG_NAME});
  $template->param(upmot_name => $data->{UPMOT_NAME});
  $template->param(upmot_count => $data->{UPMOT_COUNT});
  $template->param(upmot_cols => $data->{UPMOT_COLS});

  # uploaded background information
  $template->param(bfile_orig_name => $data->{BFILE_ORIG_NAME});
  $template->param(bfile_name => $data->{BFILE_NAME});


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
  my $service = OpalServices->new(service_url => $CENTRIMO_SERVICE_URL);

  # create OPAL requst
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  # create list of OPAL file objects
  my @infilelist = ();
  # Sequences file
  push(@infilelist, InputFileType->new($data->{SEQ_NAME}, $data->{SEQ_DATA}));
  # Comparative Sequences file
  push(@infilelist, InputFileType->new($data->{D_SEQ_NAME}, $data->{D_SEQ_DATA})) if $data->{D_SEQ_NAME};
  # Uploaded database
  push(@infilelist, InputFileType->new($data->{UPMOT_NAME}, $data->{UPMOT_DATA})) if $data->{UPMOT_NAME};
  # Background file
  push(@infilelist, InputFileType->new($data->{BFILE_NAME}, $data->{BFILE_DATA})) if $data->{BFILE_NAME};
  # description file
  push(@infilelist, InputFileType->new("description", $data->{DESCRIPTION})) if ($data->{DESCRIPTION});
  # Email address file (for logging purposes only)
  push(@infilelist, InputFileType->new("address_file", $data->{EMAIL}));
  # Submit time file (for logging purposes only)
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
