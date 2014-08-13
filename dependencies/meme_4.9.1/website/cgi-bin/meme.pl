#!@WHICHPERL@
# Author: Timothy Bailey
##
## $Id$
##
## $Log:$
## $Rev$ $Date$ $Author$
## new meme form-submission file, which integrates with opal
##
##

use strict;
use warnings;

use lib qw(@PERLLIBDIR@);

use CGI qw(:standard);
use File::Basename qw(fileparse);
use MIME::Base64;
use SOAP::Lite;

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
  $logger = Log::Log4perl->get_logger('meme.cgi.meme');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting MEME CGI") if $logger;

# setup constants
$CGI::POST_MAX = 1024 * 1024 * 10; # Limit post to 10MB
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $PROGRAM = "MEME";
my $BIN_DIR = "@MEME_DIR@/bin"; # directory for executables 
my $MEME_SERVICE_URL = "@OPAL@/MEME_@S_VERSION@";
my $HELP_EMAIL = '@contact@';

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
# print the form
#
sub make_form {
  $logger->trace("call make_form") if $logger;
  my ($utils, $q) = @_;

  my $action = "meme.cgi";
  my $logo = "../images/meme.png";
  my $alt = "$PROGRAM logo";
  my $form_description = qq {
Use this form to submit DNA or protein sequences to $PROGRAM. 
$PROGRAM will analyze your sequences for similarities among them and 
produce a description (<A HREF="../meme-intro.html"><B>motif</B></A>) 
for each pattern it discovers.
  }; # end quote

  #
  # required left side: address and sequences fields
  #
  my $seq_doc = "../help_sequences.html#sequences";
  my $alpha_doc = "../help_alphabet.html";
  my $format_doc = "../help_format.html";
  my $filename_doc = "../help_sequences.html#filename";
  my $paste_doc = "../help_sequences.html#actual-sequences";
  my $sample_file = "../examples/At.fa";
  my $sample_alphabet = "Protein";
  my $target = undef; #TODO investigate what this is for and why it isn't declared!!!!
  my $req_left = $utils->make_address_field();
  $req_left .= $utils->make_upload_sequences_field("datafile", "data", $MAXDATASET,
    $seq_doc, $alpha_doc, $format_doc, $filename_doc, $paste_doc, $sample_file,
    $sample_alphabet, $target);

  #
  # required right side
  #

  # make distribution buttons
  my $distr_doc = "../meme-input.html#distribution";
  my $distr = "How do you think the occurrences of a single motif are <A HREF='$distr_doc'><B>distributed</B></A> among the sequences?\n<BR>\n";
  my $checked = "zoops";
  my @values = (
    "<B>One per sequence</B>\n<BR>,oops", 
    "<B>Zero or one</B> per sequence\n<BR>,zoops",
    "<B>Any number</B> of repetitions\n<BR>,anr"
  );
  my $req_right = $utils->make_radio_field($distr, "dist", $checked, \@values);
  #$req_right .= "<B>Note:</B>The maximum number of occurrences of a motif is limited to $MAXSITES.";

  # make width fields
  $req_right .= qq {
<BR>
<!-- width fields -->
<BR>
$PROGRAM will find the optimum <A HREF="../help_width.html#motif_width"><B>width</B></A> 
of each motif within the limits you specify here: 
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=3 NAME="minw" VALUE=6>
<B>Minimum</B> width (>= $MINW)
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=3 NAME="maxw" VALUE=50>
<B><A HREF="../help_width.html#max_width">Maximum</B></A> width (<= $MAXW)
<BR>
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=2 NAME="nmotifs" VALUE=3>
Maximum <A HREF="../meme-input.html#nmotifs"><B>number of motifs</B></A>
to find
  }; # end quote

  # finish required fields
  my $required = $utils->make_input_table("Required", $req_left, $req_right);

  #
  # optional fields
  #

  # optional left: description, nsites, shuffle fields 
  my $descr = "sequences";
  my $opt_left = $utils->make_description_field($descr, "");

  # make nsites fields
  $opt_left .= qq {
<BR>
<!-- nsites fields -->
<BR>
$PROGRAM will find the optimum 
<A HREF="../meme-input.html#nsites"><B>number of sites</B></A>
for each motif within the limits you specify here: 
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=3 NAME="minsites" VALUE="">
<B>Minimum</B> sites (>= $MINSITES)
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=3 NAME="maxsites" VALUE="">
<B>Maximum</B> sites (<= $MAXSITES)
<BR>
<BR>
  }; # end quote
  # shuffle checkbox
  my $doc = "../help_filters.html#shuffle";
  my $text = "<A HREF='$doc'><B>Shuffle</B></A> sequence letters";
  $opt_left .= $utils->make_checkbox("shuffle", 1, $text, 0);

  # optional right fields: bfile, shuffle, dna-only: strand, pal, PSP

  my $opt_right .= qq {
Perform <A HREF="../help_psp.html#discriminative"><B>discriminative</B></A>
motif discovery &ndash; Enter the name of a file containing
&lsquo;<A HREF="../help_psp.html#negativeset"><B>negative sequences</B>&rsquo;</A>:
<BR>
<div id="upload_neg_div">
  <input class="maininput" name="upload_negfile" type="file" size=18>
  <a onclick="clearFileInputField('upload_neg_div')" href="javascript:noAction();"><b>Clear</b></a>
</div>
<br><br>
Enter the name of a file containing a 
<A HREF="../meme-input.html#bfile"><B>background Markov model</B></A>:
<BR>
<div id="upload_bfile_div">
  <input class="maininput" name="upload_bfile" type="file" size=18>
  <a onclick="clearFileInputField('upload_bfile_div')" href="javascript:noAction();"><b>Clear</b></a>
</div>
  }; # end quote

  my $psp_doc = "../help_psp.html#psp";

  my $triples_doc =  "../help_psp.html#triples";

  # DNA-ONLY options (there is no definition of alphabet anywhere that I can find hence this will always run)
  #if (!defined $alphabet || $alphabet eq "ACGT") 
  {
    my ($doc, $text, $options);
    $doc = "../meme-input.html#posonly";
    $text = "Search given <A HREF='$doc' <B>strand</B></A> only";
    $options = $utils->make_checkbox("posonly", 1, $text, 0); 
    $options .= "<BR>\n";
    $doc = "../meme-input.html#pal";
    $text = "Look for <A HREF='$doc'><B>palindromes</B></A> only";
    $options .= $utils->make_checkbox("pal", 1, $text, 0); 
    $opt_right .= $utils->make_dna_only($options);
  } # dna options

  my $optional = $utils->make_input_table("Options", $opt_left, $opt_right, undef, undef); # add 1 at end to make visibility toggle
  
  # filtering options
  my $filter_purge = "";
  my $filter_dust = "";

  $filter_purge = qq {
<!-- purge fields -->
To use purge to remove similar sequences specify the maximum 
<A HREF="http://folk.uio.no/einarro/Presentations/blosum62.html"><B>blosum62
relatedness score</B></A> (100-200 recommended): 
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=3 NAME="purgescore" VALUE="">
<BR>
<BR>     
  }; # end quote

  $filter_dust = qq {
<!-- purge fields -->
To use <A HREF="http://www.liebertonline.com/doi/abs/10.1089/cmb.2006.13.1028"><B>dust</B></A>
to filter low complexity regions from your sequences set the cut-off
 (10-20 recommended): 
<BR>
<INPUT CLASS="maininput" TYPE="TEXT" SIZE=3 NAME="dustcutoff" VALUE="">
<BR>
<BR>     
  }; # end quote

  # add the dust and purge input area
  my $filters = $utils->make_input_table("Filters", $filter_purge, $filter_dust);
  

  #
  # print final form
  #
  my $form = $utils->make_submission_form(
    $utils->make_form_header($PROGRAM, "Submission form"),
    $utils->make_submission_form_top($action, $logo, $alt, $form_description),
    $required, 
    #$optional.$filters, 
    $optional, 
    $utils->make_submit_button("Start search", $HELP_EMAIL),
    $utils->make_submission_form_bottom(),
    $utils->make_submission_form_tailer()
  );

  return $form;
  
} # print_form

#
# check the parameters and whine if any are bad
#
sub check_parameters {
  $logger->trace("call check_parameters") if $logger;
  my ($utils, $q) = @_;
  my %d = ();
  my $alpha;
  my $name;
  my $shuffle;

  $shuffle = $utils->param_bool($q, 'shuffle');

  # get the input sequences
  ($d{SEQS_DATA}, $alpha, $d{SEQS_COUNT}, $d{SEQS_MIN}, 
    $d{SEQS_MAX}, $d{SEQS_AVG}, $d{SEQS_TOTAL}) = 
  $utils->get_sequence_data(
    scalar $q->param('data'), scalar $q->param('datafile'),
    MAX => $MAXDATASET, SHUFFLE => $shuffle);
  # get the input sequences name
  $name = 'sequences';
  $d{SEQS_ORIG_NAME} = ($q->param('data') ? $name : fileparse($q->param('datafile')));
  $d{SEQS_NAME} = get_safe_name($d{SEQS_ORIG_NAME}, $name, 2);

  # get the motif distribution mode
  $d{MODE} = $utils->param_choice($q, 'dist', 'oops', 'zoops', 'anr');

  # get the motif width bounds
  $d{MINW} = $utils->param_int($q, 'minw', 'minimum width', $MINW, $MAXW, 6);
  $d{MAXW} = $utils->param_int($q, 'maxw', 'maximum width', $MINW, $MAXW, 50);
  if ($d{MINW} > $d{MAXW}) {
    $utils->whine("The minimum width (" . $d{MINW} . ") must be less than ".
      "or equal to the maximum width (" . $d{MAXW} . ").");
  }

  # get the motif count bound
  $d{NMOTIFS} = $utils->param_int($q, 'nmotifs', 'maximum motifs', 1, undef, 3);

  # get the optional description
  $d{DESCRIPTION} = $utils->param_require($q, 'description');

  # check for optional site bounds
  $d{MINSITES} = $utils->param_int($q, 'minsites', 'minimum sites', $MINSITES, $MAXSITES);
  $d{MAXSITES} = $utils->param_int($q, 'maxsites', 'maximum sites', $MINSITES, $MAXSITES);
  if (defined($d{MINSITES}) && defined($d{MAXSITES}) && $d{MINSITES} > $d{MAXSITES}) {
    $utils->whine("The minimum sites (" . $d{MINSITES} . ") must be less than ".
      "or equal to the maximum sites (" . $d{MAXSITES} . ").");
  }

  # check for optional negative sequences
  if ($q->param('upload_negfile')) {
    $utils->whine("Any number of repetitions not allowed ".
              "for discriminative motif discovery") if($d{MODE} eq "anr");
    # get the negative sequences
    ($d{NEGS_DATA}, undef, $d{NEGS_COUNT}, $d{NEGS_MIN}, 
      $d{NEGS_MAX}, $d{NEGS_AVG}, $d{NEGS_TOTAL}) = 
    $utils->get_sequence_data(undef, scalar $q->param('upload_negfile'), 
      ALPHA => $alpha, MAX => $MAXDATASET*4, NAME => "negative dataset");
    # get the negative sequences name
    $d{NEGS_NAME} = get_safe_file_name(
      scalar $q->param('upload_negfile'), 'negative_sequences', 2);
  }

  # check for optional background Markov model
  if ($q->param('upload_bfile')) {
    my $file = $q->upload('upload_bfile');
    $d{BFILE_DATA} = do {local $/; <$file>};
    $d{BFILE_DATA} =~ s/\r\n/\n/g; # Windows -> UNIX eol
    $d{BFILE_DATA} =~ s/\r/\n/g; # MacOS -> UNIX eol
    $d{BFILE_NAME} = get_safe_file_name(
      scalar $q->param('upload_bfile'), 'uploaded_bfile', 2);
  }

  # check for DNA options
  $d{POSONLY} = $utils->param_bool($q, 'posonly');
  $d{PAL} = $utils->param_bool($q, 'pal');
  $d{SHUFFLE} = $shuffle;

  # get the email
  $d{EMAIL} = $utils->param_email($q);
  
  # set alpha
  $d{ALPHA} = lc($alpha);

  return \%d;
}

#
# make the arguments list
#
sub make_arguments {
  $logger->trace("call make_arguments") if $logger;
  my ($data) = @_;
  my @args = ('-alpha', $data->{ALPHA}, '-mod', $data->{MODE}, 
    '-nmotifs', $data->{NMOTIFS}, '-minw', $data->{MINW}, 
    '-maxw', $data->{MAXW});
  push(@args, '-minsites', $data->{MINSITES}) if (defined($data->{MINSITES}));
  push(@args, '-maxsites', $data->{MAXSITES}) if (defined($data->{MAXSITES}));
  push(@args, '-bfile', $data->{BFILE_NAME}) if (defined($data->{BFILE_NAME}));
  push(@args, '-neg', $data->{NEGS_NAME}) if (defined($data->{NEGS_NAME}));
  push(@args, '-norevcomp') if ($data->{POSONLY});
  push(@args, '-pal') if ($data->{PAL});
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

  my %motif_occur = (zoops => 'Zero or one per sequence', 
    oops => 'One per sequence', anr => 'Any number of repetitions');

  #open the template
  my $template = HTML::Template->new(filename => 'meme_verify.tmpl');

  #fill in parameters
  $template->param(description => $data->{DESCRIPTION});
  $template->param(motif_occur => $motif_occur{$data->{MODE}});
  $template->param(nmotifs => $data->{NMOTIFS});
  $template->param(minsites => $data->{MINSITES});
  $template->param(maxsites => $data->{MAXSITES});
  $template->param(minw => $data->{MINW});
  $template->param(maxw => $data->{MAXW});
  $template->param(alpha => $data->{ALPHA});
  $template->param(posonly => $data->{POSONLY});
  $template->param(pal => $data->{PAL});
  $template->param(shuffle => $data->{SHUFFLE});
  $template->param(bfile_name => $data->{BFILE_NAME});

  $template->param(seqs_name => $data->{SEQS_NAME});
  $template->param(seqs_count => $data->{SEQS_COUNT});
  $template->param(seqs_min => $data->{SEQS_MIN});
  $template->param(seqs_max => $data->{SEQS_MAX});
  $template->param(seqs_avg => $data->{SEQS_AVG});
  $template->param(seqs_total => $data->{SEQS_TOTAL});

  if ($data->{NEGS_NAME}) {
    $template->param(negs_name => $data->{NEGS_NAME});
    $template->param(negs_count => $data->{NEGS_COUNT});
    $template->param(negs_min => $data->{NEGS_MIN});
    $template->param(negs_max => $data->{NEGS_MAX});
    $template->param(negs_avg => $data->{NEGS_AVG});
    $template->param(negs_total => $data->{NEGS_TOTAL});
  }
  return $template->output;
}

#
# Submit job to webservice via OPAL
#
sub submit_to_opal {
  $logger->trace("call submit_to_opal") if $logger;
  my ($utils, $q, $data) = @_;

  # make the verification form and email message
  my $verify = &make_verification($data);

  my $service = OpalServices->new(service_url => $MEME_SERVICE_URL);

  # start OPAL requst
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  #
  # create list of OPAL file objects
  #
  my @infilelist = ();
  # Sequence file
  push(@infilelist, InputFileType->new($data->{SEQS_NAME}, $data->{SEQS_DATA}));
  # Background file
  if ($data->{BFILE_NAME}) {
    push(@infilelist, InputFileType->new($data->{BFILE_NAME}, $data->{BFILE_DATA}));
  }
  # PSP negative set file
  if ($data->{NEGS_NAME}) {
    push(@infilelist, InputFileType->new($data->{NEGS_NAME}, $data->{NEGS_DATA}));
  }
  # Description file
  if ($data->{DESCRIPTION} && $data->{DESCRIPTION} =~ m/\S/) {
    push(@infilelist, InputFileType->new("description", $data->{DESCRIPTION}));
  }
  # Email address file
  push(@infilelist, InputFileType->new("address_file", $data->{EMAIL}));
  # Submit time file
  push(@infilelist, InputFileType->new("submit_time_file", loggable_date()));

  # Add file objects to request
  $req->setInputFile(@infilelist);

  # output HTTP header
  print $q->header;

  # Submit the request to OPAL
  $logger->trace("Launching MEME Job") if $logger;
  my $result = $service->launchJob($req);

  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);
}
