#!@WHICHPERL@

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
use MemeWebUtils qw(loggable_date get_safe_name is_numeric);
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
  $logger = Log::Log4perl->get_logger('meme.cgi.gomo');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting GOMO CGI") if $logger;

# constants
$CGI::POST_MAX = 1024 * 1024 * 20; # Limit post to 20MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $PROGRAM = "GOMO";
my $BIN_DIR = "@MEME_DIR@/bin";	# binary directory
my $GOMO_SERVICE_URL = "@OPAL@/GOMO_@S_VERSION@";	
my $HELP_EMAIL = '@contact@';
my $INDEX_PATH = "@MEME_DIR@/etc/gomo_db.index";
my $QUERY_URL = "get_db_list.cgi?db_names=gomo_db.csv&mode=xml&catid=~category~";
my $DOC_URL =   "get_db_list.cgi?db_names=gomo_db.csv&mode=doc";
my $THRESHOLD_DEFAULT = 0.05;
my $THRESHOLD_MAX = 0.5;

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
# Make Significance threshold
#
sub make_significance_threshold {
  my ($indent_lvl, $tab) = @_;
  $indent_lvl = 0 unless $indent_lvl;
  $tab = "\t" unless $tab;
  my $indent = $tab x $indent_lvl;
  my $content =
      $indent."<br />\n".
      $indent."<br />\n".
      $indent."<b>Significance threshold</b>:<br />\n".
      $indent."<i>q</i>-value &le; <input class=\"maininput\" type=\"text\" ".
          "size=\"4\" name=\"threshold\" value=\"$THRESHOLD_DEFAULT\" /> ".
          "(maximum $THRESHOLD_MAX)<br />\n".
      $indent."<br />\n";
  return($content);
}

#
# Make description message
#
sub make_description_message {
  my ($indent_lvl, $tab) = @_;
  $indent_lvl = 0 unless $indent_lvl;
  $tab = "\t" unless $tab;
  my $indent = $tab x $indent_lvl;
  my $content =
      $indent."Use this form to submit motifs to <b>$PROGRAM</b>.\n".
      $indent."$PROGRAM will use the DNA binding motifs to find putative target genes\n".
      $indent."and analyze their associated GO terms. A list of significant GO terms\n".
      $indent."that can be linked to the given motifs will be produced.\n".
      $indent."<a href=\"../gomo-intro.html\">Click here for more documentation.</a>\n";
  return($content);
}

#
# print the input form
#
sub make_form {
  my ($utils, $q) = @_;
  my $tab = " ";
  my $action = "gomo.cgi";
  my $logo = "../doc/images/gomo_logo.png";
  my $alt = "$PROGRAM logo";
  my $form_description = make_description_message(11, $tab); 

  #
  # required section
  #
  my $inline_name = $q->param('inline_name');
  my $inline_motifs = $q->param('inline_motifs');

  # required left side: address, motifs
  my $motif_doc = "../doc/meme-format.html";
  my $motif_example = "../examples/sample-dna-motif.meme-io";
  my $example_type = "DNA minimal motif format file";
  my $inline_descr = "MEME motifs from sequences in file <tt>'".(defined $inline_name ? $inline_name : "")."'</tt>.";
  my $req_left = $utils->make_address_field('', '', 11, $tab);
  #we could determine the alphabet from the inline_motifs (check_params does this) 
  #but this worked fine before when this variable wasn't even decared so for now I won't change it...
  my $alphabet = undef; 
  $req_left .= $utils->make_motif_field($PROGRAM, "motifs", $inline_name, $inline_motifs, $alphabet, 
    $motif_doc, $motif_example, $inline_descr, $example_type, 11, $tab);

  # required right side: database
  my $req_right = $utils->make_supported_databases_field("database", "Sequence", $INDEX_PATH, $QUERY_URL, $DOC_URL);
  
  my $multi_species = "<a href=\"../help_multiple_genomes.html\"><b>Use multiple genomes:</b></A>";
  my $checked = "1";
  my @values = ("Whenever avaliable,1", "Single genome only<br />,0");
  $req_right .= $utils->make_radio_field($multi_species, "multispecies", $checked, \@values, 12, $tab);
 
  $req_right .= make_significance_threshold(12, $tab);

  my $required = $utils->make_input_table("", $req_left, $req_right, 8, $tab);

  # 
  # optional arguments
  #

  # optional left: description
  my $descr = "motifs";
  my $opt_left = $utils->make_description_field($descr, '');

  #
  # finally print form
  #
  my $form = $utils->make_submission_form(
    $utils->make_form_header($PROGRAM, "Submission form", "", 0, $tab),
    $utils->make_submission_form_top($action, $logo, $alt, $form_description, 2, $tab),
    $required,
    "", 
    $utils->make_submit_button("Start search", $HELP_EMAIL, 8, $tab),
    $utils->make_submission_form_bottom(6, $tab),
    $utils->make_submission_form_tailer(0, $tab),
    1,    #indent_lvl
    $tab  #tab character
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
  $d{MOTIFS_NAME} = get_safe_name($d{MOTIFS_ORIG_NAME}, 'motifs.meme', 2);
  
  # check motifs
  (undef, $d{MOTIFS_ALPHA}, $d{MOTIFS_COUNT}, $d{MOTIFS_COLS}) = 
  $utils->check_meme_motifs($d{MOTIFS_DATA}, ALPHA => 'DNA') if ($d{MOTIFS_DATA});

  # check if the search is multispecies
  $d{DB_MULTI} = $utils->param_bool($q, 'multispecies');

  # get the database
  if (defined scalar $q->param('database')) {
    my $dbid = $utils->param_require($q, 'database');
    if ($dbid =~ m/^\d+$/) {
      my @entry = load_entry($INDEX_PATH, $dbid);
      $d{DB_NAME} = $entry[CatList::DB_NAME];
      my @dbs = ($entry[CatList::DB_ROOT] . '.na');
      my @cdb_names = ();
      if ($d{DB_MULTI}) {
        for (my $i = CatList::G_COMP_ROOT; $i < scalar(@entry); $i += 2) {
          push(@dbs, $entry[$i] . '.na');
          push(@cdb_names, $entry[$i+1]);
        }
      }
      $d{CDB_NAMES} = \@cdb_names;
      $d{DBS} = \@dbs;
    } else {
      $utils->whine("You have not selected a database.");
    }
  } else {
    $utils->whine("You have not selected a database.");
  }

  # get the q-value significance threshold
  $d{QV_SIG_THRESHOLD} = $utils->param_num($q, 'threshold', 'significance threshold', 0, $THRESHOLD_MAX);

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

  push(@args, '-t', $data->{QV_SIG_THRESHOLD}) if (defined($data->{QV_SIG_THRESHOLD}));
  push(@args, $data->{MOTIFS_NAME}, @{$data->{DBS}});
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
  my $template = HTML::Template->new(filename => 'gomo_verify.tmpl');
  $template->param(description => $data->{DESCRIPTION});
  $template->param(db_name => $data->{DB_NAME});
  $template->param(db_multi => $data->{DB_MULTI});
  my @cdb_names = @{$data->{CDB_NAMES}};
  my @cdb_loop = ();
  for (my $i = 0; $i < scalar(@cdb_names); $i++) {
    push(@cdb_loop, {cdb_name => $cdb_names[$i]});
  }
  $template->param(cdb_names => \@cdb_loop);
  $template->param(qv_sig_threshold => $data->{QV_SIG_THRESHOLD});

  $template->param(motifs_name => $data->{MOTIFS_NAME});
  $template->param(motifs_orig_name => $data->{MOTIFS_ORIG_NAME});
  $template->param(motifs_safe_name => $data->{MOTIFS_NAME} ne $data->{MOTIFS_ORIG_NAME});
  $template->param(motifs_alpha => $data->{MOTIFS_ALPHA});
  $template->param(motifs_count => $data->{MOTIFS_COUNT});
  $template->param(motifs_cols => $data->{MOTIFS_COLS});
  
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

  my $gomo = OpalServices->new(service_url => $GOMO_SERVICE_URL);

  #
  # start OPAL request
  #
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));
  
  #
  # create list of OPAL file objects
  #
  my @infilelist = ();
  # 1) Motif File
  my $inputfile = InputFileType->new($data->{MOTIFS_NAME}, $data->{MOTIFS_DATA});
  push(@infilelist, $inputfile);
  # 2) Uploaded sequence file
  if ($data->{UPLOAD_NAME}) {
    $inputfile = InputFileType->new($data->{UPLOAD_NAME}, $data->{UPLOAD_DATA});
    push(@infilelist, $inputfile);
  }
  # Email address file (for logging purposes only)
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
  my $result = $gomo->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);

} # submit_to_opal

