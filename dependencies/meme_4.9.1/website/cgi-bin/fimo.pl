#!@WHICHPERL@
# Author: Timothy Bailey


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
use MemeWebUtils qw(get_safe_file_name loggable_date);

# Setup logging
my $logger = undef;
eval {
  require Log::Log4perl;
  Log::Log4perl->import();
};
unless ($@) {
  Log::Log4perl::init('@APPCONFIGDIR@/logging.conf');
  $logger = Log::Log4perl->get_logger('meme.cgi.fimo');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting FIMO CGI") if $logger;

# setup constants
$CGI::POST_MAX = 1024 * 1024 * 20; # Limit post to 20MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $PROGRAM = "FIMO";
my $BIN_DIR = "@MEME_DIR@/bin";	# binary directory
my $SERVICE_URL = "@OPAL@/FIMO_@S_VERSION@";
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
# make the input form
#
sub make_form {
  my ($utils, $q) = @_;

  # get data
  my $address = $q->param('address');
  my $inline_name = $q->param('inline_name');
  my $inline_motifs = $q->param('inline_motifs');


  my $action = "fimo.cgi";
  my $logo = "../doc/images/fimo_logo.png";
  my $alt = "$PROGRAM logo";
  my $form_description = "Use this form to submit motifs to <B>$PROGRAM</B> to be used in searching a sequence database.";

  #
  # required section
  #

  # required left side: address, motifs
  my $motif_doc = "../doc/meme-format.html";
  my $motif_example = "../examples/sample-dna-motif.meme-io";
  my $example_type = "DNA minimal motif format file";
  my $inline_descr = "MEME motifs from sequences in file <TT>'" . (defined $inline_name ? $inline_name : "") . "'</TT>.";
  my $req_left = $utils->make_address_field($address, $address);
  $req_left .= "<BR>\n";
  my $alphabet = undef; #FIXME I have no idea where this is meant to be initilised, it is not a parameter as I would have expected.
  $req_left .= $utils->make_motif_field($PROGRAM, "motifs", $inline_name, $inline_motifs, $alphabet,
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
  my @pv_list = ("1", "0.1", "0.01", "0.001", "1e-4", "1e-5", "1e-6", "1e-7", "1e-8", "1e-9");
  my $pv_selected = "1e-4";
  my $opt_right = $utils->make_select_field('<i>p</i>-value output theshold<br/>', 'output_pthresh', 
      $pv_selected, @pv_list);
  $opt_right .= "<BR><BR>\n";
  #my $text = "Scan given <A HREF='$doc' <B>strand</B></A> only (DNA motifs only)";
  my $text = "Scan given strand only";
  $opt_right .= $utils->make_checkbox("norc", 1, $text, 0);
  $opt_right .= "<BR>\n";

  my $optional = $utils->make_input_table("Optional", $opt_left, '<td style="width: 60%;">' .
      $opt_right . '</br></td>');

  my $custom_track_links = qq {
    <h2>FIMO Custom Tracks For the UCSC Genome Browser</h2>
    <p>
    The links below display custom tracks for FIMO in the UCSC Genome Browser.
    The tracks display statistically significant matches between the human
    genome and motifs from the <a href="http://jaspar.genereg.net">JASPAR CORE 2009</a>
    and <a href="http://biobase-international.com/index.php?id=transfac">TRANSFAC
    10.2</a> motif databases.
    The quality of each match is summarized as
    a q-value, defined as the minimal false discovery rate threshold at
    which the match would be deemed significant.  Only matches with a
    q-value &le; 0.1 are shown.
    <p/>
    <ul>
    <li>JASPAR <a
    href="http://genome.ucsc.edu/cgi-bin/hgTracks?db=hg18&position=chr21:33,040,950-3,041,655&hgct_customText=track%20type=bigBed%20name=FIMO-JASPAR_CORE%20description=%22FIMO%20matches%20to%20JASPAR%20CORE%202009%20motifs%20with%20q-value%20%3c%3d%200.1%22%20visibility=full%20bigDataUrl=http://noble.gs.washington.edu/custom-tracks/fimo.hg18.jaspar_core-0.1.bb%20htmlUrl=http://noble.gs.washington.edu/custom-tracks/fimo.jaspar_core.description.html%20useScore=1">
    hg18</a>,
    <a
    href="http://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=chr21:34,119,079-34,119,784&hgct_customText=track%20type=bigBed%20name=FIMO-JASPAR_CORE%20description=%22FIMO%20matches%20to%20JASPAR%20CORE%202009%20motifs%20with%20q-value%20%3c%3d%200.1%22%20visibility=full%20bigDataUrl=http://noble.gs.washington.edu/custom-tracks/fimo.hg19.jaspar_core-0.1.bb%20htmlUrl=http://noble.gs.washington.edu/custom-tracks/fimo.jaspar_core.description.html%20useScore=1">
    hg19</a></li>
    <li>TRANSFAC <a
    href="http://genome.ucsc.edu/cgi-bin/hgTracks?db=hg18&position=chr21:33,040,950-33,041,655&hgct_customText=track%20type=bigBed%20name=FIMO-TRANSFAC%20description=%22FIMO%20matches%20to%20TRANSFAC%20motifs%20with%20q-value%20%3c%3d%200.1%22%20visibility=full%20bigDataUrl=http://noble.gs.washington.edu/custom-tracks/fimo.hg18.transfac-0.1.bb%20htmlUrl=http://noble.gs.washington.edu/custom-tracks/fimo.transfac.description.html%20useScore=1">
    hg18</a>,
    <a
    href="http://genome.ucsc.edu/cgi-bin/hgTracks?db=hg19&position=chr21:34,119,079-34,119,784&hgct_customText=track%20type=bigBed%20name=FIMO-TRANSFAC%20description=%22FIMO%20matches%20to%20TRANSFAC%20motifs%20with%20q-value%20%3c%3d%200.1%22%20visibility=full%20bigDataUrl=http://noble.gs.washington.edu/custom-tracks/fimo.hg19.transfac-0.1.bb%20htmlUrl=http://noble.gs.washington.edu/custom-tracks/fimo.transfac.description.html%20useScore=1">
    hg19</a></li>
    </li>
    </ul>
  };

  #
  # make final form
  #
  my $form = $utils->make_submission_form(
    $utils->make_form_header($PROGRAM, "Submission form"),
    $utils->make_submission_form_top($action, $logo, $alt, $form_description),
    $required,
    $optional, 
    $utils->make_submit_button("Start search", $HELP_EMAIL),
    $utils->make_submission_form_bottom() . $custom_track_links,
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
  my $inline_motifs = $q->param('inline_motifs');
  my $motifs_file_dualvar = $q->param('motifs'); # stop perl evaluating this in array context and collapsing it to nothing
  $d{MOTIFS_DATA} = $utils->upload_motif_file($motifs_file_dualvar, $inline_motifs);
  $d{MOTIFS_ORIG_NAME} = fileparse($inline_motifs ? $q->param('inline_name') . '.meme' : $q->param('motifs'));
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

  $d{NORC} = $utils->param_bool($q, 'norc');
  $d{PV} = $utils->param_num($q, 'output_pthresh', 'p-value output threshold', 0, 1);

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
  push(@args, '-pvthresh', $data->{PV}) if (defined($data->{PV}));
  push(@args, '-norc') if ($data->{NORC});
  push(@args, $data->{MOTIFS_NAME});
  push(@args, $data->{DBSEQS_DB});

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
  my $template = HTML::Template->new(filename => 'fimo_verify.tmpl');
  $template->param(description => $data->{DESCRIPTION});

  $template->param(pv => $data->{PV});
  $template->param(norc => $data->{NORC});

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

  my $fimo = OpalServices->new(service_url => $SERVICE_URL);

  # start OPAL request
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));
  # create list of OPAL file objects
  my @infilelist = ();
  # Motif File
  push(@infilelist, InputFileType->new($data->{MOTIFS_NAME}, $data->{MOTIFS_DATA}));
  # Uploaded sequence file
  if ($data->{UPSEQS_NAME}) {
    push(@infilelist, InputFileType->new($data->{UPSEQS_NAME}, $data->{UPSEQS_DATA}));
  }
  # Description file
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
  my $result = $fimo->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);

} # submit
