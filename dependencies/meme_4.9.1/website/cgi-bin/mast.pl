#!@WHICHPERL@
# Author: Timothy Bailey
##
## $Id$
##
## $Log:$
## $Rev$ $Date$ $Author$

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI qw(:standard);
use Encode;
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
  $logger = Log::Log4perl->get_logger('meme.cgi.mast');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting MAST CGI") if $logger;

# setup constants
$CGI::POST_MAX = 1024 * 1024 * 10; # Limit post to 10MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $PROGRAM = "MAST";
my $BIN_DIR = "@MEME_DIR@/bin"; # directory for executables 
my $MAST_SERVICE_URL = "@OPAL@/MAST_@S_VERSION@";
my $HELP_EMAIL = '@contact@';
my $INDEX_PATH = "@MEME_DIR@/etc/fasta_db.index";
my $QUERY_URL = "get_db_list.cgi?db_names=fasta_db.csv&mode=xml&catid=~category~&short_only=1";
my $DOC_URL =   "get_db_list.cgi?db_names=fasta_db.csv&mode=doc&short_only=1";
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
# print the input form
#
sub make_form {
  $logger->trace("call make_form") if $logger;
  my ($utils, $q) = @_;

  my $action = "mast.cgi";
  my $logo = "../images/mast.png";
  my $alt = "$PROGRAM logo";
  my $form_description = "Use this form to submit motifs to <B>$PROGRAM</B> to be used in searching a sequence database.";

  #
  # required section
  #
  my $inline_name = $q->param('inline_name');
  my $inline_motifs = $q->param('inline_motifs');
  my $alphabet = undef; # Used to show DNA only options
  (undef, $alphabet) = $utils->check_meme_motifs($inline_motifs) if ($inline_motifs);

  # required left side: address, motifs
  my $motif_doc = "../mast-input.html#motif-file";
  my $motif_example = "../examples/sample-dna-motif.meme-io";
  my $example_type = "DNA motifs";
  my $inline_descr = "MEME motifs from sequences in file <TT>'".(defined $inline_name ? $inline_name : "")."'</TT>.";
  my $req_left = $utils->make_address_field();
  $req_left .= "<BR>\n";
  $req_left .= $utils->make_motif_field($PROGRAM, "motifs", $inline_name, $inline_motifs, $alphabet, 
    $motif_doc, $motif_example, $inline_descr, $example_type);

  # required right side: database

  my $sample_db = "../examples/sample-kabat.seq";
  my $req_right = $utils->make_supported_databases_field("database", "Sequence", $INDEX_PATH, $QUERY_URL, $DOC_URL);
  $req_right .= "<BR>\n<B>or</B>\n<BR>\n";
  $req_right .= $utils->make_upload_fasta_field("upload_db", $MAX_UPLOAD_SIZE, $sample_db);

  my $required = $utils->make_input_table("Required", $req_left, $req_right);

  # 
  # optional arguments
  #

  # optional left: description, ev, mev, use_seq_comp
  my $descr = "motifs";
  my $opt_left = $utils->make_description_field($descr, '');

  # make EV field
  my $ev_doc = "../mast-input.html#ev";
  my $ev_text = "<A HREF='$ev_doc'><B>Display sequences</A></B> with <I>E</I>-value below:";
  my $ev_name = "ev";
  my $ev_selected = "10";
  my @ev_list = ("0.01", "0.1", "1", "10", "20", "50", "100", "200", "500", "1000");
  $opt_left .= "<BR>\n<P>\n" .
    $utils->make_select_field2($ev_text, $ev_name, $ev_selected, @ev_list) .
    "</P>\n";

  # make MEV field
  my $mev_doc = "../mast-input.html#mev";
  my $mev_text = "<A HREF='$mev_doc'><B>Ignore motifs</A></B> if <I>E</I>-value above:";
  my $mev_name = "mev";
  my $mev_selected = ",use all motifs";
  my @mev_list = ($mev_selected, "100", "50", "20", "10", "5", "2", "1", 
    "0.5", "0.2", "0.1", "0.05", "0.02", "0.01", "0.005", "0.002", "0.001", 
    "1e-5", "1e-10", "1e-50", "1e-100");
  $opt_left .= "<P>\n" .
    $utils->make_select_field2($mev_text, $mev_name, $mev_selected, @mev_list) .
    "</P>\n";

  # make seq_comp checkbox
  my $seq_comp_doc = "../mast-input.html#use_seq_comp";
  my $seq_comp_descr = "Use individual <A HREF='$seq_comp_doc'><B> sequence composition</B></A><BR>in <I>E</I>- and <I>p</I>-value calculation";
  $opt_left .= $utils->make_checkbox("use_seq_comp", 1, $seq_comp_descr, 0);

  # optional right side: scale, translate, dna-only

  # make scale threshold checkbox
  my $use_seq_p_doc = "../mast-input.html#use_seq_p";
  my $use_seq_p_descr = "<A HREF='$use_seq_p_doc'><B>Scale motif display threshold</B></A> by sequence length";
  my $opt_right = $utils->make_checkbox("use_seq_p", 1, $use_seq_p_descr, 0);

  # make translate DNA checkbox
  my $translate_dna_doc = "../mast-input.html#dna";
  my $translate_dna_descr = "<A HREF='$translate_dna_doc'><B>Search nucleotide</B></A> database with protein motifs";
  $opt_right .= "<P>\n" .
    $utils->make_checkbox("dna", 1, $translate_dna_descr, 0) .
    "</P>\n";

  # DNA options
  if (!defined $alphabet || $alphabet eq "DNA") {
    # make strands menu
    my $strands_doc = "../mast-input.html#strands";
    my $strands_text = "Treatment of <A HREF='$strands_doc'><B>reverse complement</B></A> strands:<BR>";
    my $strands_name = "strands";
    my $strands_selected = "0,combine with given strand";
    my @strands_list = ($strands_selected, "1,treat as separate sequence", "2,none");
    my $strands_options = $utils->make_select_field2($strands_text, $strands_name, $strands_selected, @strands_list);
    $opt_right .= $utils->make_dna_only($strands_options);
  } # dna options

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

}

#
# check the parameters and whine if any are bad
#
sub check_parameters {
  $logger->trace("call check_parameters") if $logger;
  my ($utils, $q) = @_;
  my %d = ();
  my %motif_options = ();
  
  $d{TRANSLATE} = $utils->param_bool($q, 'dna');
  # translate constrains the motifs to PROTEIN and the sequences to DNA
  if ($d{TRANSLATE}) {
    $motif_options{ALPHA} = 'PROTEIN';
  }
  
  # get motifs
  my $inline_motifs = $q->param('inline_motifs');
  my $motifs_file_dualvar = $q->param('motifs'); # stop perl evaluating this in array context and collapsing it to nothing
  $d{MOTIFS_DATA} = $utils->upload_motif_file($motifs_file_dualvar, $inline_motifs);
  $d{MOTIFS_ORIG_NAME} = fileparse($inline_motifs ? $q->param('inline_name') . '.meme' : $q->param('motifs'));
  $d{MOTIFS_NAME} = get_safe_file_name($d{MOTIFS_ORIG_NAME}, 'motifs.meme', 2);
  
  # check motifs
  (undef, $d{MOTIFS_ALPHA}, $d{MOTIFS_COUNT}, $d{MOTIFS_COLS}) = 
  $utils->check_meme_motifs($d{MOTIFS_DATA}, %motif_options) if ($d{MOTIFS_DATA});

  # work out requried sequence alphabet
  my $seqs_dna = ($d{TRANSLATE} || $d{MOTIFS_ALPHA} eq 'DNA');
  my $seqs_alpha = ($seqs_dna ? 'DNA' : 'PROTEIN');

  # get the sequences
  if ($q->param('upload_db')) {
    $logger->trace("call get_sequence_data") if $logger;
    ($d{SEQS_DATA}, $d{SEQS_ALPHA}, $d{SEQS_COUNT}, $d{SEQS_MIN}, 
      $d{SEQS_MAX}, $d{SEQS_AVG}, $d{SEQS_TOTAL}) = 
    $utils->get_sequence_data('', $q->upload('upload_db'), 
      NAME => 'uploaded dataset', ALPHA => $seqs_alpha);
    # get the sequences name
    $d{SEQS_ORIG_NAME} = fileparse($q->param('upload_db'));
    $d{SEQS_NAME} = get_safe_file_name($d{SEQS_ORIG_NAME}, 'uploaded', 2);
  } elsif (defined scalar $q->param('database')) {
    my $dbid = $utils->param_require($q, 'database');
    if ($dbid =~ m/^\d+$/) {
      $logger->trace("call load_entry") if $logger;
      my @entry = load_entry($INDEX_PATH, $dbid);
      my $db_name = $entry[CatList::DB_NAME];
      # check the alphabet and length of the database
      unless (($entry[CatList::DB_NUCL] && $seqs_dna) || 
        ($entry[CatList::DB_PROT] && !$seqs_dna)) {
        $utils->whine("There is no $seqs_alpha version of database '$db_name'.");
      } elsif ($seqs_dna && !$entry[CatList::DB_SHORT]) {
        $utils->whine("The DNA sequences in database '$db_name' are too long ".
          "for searching by $PROGRAM.");
      } else {
        $d{DB_NAME} = $db_name;
        $d{DB} = $entry[CatList::DB_ROOT] . ($seqs_dna ? '.na' : '.aa');
      }
    } else {
      $utils->whine("You have not selected a database.");
    }
  } else {
    $utils->whine("You have not selected a database.");
  }

  $d{SEQP} = $utils->param_bool($q, 'use_seq_p');
  $d{COMP} = $utils->param_bool($q, 'use_seq_comp');

  $d{EV} = $utils->param_num($q, 'ev', 'sequence display evalue threshold', 0);
  $d{MEV} = $utils->param_num($q, 'mev', 'motif-ignore evalue threshold', 0);
  $d{STRANDS} = $utils->param_choice($q, 'strands', 0, 1, 2) if (defined($q->param('strands')));

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
  my @st_opts = (undef, ' -sep', ' -norc');
  # Opal is stupid and splits on any space regardless of quoting so to pass
  # a string with spaces the spaces must be encoded or they will end up at the
  # other end as seperate arguments - to avoid that this does a modified URL
  # encoding. The modification to url encoding is that anything not
  # alphanumeric, dash or dot is encoded using underscore '_' as the
  # escape character. The reason I don't use percent '%' is that DRMAA seems to
  # treat it as a special character (which is also very stupid)...
  # I considered tilde but then decided that the risk of some shell deciding to
  # expand it was too great.
  my $db_name_encoded = $data->{DB_NAME};
  $db_name_encoded = encode("utf8", $db_name_encoded); # convert to octets
  $db_name_encoded =~ s/([^A-Za-z0-9\-\.])/sprintf('_%02x', ord($1))/eg;

  my @args = ();
  push(@args, '-dna') if ($data->{TRANSLATE});
  push(@args, '-seqp') if ($data->{SEQP});
  push(@args, '-comp') if ($data->{COMP});
  push(@args, $st_opts[$data->{STRANDS}]) if (defined($st_opts[$data->{STRANDS}]));
  push(@args, '-ev', $data->{EV});
  push(@args, '-mev', $data->{MEV}) if (defined($data->{MEV}));
  push(@args, '-upload_db', $data->{SEQS_NAME}) if (defined($data->{SEQS_NAME}));
  push(@args, '-df', $db_name_encoded) unless (defined($data->{SEQS_NAME}));
  push(@args, $data->{MOTIFS_NAME});
  push(@args, $data->{DB}) unless (defined($data->{SEQS_NAME}));

  $logger->info(join(' ', @args)) if $logger;
  return @args;
}

#
# make the verification message in HTML
#
sub make_verification {
  $logger->trace("call make_verification") if $logger;
  my ($data) = @_;

  my @st_opts = ('combine with given strand', 'treat as seperate sequence', 'none');

  #open the template
  my $template = HTML::Template->new(filename => 'mast_verify.tmpl');
  $template->param(description => $data->{DESCRIPTION});
  $template->param(db_name => $data->{DB_NAME});
  if (not defined($data->{SEQS_ALPHA}) || $data->{SEQS_ALPHA} eq 'DNA') {
    $template->param(strand_treatment => $st_opts[$data->{STRANDS}]);
  }
  $template->param(ev => $data->{EV});
  $template->param(mev => $data->{MEV});
  $template->param(comp => $data->{COMP});
  $template->param(seqp => $data->{SEQP});
  $template->param(translate => $data->{TRANSLATE});

  $template->param(motifs_name => $data->{MOTIFS_NAME});
  $template->param(motifs_orig_name => $data->{MOTIFS_ORIG_NAME});
  $template->param(motifs_safe_name => $data->{MOTIFS_NAME} ne $data->{MOTIFS_ORIG_NAME});
  $template->param(motifs_alpha => $data->{MOTIFS_ALPHA});
  $template->param(motifs_count => $data->{MOTIFS_COUNT});
  $template->param(motifs_cols => $data->{MOTIFS_COLS});

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
# Submit job to OPAL
#
sub submit_to_opal
{
  $logger->trace("call submit_to_opal") if $logger;
  my ($utils, $q, $data) = @_;

  my $verify = &make_verification($data);

  my $mast = OpalServices->new(service_url => $MAST_SERVICE_URL);

  # start OPAL request
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  # create list of OPAL file objects
  my @infilelist = ();
  # Motif File
  push(@infilelist, InputFileType->new($data->{MOTIFS_NAME}, $data->{MOTIFS_DATA}));
  # Uploaded sequence file
  if ($data->{SEQS_NAME}) {
    push(@infilelist, InputFileType->new($data->{SEQS_NAME}, $data->{SEQS_DATA}));
  }
  # Description
  if ($data->{DESCRIPTION} && $data->{DESCRIPTION} =~ m/\S/) {
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
  my $result = $mast->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);

} # submit_to_opal
