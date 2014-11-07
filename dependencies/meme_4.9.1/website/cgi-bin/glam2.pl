#!@WHICHPERL@
##
## $Id:$
##
## $Log:$
## $Rev: $
##
# Author: Timothy Bailey

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI qw(:standard);
use File::Basename qw(fileparse);
use SOAP::Lite;
use MIME::Base64;

use Globals;
use MemeWebUtils qw(loggable_date get_safe_name getnum);
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
  $logger = Log::Log4perl->get_logger('meme.cgi.glam2');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting GLAM2 CGI") if $logger;

# get the directories using the new installation scheme

# constant globals
$CGI::POST_MAX = 1024 * 1024 * 10; # Limit post to 10MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $PROGRAM = 'GLAM2';
my $BIN_DIR = '@MEME_DIR@/bin';
my $SERVICE_URL = '@OPAL@/GLAM2_@S_VERSION@';
my $HELP_EMAIL = '@contact@';
my $MINSEQS = 2;
my $MINCOLS = 2;
my $MAXCOLS = 300;
my $MINREPS = 1;
my $MAXREPS = 100;
my $MINITER = 1;
my $MAXITER = 1000000;
my $MINPSEU = 0;
my $DEFAULT_MIN_SEQS = $MINSEQS;
my $DEFAULT_MIN_COLS = $MINCOLS;
my $DEFAULT_MAX_COLS = 50;
my $DEFAULT_INI_COLS = 20;
my $DEFAULT_NREPS = 10;
my $DEFAULT_NITER = 2000;
my $DEFAULT_DEL_PSEUDO = 0.1;
my $DEFAULT_NO_DEL_PSEUDO = 2;
my $DEFAULT_INS_PSEUDO = 0.02;
my $DEFAULT_NO_INS_PSEUDO = 1;


&process_request();
exit(0);

sub process_request {
  $logger->trace("call process_request") if $logger;
  my $q = CGI->new;
  my $utils = new MemeWebUtils($PROGRAM, $BIN_DIR);
  
  unless (defined $q->param('target_action')) { # display the form
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
  #my $template = HTML::Template->new(filename => 'glam2.tmpl');

  #fill in parameters
  #$template->param(version => $VERSION);
  #$template->param(help_email => $HELP_EMAIL);

  if (!$utils->has_errors()) { # output the html
    print $q->header, &make_form($utils, $q);
  } else { # finish off the errors
    $utils->print_tailers();
  }
}

#
# print the glam2 input form
#
sub make_form
{
  $logger->trace("call make_form") if $logger;
  my ($utils, $q) = @_;

  my $action = "glam2.cgi";
  my $logo = "../doc/images/glam2_logo.png";
  my $alt = "$PROGRAM logo";
  my $form_description = qq {
Use this form to submit DNA or protein sequences to $PROGRAM.
$PROGRAM will analyze your sequences for <B>gapped</B> motifs.
  }; # end quote

  my $data = $q->param('data');
  my $address = $q->param('address');
  my $description = $q->param('description');
  my $alphabet = undef;
  (undef, $alphabet) = $utils->get_sequence_data($data, '') if ($data);

  #
  # print the sequence input fields unless sequences already input
  #
  my $req_left;
  if (! $data) {
    #
    # required left side: sequences fields
    #
    my $seq_doc = "../help_sequences.html#sequences";
    my $alpha_doc = "../help_alphabet.html";
    my $format_doc = "../help_format.html";
    my $filename_doc = "../help_sequences.html#filename";
    my $paste_doc = "../help_sequences.html#actual-sequences";
    my $sample_file = "../examples/At.fa";
    my $sample_alphabet = "Protein";
    my $target = undef;
    $req_left = $utils->make_upload_sequences_field("datafile", "data", $MAXDATASET,
      $seq_doc, $alpha_doc, $format_doc, $filename_doc, $paste_doc, 
      $sample_file, $sample_alphabet, $target);
  } else {
    $req_left = qq {
<H3>$PROGRAM will analyze your previously provided sequences.</H3>
<INPUT TYPE="hidden" NAME="data" VALUE="$data">
    }; # end quote
  }

  #
  # required right side: email, embed
  #

  my $req_right = $utils->make_address_field($address, $address);
  $req_right .= "<BR>\n";
  my $embed_text = qq {
<B>Embed</B> the input sequences in the HTML results so 
that your query can be easily resubmitted and modified. 
This will increase the size of your output HTML file 
substantially if your sequence data is large!
  }; # end quote
  $req_right .= $utils->make_checkbox("embed_seqs", 1, $embed_text, 1);

  # finish required fields
  my $required = $utils->make_input_table("Required", $req_left, $req_right);

  #
  # optional fields
  #

  # optional left: description, limits
  my $descr = "sequences";
  my $opt_left = $utils->make_description_field($descr, $description);

  # limits fields
  $opt_left .= qq {
<BR>
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-z" VALUE="$DEFAULT_MIN_SEQS">
<B>Minimum</B> number of <B>sequences</B> in the alignment (>=$MINSEQS) 
<BR>
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-a" VALUE="$DEFAULT_MIN_COLS">
<A HREF="../help_width.html#aligned_cols"><B>Minimum</B></A> number of aligned <B>columns</B> (>=$MINCOLS) 
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-b" VALUE="$DEFAULT_MAX_COLS">
<A HREF="../help_width.html#aligned_cols"><B>Maximum</B></A> number of aligned <B>columns</B> (<=$MAXCOLS) 
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-w" VALUE="$DEFAULT_INI_COLS">
<A HREF="../help_width.html#aligned_cols"><B>Initial</B></A> number of aligned <B>columns</B> 
<BR>
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-r" VALUE="$DEFAULT_NREPS">
<B>Number</B> of alignment <B>replicates</B> (<=$MAXREPS) 
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-n" VALUE="$DEFAULT_NITER">
<B>Maximum</B> number of <B>iterations</B> without improvement (<=$MAXITER)
<BR>
  }; # end quote

  # optional right fields: pseudocounts, shuffle, dna-only
  my $opt_right = qq {
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-D" VALUE="$DEFAULT_DEL_PSEUDO">
<B>Deletion</B> pseudocount (>$MINPSEU)
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-E" VALUE="$DEFAULT_NO_DEL_PSEUDO">
<B>No-deletion</B> pseudocount (>$MINPSEU)
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-I" VALUE="$DEFAULT_INS_PSEUDO">
<B>Insertion</B> pseudocount (>$MINPSEU)
<BR>
<INPUT class="maininput" TYPE="TEXT" SIZE="6" NAME="-J" VALUE="$DEFAULT_NO_INS_PSEUDO">
<B>No-insertion</B> pseudocount (>$MINPSEU)
<BR>
<BR>
  }; # end quote
  my $shuffle_doc = "../help_sequences.html#shuffle";
  my $shuffle_text = "<A HREF='$shuffle_doc'><B>Shuffle</B></A> sequence letters";
  $opt_right .= $utils->make_checkbox("shuffle", 1, $shuffle_text, 0);

  # DNA-ONLY options
  if (!defined $alphabet || $alphabet eq 'DNA') {
    my $strands_text = "<B>Examine both strands</B> - forward and reverse complement";
    my $strands_options = $utils->make_checkbox("-2", 1, $strands_text, 1);
    $opt_right .= $utils->make_dna_only($strands_options);
  }

  # finish optional part
  my $optional = $utils->make_input_table("Optional", $opt_left, $opt_right);

  #
  # print final form
  #
  my $form = $utils->make_submission_form(
    $utils->make_form_header($PROGRAM, "Submission form"),
    $utils->make_submission_form_top($action, $logo, $alt, $form_description),
    $required,
    $optional,
    $utils->make_submit_button("Start search", $HELP_EMAIL),
    $utils->make_submission_form_bottom(),
    $utils->make_submission_form_tailer()
  );

  return $form;
} # make_form

#
# check the parameters and whine if any are bad
#
sub check_parameters {
  $logger->trace("call check_parameters") if $logger;
  my ($utils, $q) = @_;
  my %d = ();
  $d{SHUFFLE} = $utils->param_bool($q, 'shuffle');
  # get the input sequences
  my $inline_data = $q->param('data');
  my $seqs_dualval_file = $q->param('datafile');
  ($d{SEQS_DATA}, $d{SEQS_ALPHA}, $d{SEQS_COUNT}, $d{SEQS_MIN}, 
    $d{SEQS_MAX}, $d{SEQS_AVG}, $d{SEQS_TOTAL}) = 
  $utils->get_sequence_data(
    $inline_data, $seqs_dualval_file,
    MAX => $MAXDATASET, SHUFFLE => $d{SHUFFLE});
  # get the input sequences name
  $d{SEQS_ORIG_NAME} = fileparse(
    $inline_data ? 'sequences' : $seqs_dualval_file);
  $d{SEQS_NAME} = get_safe_name($d{SEQS_ORIG_NAME}, 'sequences', 2);
  # get settings
  $d{MIN_SEQS} = $utils->param_int($q, '-z', 'minimum number of sequences', 2);
  $d{MIN_COLS} = $utils->param_int($q, '-a', 'minimum number of aligned columns', 2, 300);
  $d{MAX_COLS} = $utils->param_int($q, '-b', 'maximum number of aligned columns', 2, 300);
  if (defined($d{MIN_COLS}) && defined($d{MAX_COLS}) && $d{MIN_COLS} > $d{MAX_COLS}) {
    $utils->whine('The minimum number of aligned columns (' . $d{MIN_COLS} .
      ') must be &le; to the maximum number of aligned columns (' . 
      $d{MAX_COLS} .').');
  }
  $d{INITIAL_COLS} = $utils->param_int($q, '-w', 'initial number of aligned columns', 
    (defined($d{MIN_COLS}) ? $d{MIN_COLS} : 2), 
    (defined($d{MAX_COLS}) ? $d{MAX_COLS} : 300));
  $d{RUNS} = $utils->param_int($q, '-r', 'number of alignment replicates', 1, 100);
  $d{RUN_NO_IMPR} = $utils->param_int($q, '-n', 'maximum iterations without improvement', 1, 1000000);
  $d{DEL_PSEUDO} = $utils->param_num($q, '-D', 'deletion pseudocount', 0);
  $d{NO_DEL_PSEUDO} = $utils->param_num($q, '-E', 'no-deletion pseudocount', 0);
  $d{INS_PSEUDO} = $utils->param_num($q, '-I', 'insertion pseudocount', 0);
  $d{NO_INS_PSEUDO} = $utils->param_num($q, '-J', 'no-insertion pseudocount', 0);
  $d{REV_COMP} = $utils->param_bool($q, '-2'); 
  $d{EMBED} = $utils->param_bool($q, 'embed_seqs');

  # get the description
  $d{DESCRIPTION} = $utils->param_require($q, 'description');

  # get the email
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
  push(@args, '-alpha', $data->{SEQS_ALPHA});
  push(@args, '-min_seqs', $data->{MIN_SEQS}) if (defined($data->{MIN_SEQS}));
  push(@args, '-min_cols', $data->{MIN_COLS}) if (defined($data->{MIN_COLS}));
  push(@args, '-max_cols', $data->{MAX_COLS}) if (defined($data->{MAX_COLS}));
  push(@args, '-initial_cols', $data->{INITIAL_COLS}) if (defined($data->{INITIAL_COLS}));
  push(@args, '-runs', $data->{RUNS}) if (defined($data->{RUNS}));
  push(@args, '-run_no_impr', $data->{RUN_NO_IMPR}) if (defined($data->{RUN_NO_IMPR}));
  push(@args, '-del_pseudo', $data->{DEL_PSEUDO}) if (defined($data->{DEL_PSEUDO}));
  push(@args, '-no_del_pseudo', $data->{NO_DEL_PSEUDO}) if (defined($data->{NO_DEL_PSEUDO}));
  push(@args, '-ins_pseudo', $data->{INS_PSEUDO}) if (defined($data->{INS_PSEUDO}));
  push(@args, '-no_ins_pseudo', $data->{NO_INS_PSEUDO}) if (defined($data->{NO_INS_PSEUDO}));
  push(@args, '-rev_comp') if ($data->{REV_COMP} && $data->{SEQS_ALPHA} eq 'DNA');
  push(@args, '-embed') if ($data->{EMBED});
  push(@args, $data->{SEQS_NAME});
  $logger->info(join(' ', @args)) if $logger;
  return @args;
}

#
# make the verification message in HTML
#
sub make_verification {
  $logger->trace("call make_verification") if $logger;
  my ($data) = @_;

  #open the template
  my $template = HTML::Template->new(filename => 'glam2_verify.tmpl');
  $template->param(description => $data->{DESCRIPTION});
  $template->param(min_seqs => $data->{MIN_SEQS});
  $template->param(min_cols => $data->{MIN_COLS});
  $template->param(max_cols => $data->{MAX_COLS});
  $template->param(initial_cols => $data->{INITIAL_COLS});
  $template->param(runs => $data->{RUNS});
  $template->param(run_no_impr => $data->{RUN_NO_IMPR});
  $template->param(del_pseudo => $data->{DEL_PSEUDO});
  $template->param(no_del_pseudo => $data->{NO_DEL_PSEUDO});
  $template->param(ins_pseudo => $data->{INS_PSEUDO});
  $template->param(no_ins_pseudo => $data->{NO_INS_PSEUDO});
  $template->param(dna => $data->{SEQS_ALPHA} eq 'DNA');
  $template->param(rev_comp => $data->{REV_COMP});
  $template->param(shuffle => $data->{SHUFFLE});
  $template->param(embed => $data->{EMBED});

  $template->param(seqs_name => $data->{SEQS_NAME});
  $template->param(seqs_orig_name => $data->{SEQS_ORIG_NAME});
  $template->param(seqs_safe_name => $data->{SEQS_NAME} ne $data->{SEQS_ORIG_NAME});
  $template->param(seqs_alpha => $data->{SEQS_ALPHA});
  $template->param(seqs_count => $data->{SEQS_COUNT});
  $template->param(seqs_min => $data->{SEQS_MIN});
  $template->param(seqs_max => $data->{SEQS_MAX});
  $template->param(seqs_avg => $data->{SEQS_AVG});
  $template->param(seqs_total => $data->{SEQS_TOTAL});
  return $template->output;
}

#
# Submit job to webservice via OPAL
#
sub submit_to_opal
{
  $logger->trace("call submit_to_opal") if $logger;
  my ($utils, $q, $data) = @_;

  my $verify = &make_verification($data);

  my $service = OpalServices->new(service_url => $SERVICE_URL);

  # start OPAL requst
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  # create list of OPAL file objects
  my @infilelist = ();
  # Sequence file
  push(@infilelist, InputFileType->new($data->{SEQS_NAME}, $data->{SEQS_DATA}));
  # Description
  push(@infilelist, InputFileType->new("description", $data->{DESCRIPTION}))
      if $data->{DESCRIPTION};
  # Email address file (for logging purposes only)
  push(@infilelist, InputFileType->new("address_file", $data->{EMAIL}));
  # Submit time file (for logging purposes only)
  push(@infilelist, InputFileType->new("submit_time_file", loggable_date()));

  # Add file objects to request
  $req->setInputFile(@infilelist);

  # output HTTP header
  print $q->header;

  # Submit the request to OPAL
  my $result = $service->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);
} # submit_to_opal

