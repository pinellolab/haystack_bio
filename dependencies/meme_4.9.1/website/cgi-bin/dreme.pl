#!@WHICHPERL@

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI qw(:standard);
use File::Basename qw(fileparse);
use HTML::Template;

use MemeWebUtils qw(getnum get_safe_name loggable_date);
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
  $logger = Log::Log4perl->get_logger('meme.cgi.dreme');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting DREME CGI") if $logger;

# constants
$CGI::POST_MAX = 1024 * 1024 * 20; # Limit post to 20MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $VERSION = '@S_VERSION@';
my $PROGRAM = 'DREME';
my $HELP_EMAIL = '@contact@';
my $BIN_DIR = '@MEME_DIR@/bin';
my $DREME_SERVICE_URL = '@OPAL@/DREME_@S_VERSION@';
my $DNA_ALPHABET = "ACGTURYKMSWBDHVN"; #includes IUPAC ambiguous characters

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

sub display_form {
  $logger->trace("call display_form") if $logger;
  my ($utils, $q) = @_;

  #open the template
  my $template = HTML::Template->new(filename => 'dreme.tmpl');

  #fill in parameters
  $template->param(version => $VERSION);
  $template->param(help_email => $HELP_EMAIL);

  if (!$utils->has_errors()) { # output the html
    print $q->header, $template->output;
  } else { # finish off the errors
    $utils->print_tailers();
  }
}

sub check_parameters {
  $logger->trace("call check_parameters") if $logger;
  my ($utils, $q) = @_;
  my %d = ();

  my ($use_pasted, $name);
  $use_pasted = $utils->param_bool($q, 'pos_use_pasted');
  # need to enforce scalar evaluation as the param function returns an empty 
  # array when the parameter is undefined
  ($d{POS_FASTA}, undef, $d{POS_NUM}, $d{POS_MIN}, $d{POS_MAX}, 
    $d{POS_AVE}, $d{POS_TOTAL}) = 
  $utils->get_sequence_data(scalar $q->param('pos_pasted'), 
    scalar $q->param('pos_file'), PASTE => $use_pasted, ALPHA => 'DNA');

  $name = 'sequences';
  $d{POS_ORIG_NAME} = ($use_pasted ? $name : fileparse($q->param('pos_file')));
  $d{POS_NAME} = &get_safe_name($d{POS_ORIG_NAME}, $name, 2);

  if ($utils->param_bool($q, 'has_negs')) {
    $use_pasted = $utils->param_bool($q, 'neg_use_pasted');

    ($d{NEG_FASTA}, undef, $d{NEG_NUM}, $d{NEG_MIN}, $d{NEG_MAX}, 
      $d{NEG_AVE}, $d{NEG_TOTAL}) = 
    $utils->get_sequence_data(scalar $q->param('neg_pasted'), 
      scalar $q->param('neg_file'), NAME => 'comparative sequences', 
      PASTE => $use_pasted, ALPHA => 'DNA');

    $name = 'comparative_sequences';
    $d{NEG_ORIG_NAME} = ($use_pasted ? $name : fileparse($q->param('neg_file')));
    $d{NEG_NAME} = &get_safe_name($d{NEG_ORIG_NAME}, $name, 2);
  }

  $d{EMAIL} = $utils->param_email($q);

  $d{DESCRIPTION} = $utils->param_require($q, 'description');

  $d{NORC} = $utils->param_bool($q, 'norc');

  $d{LIMIT_EVALUE} = $utils->param_num($q, 'limit_evalue', 'evalue limit', 0);

  if ($utils->param_bool($q, 'limit_count_enabled')) {
    $d{LIMIT_COUNT} = $utils->param_int($q, 'limit_count', 'maximum motif count', 1);
  }

  return \%d;
}

sub make_arguments {
  $logger->trace("call make_arguments") if $logger;
  my ($data) = @_;
  my @args = ();
  push(@args, '-norc') if ($data->{NORC});
  push(@args, '-e', $data->{LIMIT_EVALUE});
  push(@args, '-m', $data->{LIMIT_COUNT}) if (defined($data->{LIMIT_COUNT}));
  push(@args, '-n', $data->{NEG_NAME}) if (defined($data->{NEG_NAME}));
  push(@args, $data->{POS_NAME});

  $logger->info(join(' ', @args)) if $logger;
  return @args;
}

sub make_verification {
  $logger->trace("call make_verification") if $logger;
  my ($data) = @_;

  #open the template
  my $template = HTML::Template->new(filename => 'dreme_verify.tmpl');

  #fill in parameters
  $template->param(description => $data->{DESCRIPTION});
  $template->param(pos_file => $data->{POS_NAME});
  $template->param(pos_count => $data->{POS_NUM});
  $template->param(pos_min => $data->{POS_MIN});
  $template->param(pos_max => $data->{POS_MAX});
  $template->param(pos_avg => $data->{POS_AVE});
  $template->param(pos_total => $data->{POS_TOTAL});

  if ($data->{NEG_NAME}) {
    $template->param(neg_file => $data->{NEG_NAME});
    $template->param(neg_count => $data->{NEG_NUM});
    $template->param(neg_min => $data->{NEG_MIN});
    $template->param(neg_max => $data->{NEG_MAX});
    $template->param(neg_avg => $data->{NEG_AVE});
    $template->param(neg_total => $data->{NEG_TOTAL});
  }

  $template->param(norc => $data->{NORC});
  $template->param(evalue => $data->{LIMIT_EVALUE});
  if (defined($data->{LIMIT_COUNT})) {
    $template->param(count => $data->{LIMIT_COUNT});
  }

  return $template->output;
}

sub submit_to_opal {
  $logger->trace("call submit_to_opal") if $logger;
  my ($utils, $q, $data) = @_;

  my $verify = &make_verification($data);

  my $service = OpalServices->new(service_url => $DREME_SERVICE_URL);

  # start OPAL request
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  # create list of OPAL file objects
  my @infilelist = ();
  # positives
  push(@infilelist, InputFileType->new($data->{POS_NAME}, $data->{POS_FASTA}));
  # optional negatives
  if ($data->{NEG_NAME}) {
    push(@infilelist, InputFileType->new($data->{NEG_NAME}, $data->{NEG_FASTA}));
  }
  # Description file
  if ($data->{DESCRIPTION} && $data->{DESCRIPTION} =~ m/\S/) {
    push(@infilelist, InputFileType->new("description", $data->{DESCRIPTION}));
  }
  # Email address file (for logging)
  push(@infilelist, InputFileType->new("address_file", $data->{EMAIL}));
  # Submit time file (for logging)
  push(@infilelist, InputFileType->new("submit_time_file", loggable_date()));

  # Add file objects to request
  $req->setInputFile(@infilelist);

  # output HTTP header
  print $q->header;

  # Submit the request to OPAL
  my $result = $service->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);
}
