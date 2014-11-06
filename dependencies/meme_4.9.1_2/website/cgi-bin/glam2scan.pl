#!@WHICHPERL@
##
# Author: Timothy Bailey

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI qw(:standard);
use File::Basename qw(fileparse);
use MIME::Base64;
use SOAP::Lite;

use CatList qw(load_entry);
use Globals;
use MemeWebUtils qw(loggable_date get_safe_file_name);
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
  $logger = Log::Log4perl->get_logger('meme.cgi.glam2scan');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting GLAM2Scan CGI") if $logger;

# setup constants
$CGI::POST_MAX = 1024 * 1024 * 10; # Limit post to 10MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $PROGRAM = "GLAM2SCAN";
my $BIN_DIR = "@MEME_DIR@/bin"; # directory for executables 
my $SERVICE_URL = "@OPAL@/GLAM2SCAN_@S_VERSION@";
my $HELP_EMAIL = '@contact@';
my $INDEX_PATH = "@MEME_DIR@/etc/fasta_db.index";
my $QUERY_URL = "get_db_list.cgi?db_names=fasta_db.csv&mode=xml&catid=~category~";
my $DOC_URL =   "get_db_list.cgi?db_names=fasta_db.csv&mode=doc";
my $MAX_UPLOAD_SIZE = 1000000;
my $MAX_ALIGNMENTS = 200;

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
  #my $template = HTML::Template->new(filename => 'dreme.tmpl');

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
# make the input form
#
# Note: if query was included when this was called, that part of form
# is omitted.
#
sub make_form
{
  my ($utils, $q) = @_;

  my $action = "glam2scan.cgi";
  my $logo = "../doc/images/glam2scan_logo.png";
  my $alt = "$PROGRAM logo";
  my $form_description = "Use this form to submit a GLAM2 motif to ".
  "<B>$PROGRAM</B> to be used in searching a sequence database.";

  my $inline_name = $q->param('inline_name');
  my $inline_aln = $q->param('inline_aln');
  
  my $address = $q->param('address');
  my $alphabet = undef;

  #
  # required section
  #
  # required left side: address, motif, alphabet
  my $aln_doc = "../doc/glam2_ref.html";
  my $aln_example = "../examples/sample-glam2-aln.dna";
  my $example_type = "GLAM2 nucleotide motif";
  my $inline_descr = "GLAM2 motif <TT>'" . (defined $inline_name ? $inline_name : "") . "'</TT>.";
  my $req_left = $utils->make_address_field($address, $address);
  $req_left .= "<BR>\nSpecify the name of your motif file--a GLAM2 output file (HTML or text format).\n";
  $req_left .= $utils->make_motif_field($PROGRAM, "aln", $inline_name, $inline_aln, $alphabet,
    $aln_doc, $aln_example, $inline_descr, $example_type);
  unless ($alphabet) {
    my $text = "<BR>\n<BR>\nSpecify the <B>alphabet</B> of your motif:";
    my @values = (",", "DNA,nucleotide", "PROTEIN,protein");
    $req_left .= $utils->make_select_field2($text, "alphabet", "", @values);
  }

  # required right side: database
  my $fasta_doc = "../doc/fasta-format.html";
  my $sample_db = "../examples/sample-kabat.seq";
  my $req_right .= $utils->make_supported_databases_field("database", "Sequence", $INDEX_PATH, $QUERY_URL, $DOC_URL);
  $req_right .= "<BR>\n<B>or</B>\n<BR>\n";
  $req_right .= $utils->make_upload_fasta_field("upload_db", $MAX_UPLOAD_SIZE, $fasta_doc, $sample_db);

  my $required = $utils->make_input_table("Required", $req_left, $req_right);

  # 
  # optional arguments
  #

  # optional left: description, -n
  my $descr = "motif";
  my $opt_left = $utils->make_description_field($descr, '');

  $opt_left .= qq {
<BR>
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=3 NAME="-n" VALUE="25">
<B>Number of alignments</B> to report (<= $MAX_ALIGNMENTS)
  }; # end_quote


  # optional right side: dna-only

  # add DNA-ONLY options if motif is nucleotide or undefined
  my $opt_right = "";
  if (!defined $alphabet || $alphabet eq "n") {
    my $text = "<B>Examine both strands</B> - forward and reverse complement";
    my $options .= $utils->make_checkbox("-2", "1", $text, 1);
    $opt_right .= $utils->make_dna_only($options)
  } # add DNA-ONLY options


  my $optional = $utils->make_input_table("Optional", $opt_left, $opt_right);

  #
  # make final form
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
  # get the alphabet
  $d{ALPHA} = $utils->param_choice($q, 'alphabet', '', 'DNA', 'PROTEIN');
  $utils->whine("You have not specified the alphabet of the motifs.") unless ($d{ALPHA});

  # get the glam2 alignment
  my $inline_aln = $q->param('inline_aln');
  my $aln_file_dualvar = $q->param('aln'); # stop perl evaluating this in array context and collapsing it to nothing
  $d{ALN_DATA} = $utils->upload_motif_file($aln_file_dualvar, $inline_aln);
  $d{ALN_ORIG_NAME} = fileparse($inline_aln ? $q->param('inline_name') . '.glam2' : $q->param('aln'));
  $d{ALN_NAME} = get_safe_file_name($d{ALN_ORIG_NAME}, 'motifs.glam2', 2);
  # TODO validate glam2 file

  # get the sequences
  if ($q->param('upload_db')) {
    $logger->trace("call get_sequence_data") if $logger;
    ($d{UPSEQS_DATA}, $d{UPSEQS_ALPHA}, $d{UPSEQS_COUNT}, $d{UPSEQS_MIN}, 
      $d{UPSEQS_MAX}, $d{UPSEQS_AVG}, $d{UPSEQS_TOTAL}) = 
    $utils->get_sequence_data('', $q->upload('upload_db'), 
      NAME => 'uploaded dataset', ALPHA => $d{ALPHA});
    # get the sequences name
    $d{UPSEQS_ORIG_NAME} = fileparse($q->param('upload_db'));
    $d{UPSEQS_NAME} = get_safe_file_name($d{UPSEQS_ORIG_NAME}, 'uploaded', 2);
  } elsif (defined scalar $q->param('database')) {
    my $dbid = $utils->param_require($q, 'database');
    if ($dbid =~ m/^\d+$/) {
      $logger->trace("call load_entry") if $logger;
      my @entry = load_entry($INDEX_PATH, $dbid);
      my $db_name = $entry[CatList::DB_NAME];
      # check the alphabet and length of the database
      unless (($entry[CatList::DB_NUCL] && $d{ALPHA} eq 'DNA') || 
        ($entry[CatList::DB_PROT] && $d{ALPHA} eq 'PROTEIN')) {
        $utils->whine("There is no ".$d{ALPHA}." version of database '$db_name'.");
      } else {
        $d{DBSEQS_NAME} = $db_name;
        $d{DBSEQS_DB} = $entry[CatList::DB_ROOT] . ($d{ALPHA} eq 'DNA' ? '.na' : '.aa');
      }
    } else {
      $utils->whine("You have not selected a database.");
    }
  } else {
    $utils->whine("You have not selected a database.");
  }

  # get the number of alignments to report
  $d{ALIGNS} = $utils->param_int($q, '-n', 'number of alignments to report', 1, $MAX_ALIGNMENTS);

  #check if both strands should be examined
  $d{REVCOMP} = $utils->param_bool($q, '-2');

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
  push(@args, '-alpha', $data->{ALPHA});
  push(@args, '-aligns', $data->{ALIGNS}) if (defined($data->{ALIGNS}));
  push(@args, '-up_seqs', $data->{UPSEQS_NAME}) if (defined($data->{UPSEQS_NAME}));
  push(@args, '-revcomp') if ($data->{REVCOMP});
  push(@args, $data->{ALN_NAME});
  push(@args, $data->{DBSEQS_DB}) unless (defined($data->{UPSEQS_NAME}));
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
  my $template = HTML::Template->new(filename => 'glam2scan_verify.tmpl');

  $template->param(description => $data->{DESCRIPTION});
  $template->param(dbseqs_name => $data->{DBSEQS_NAME});
  $template->param(is_dna => $data->{ALPHA} eq 'DNA');
  $template->param(aligns => $data->{ALIGNS});
  $template->param(revcomp => $data->{REVCOMP});

  $template->param(aln_name => $data->{ALN_NAME});
  $template->param(aln_orig_name => $data->{ALN_ORIG_NAME});
  $template->param(aln_safe_name => $data->{ALN_NAME} ne $data->{ALN_ORIG_NAME});

  $template->param(upseqs_name => $data->{UPSEQS_NAME});
  $template->param(upseqs_orig_name => $data->{UPSEQS_ORIG_NAME});
  $template->param(upseqs_safe_name => $data->{UPSEQS_NAME} ne $data->{UPSEQS_ORIG_NAME});
  $template->param(upseqs_alpha => $data->{UPSEQS_ALPHA});
  $template->param(upseqs_count => $data->{UPSEQS_COUNT});
  $template->param(upseqs_min => $data->{UPSEQS_MIN});
  $template->param(upseqs_max => $data->{UPSEQS_MAX});
  $template->param(upseqs_avg => $data->{UPSEQS_AVG});
  $template->param(upseqs_total => $data->{UPSEQS_TOTAL});

  return $template->output;
}

#
# Submit the job to OPAL
#
sub submit_to_opal
{
  my ($utils, $q, $data) = @_;

  my $verify = &make_verification($data);

  my $service = OpalServices->new(service_url => $SERVICE_URL);

  #
  # start OPAL request
  #
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  #
  # create list of OPAL file objects
  #
  my @infilelist = ();
  # Alignment File
  my $inputfile = InputFileType->new($data->{ALN_NAME}, $data->{ALN_DATA});
  push(@infilelist, $inputfile);
  # Uploaded sequence file
  if (defined($data->{UPSEQS_NAME})) {
    push(@infilelist, InputFileType->new($data->{UPSEQS_NAME}, $data->{UPSEQS_DATA}));
  }
  if ($data->{DESCRIPTION}) {
    push(@infilelist, InputFileType->new('description', $data->{DESCRIPTION}));
  }
  # Email address file
  $inputfile = InputFileType->new("address_file", $data->{EMAIL});
  push(@infilelist, $inputfile);
  # Submit time file (for logging purposes only)
  $inputfile = InputFileType->new("submit_time_file", loggable_date());
  push(@infilelist, $inputfile);

  # Add file objects to request
  $req->setInputFile(@infilelist);

  # output HTTP header
  print $q->header;

  # Submit the request to OPAL
  my $result = $service->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);

} # submit_to_opal

