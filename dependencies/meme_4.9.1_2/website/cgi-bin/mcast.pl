#!@WHICHPERL@

# Author: Charles Grant

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI qw(:standard);
use File::Basename qw(fileparse);
use XML::Simple;

use CatList qw(load_entry);
use Globals;
use OpalServices;
use OpalTypes;
use MemeWebUtils qw(getnum get_safe_file_name loggable_date);

# Setup logging
my $logger = undef;
eval {
  require Log::Log4perl;
  Log::Log4perl->import();
};
unless ($@) {
  Log::Log4perl::init('@APPCONFIGDIR@/logging.conf');
  $logger = Log::Log4perl->get_logger('meme.cgi.mcast');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting MCAST CGI") if $logger;

# setup constants
$CGI::POST_MAX = 1024 * 1024 * 10; # Limit post to 10MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $PROGRAM = "MCAST";
my $BIN_DIR = "@MEME_DIR@/bin";
my $SERVICE_URL = "@OPAL@/MCAST_@S_VERSION@";
my $HELP_EMAIL = '@contact@';
my $INDEX_PATH = "@MEME_DIR@/etc/fasta_db.index";
my $QUERY_URL = "get_db_list.cgi?db_names=fasta_db.csv&mode=xml&catid=~category~";
my $DOC_URL =   "get_db_list.cgi?db_names=fasta_db.csv&mode=doc";
my $MAX_UPLOAD_SIZE = 1000000;


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
# make a text field
#
sub make_text_field {
  my ($title, $name, $value, $width) = @_;

  my $content = qq {
$title
<input class="maininput" type="text" size="$width" name="$name" VALUE="$value">
<BR>
  }; # end quote
  return($content);
} # make_text_field

#
# make the input form
#
sub make_form {
  my ($utils, $q) = @_;

  my $inline_name = $q->param('inline_name');
  my $inline_motifs = $q->param('inline_motifs');
  my $address = $q->param('address');

  my $action = "mcast.cgi";
  my $logo = "../doc/images/mcast-logo.png";
  my $alt = "$PROGRAM logo";
  my $form_description = "Use this form to submit motifs to <B>$PROGRAM</B> to be used in searching a sequence database.";

  #
  # required section
  #

  # required left side: address, motifs
  my $motif_doc = "../doc/meme-format.html";
  my $motif_example = "../examples/sample-dna-motif.meme-io";
  my $example_type = "DNA minimal motif format file";
  my $inline_descr = "MEME motifs from sequences in file <TT>'".(defined $inline_name ? $inline_name : "")."'</TT>.";
  my $req_left = $utils->make_address_field($address, $address);
  $req_left .= "<BR>\n";
  # the alphabet could be extracted from inline_motifs, but this is closest to 
  # what was happening when it was not declared at all.
  my $alphabet = undef; 
  $req_left .= $utils->make_motif_field($PROGRAM, "motifs", $inline_motifs, $alphabet,
    $motif_doc, $motif_example, $inline_descr, $example_type);

  # required right side: database
  my $fasta_doc = "../doc/fasta-format.html";
  my $sample_db = "../examples/sample-kabat.seq";
  my $req_right = $utils->make_supported_databases_field("database", "Sequence", $INDEX_PATH, $QUERY_URL, $DOC_URL);
  $req_right .= "<BR>\n<B>or</B>\n<BR>\n";
  $req_right .= $utils->make_upload_fasta_field("upload_db", $MAX_UPLOAD_SIZE, $fasta_doc, $sample_db);

  my $required = $utils->make_input_table("Required", $req_left, $req_right);

  # 
  # optional arguments
  #

  # optional left: description
  my $descr = "motifs";
  my $opt_left = $utils->make_description_field($descr, '');


  # optional right side:
  # Text box for motif hit p-value threshold
  my $opt_right = make_text_field(
                  '<a href="../doc/mcast.html#p-thresh"><i>P</i>-value threshold for motif hits</a>',
                  "motif_pvthresh",
                  "0.0005",
                  "5"
                );
  # Text box for max allowed distance between adj. hits.
  $opt_right .= "<p />\n";
  $opt_right .= make_text_field(
                  '<a href="../doc/mcast.html#max-gap">Maximum allowed distance between adjacent hits</a>',
                  "max_gap",
                  "50",
                  "5"
                );
  # e-value output threshold
  my @ev_list = ("0.01", "0.1", "1.0", "10.0", "100.0", "1000.0", "10000.0");
  my $ev_selected = "10.0";
  $opt_right .= "<p />\n";
  $opt_right .= $utils->make_select_field(
                  '<a href="../doc/mcast.html#e-thresh">Print matches with <i>E</i>-values less than this value</a> ', 
                  'output_evthresh', 
                  $ev_selected, 
                  @ev_list
  );
  # Text box for pseudocount weight.
  $opt_right .= "<p />\n";
  $opt_right .= make_text_field(
                  '<a href="../doc/mcast.html#bg-weight">Pseudocount weight</a>',
                  "pseudocount_weight",
                  "4",
                  "5"
                );

  my $optional = $utils->make_input_table("Optional", $opt_left, '<td style="width: 80%;">' .
      $opt_right . '</td>');

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

  # get motifs
  $d{MOTIFS_DATA} = $utils->upload_motif_file(scalar $q->param('motifs'), scalar $q->param('inline_motifs'));
  $d{MOTIFS_ORIG_NAME} = fileparse($q->param('inline_motifs') ? $q->param('inline_name') : $q->param('motifs'));
  $d{MOTIFS_NAME} = get_safe_file_name($d{MOTIFS_ORIG_NAME}, 'motifs.meme', 2);
  
  # check motifs
  (undef, $d{MOTIFS_ALPHA}, $d{MOTIFS_COUNT}, $d{MOTIFS_COLS}) = 
  $utils->check_meme_motifs($d{MOTIFS_DATA}) if ($d{MOTIFS_DATA});

  # get the sequences
  if ($q->param('upload_db')) {
    $logger->trace("call get_sequence_data") if $logger;
    ($d{UPSEQS_DATA}, $d{UPSEQS_ALPHA}, $d{UPSEQS_COUNT}, $d{UPSEQS_MIN}, 
      $d{UPSEQS_MAX}, $d{UPSEQS_AVG}, $d{UPSEQS_TOTAL}) = 
    $utils->get_sequence_data('', $q->upload('upload_db'), 
      NAME => 'uploaded dataset', ALPHA => $d{MOTIFS_ALPHA});
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
      unless (($entry[CatList::DB_NUCL] && $d{MOTIFS_ALPHA} eq 'DNA') || 
        ($entry[CatList::DB_PROT] && $d{MOTIFS_ALPHA} eq 'PROTEIN')) {
        $utils->whine("There is no ".$d{MOTIFS_ALPHA}." version of database '$db_name'.");
      } else {
        $d{DBSEQS_NAME} = $db_name;
        $d{DBSEQS_DB} = $entry[CatList::DB_ROOT] . ($d{MOTIFS_ALPHA} eq 'DNA' ? '.na' : '.aa');
      }
    } else {
      $utils->whine("You have not selected a database.");
    }
  } else {
    $utils->whine("You have not selected a database.");
  }

  $d{BGWEIGHT} = $utils->param_num($q, 'pseudocount_weight', 'pseudocount weight', 0);
  $d{MOTIF_PVTHRESH} = $utils->param_num($q, 'motif_pvthresh', 'motif hit pvalue threshold', 0, 1);
  $d{MAX_GAP} = $utils->param_int($q, 'max_gap', 'maximum distance between adjacent hits', 0); 
  $d{OUTPUT_EVTHRESH} = $utils->param_num($q, 'output_evthresh', 'evalue maximum', 0);

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
  push(@args, '-upseqs', $data->{UPSEQS_NAME}) if (defined($data->{UPSEQS_NAME}));
  push(@args, '-bgweight', $data->{BGWEIGHT}) if (defined($data->{BGWEIGHT}));
  push(@args, '-motif_pvthresh', $data->{MOTIF_PVTHRESH}) if (defined($data->{MOTIF_PVTHRESH}));
  push(@args, '-max_gap', $data->{MAX_GAP}) if (defined($data->{MAX_GAP}));
  push(@args, '-output_evthresh', $data->{OUTPUT_EVTHRESH}) if (defined($data->{OUTPUT_EVTHRESH}));
  push(@args, $data->{MOTIFS_NAME});
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
  my $template = HTML::Template->new(filename => 'mcast_verify.tmpl');
  $template->param(description => $data->{DESCRIPTION});
  
  $template->param(bgweight => $data->{BGWEIGHT});
  $template->param(motif_pvthresh => $data->{MOTIF_PVTHRESH});
  $template->param(max_gap => $data->{MAX_GAP});
  $template->param(output_evthresh => $data->{OUTPUT_EVTHRESH});  

  $template->param(motifs_name => $data->{MOTIFS_NAME});
  $template->param(motifs_orig_name => $data->{MOTIFS_ORIG_NAME});
  $template->param(motifs_safe_name => $data->{MOTIFS_NAME} ne $data->{MOTIFS_ORIG_NAME});
  $template->param(motifs_alpha => $data->{MOTIFS_ALPHA});
  $template->param(motifs_count => $data->{MOTIFS_COUNT});
  $template->param(motifs_cols => $data->{MOTIFS_COLS});

  $template->param(dbseqs_name => $data->{DBSEQS_NAME});

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
# Submit job to OPAL
#
sub submit_to_opal
{
  my ($utils, $q, $data) = @_;

  my $verify = &make_verification($data);
  my $mcast = OpalServices->new(service_url => $SERVICE_URL);

  #
  # start OPAL request
  #
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));
  #
  # create list of OPAL file objects
  #
  my @infilelist = ();
  # Motif File
  push(@infilelist, InputFileType->new($data->{MOTIFS_NAME}, $data->{MOTIFS_DATA}));
  # Uploaded sequence file
  if (defined($data->{UPSEQS_NAME})) {
    push(@infilelist, InputFileType->new($data->{UPSEQS_NAME}, $data->{UPSEQS_DATA}));
  }
  # Description
  if ($data->{DESCRIPTION}) {
    push(@infilelist, InputFileType->new("description", $data->{DESCRIPTION})); 
  }
  # Email address file (for logging purposes only)
  push(@infilelist, InputFileType->new("address_file", $data->{EMAIL}));
  # Submit time file (for logging purposes only)
  push(@infilelist, InputFileType->new("submit_time_file", loggable_date()));

  # Add file objects to request
  $req->setInputFile(@infilelist);

  # output HTTP header
  print $q->header;

  # Submit the request to OPAL
  my $result = $mcast->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);

} # submit_to_opal
