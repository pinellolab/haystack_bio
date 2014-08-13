#!@WHICHPERL@

use strict;
use warnings;

use CGI qw(:standard);
use Cwd qw(abs_path);
use Fcntl qw(O_CREAT O_WRONLY);
use File::Basename qw(fileparse);
use File::Copy qw(cp);
use File::Spec::Functions qw(catfile);
use File::Temp qw(tempdir);
use HTML::Template;

use lib qw(@PERLLIBDIR@);

use CatList qw(open_entries_iterator load_entry);
use MemeWebUtils qw(get_safe_name add_status_msg update_status loggable_date);
use MotifUtils qw(seq_to_intern matrix_to_intern intern_to_meme motif_id motif_width);
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
  $logger = Log::Log4perl->get_logger('meme.cgi.tomtom');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting TOMTOM CGI") if $logger;

#defaults
$CGI::POST_MAX = 1024 * 1024 * 10; # 10MB upload limit
$CGI::DISABLE_UPLOADS = 0; # Allow file uploads
my $MOTIF_DB_INDEX = '@MEME_DIR@/etc/motif_db.index';
my $VERSION = '@S_VERSION@';
my $PROGRAM = 'TOMTOM';
my $HELP_EMAIL = '@contact@';
my $BIN_DIR = '@MEME_DIR@/bin';
my $TOMTOM_OUTPUT_DIR = "../output"; # directory for results
my $TOMTOM_SERVICE_URL = "@OPAL@/TOMTOM_@S_VERSION@";

&process_request();
exit(0);

#
# Checks for the existance of the cgi parameter search
# and invokes the approprate response
#
sub process_request {
  $logger->trace("call process_request") if $logger;
  my $q = CGI->new;
  my $utils = new MemeWebUtils($PROGRAM, $BIN_DIR);
  #decide what to do
  my $search = $q->param('search');

  if (not defined $search) {
    display_form($utils, $q);
  } else {
    my $data = check_parameters($utils, $q);
    if ($utils->has_errors()) {
      $utils->print_tailers();
    } else {
      if ($data->{ENQUEUE}) {
        submit_to_opal($utils, $q, $data);
      } else {
        run_on_wserver($utils, $q, $data);
      }
    }
  }
}

#
# display_form
#
# Displays the tomtom page.
#
sub display_form {
  $logger->trace("call display_form") if $logger;
  my ($utils, $q) = @_;
  
  #there may be inline motifs
  my $inline_motifs = $q->param('inline_motifs');
  my $inline_name = $q->param('inline_name');
  my $inline_count = 0;

  if ($inline_motifs) {
    my ($found, $alpha, $matricies, $columns) = $utils->check_meme_motifs($inline_motifs);
    $inline_count = $matricies;
  }

  #open the template
  my $template = HTML::Template->new(filename => 'tomtom.tmpl');

  #fill in parameters
  $template->param(version => $VERSION);
  $template->param(help_email => $HELP_EMAIL);
  $template->param(inline_motifs => $inline_motifs);
  $template->param(inline_name => $inline_name);
  $template->param(inline_count => $inline_count);

  #create the parameter for the database selection
  my $iter = open_entries_iterator($MOTIF_DB_INDEX, 0); #load the first category
  my @dbs_param = ();
  my $first = 1; #TRUE
  while ($iter->load_next()) { #auto closes
    my %row_data = (db_id => $iter->get_index(), 
      db_name => ($iter->get_entry())[CatList::DB_NAME],
      selected => $first);
    $first = 0; #FALSE
    push(@dbs_param, \%row_data);
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
# consensus_and_matrix_motif 
#
# Reads and checks the consensus or matrix motif parameters
#
sub consensus_and_matrix_motif {
  $logger->trace("call consensus_and_matrix_motif") if $logger;
  my ($utils, $q, $motif_src) = @_;
  my ($intern, $errors);
  my ($iupac, $origname, $name, $data, $count, $cols);
  if ($motif_src eq 'consensus') {
    # consensus motif parameters
    my $sequence = $utils->param_require($q, 'consensus_sequence');
    my $sites = $utils->param_num($q, 'consensus_sites', 'sites', 1, undef, 20);
    my $pseudo = $utils->param_num($q, 'consensus_pseudocount', 'pseudocount', 0, undef, 1);
    my $bg = $utils->param_dna_bg($q, 'consensus_background_', 'background letter');
    # convert into internal format ignoring parsing errors
    ($intern) = seq_to_intern($bg, $sequence, $sites, $pseudo);
    $errors = ["The IUPAC query motif you specified did not contain any ".
      "IUPAC letter codes."] unless $intern;
  } elsif ($motif_src eq 'matrix') {
    # matrix motif parameters
    my $matrix = $utils->param_require($q, 'matrix_data');
    my $orient = $utils->param_choice($q, 'matrix_orientation', 'row', 'col', 'auto');
    my $sites = $utils->param_num($q, 'matrix_sites', 'sites', 1, undef, 20);
    my $pseudo = $utils->param_num($q, 'matrix_pseudocount', 'pseudocount', 0, undef, 1);
    my $bg = $utils->param_dna_bg($q, 'matrix_background_', 'background letter');
    # convert into internal format
    ($intern, $errors) = matrix_to_intern($bg, $matrix, $orient, $sites, $pseudo);
  }
  if ($intern) {
    $iupac = motif_id($intern);
    $name = motif_id($intern) . '.meme';
    $data = intern_to_meme($intern, 0, 1, 1);
    $count = 1;
    $cols = motif_width($intern);
  } else {
    $utils->whine(@$errors);
  }
  return ($iupac, $name, $name, $data, $count, $cols);
}

#
# meme_and_inline_motifs 
#
# Reads and checks the meme or inline motif parameters
#
sub meme_and_inline_motifs {
  $logger->trace("call meme_and_inline_motifs") if $logger;
  my ($utils, $q, $motif_src) = @_;
  my ($origname, $name, $data, $count, $cols);
  if ($motif_src eq 'meme') {
    # meme motif parameter
    my $file = $q->upload('meme_file');
    if (defined($file)) {
      $data = do { local $/;  <$file> };
      close($file);
      $name = $q->param('meme_file');
    } else {
      $utils->whine("Query motifs file not supplied.");
    }
  } elsif ($motif_src eq 'inline') {
    # inline motif parameters
    $data = $utils->param_require($q, 'inline_motifs');
    $name = $utils->param_require($q, 'inline_name');
  }
  if ($data) {
    $origname = fileparse($name);
    $name = get_safe_name($origname, 'query.meme', 0);
    (undef, undef, $count, $cols) = 
    $utils->check_meme_motifs($data, ALPHA => 'DNA', NAME => 'query motifs');
  } else {
    $data = undef;
    $name = undef;
  }
  return ($origname, $name, $data, $count, $cols);
}

#
# check_parameters
#
# Reads and checks the parameters in preperation for calling tomtom.
#
sub check_parameters {
  $logger->trace("call check_parameters") if $logger;
  my ($utils, $q) = @_;
  my %d = ();

  # get the query motifs
  my $src = $utils->param_choice($q, 
    'motif_src', 'consensus', 'matrix', 'meme', 'inline');
  $d{QUERIES_SRC} = $src;
  if ($src eq 'consensus' || $src eq 'matrix') {
    ($d{QUERIES_IUPAC}, $d{QUERIES_ORIG_NAME}, $d{QUERIES_NAME}, 
      $d{QUERIES_DATA}, $d{QUERIES_COUNT}, $d{QUERIES_COLS}, $d{QUERIES_PSPM}) =
    &consensus_and_matrix_motif($utils, $q, $src);
  } elsif ($src eq 'meme' || $src eq 'inline') {
    ($d{QUERIES_ORIG_NAME}, $d{QUERIES_NAME}, 
      $d{QUERIES_DATA}, $d{QUERIES_COUNT}, $d{QUERIES_COLS}) = 
    &meme_and_inline_motifs($utils, $q, $src);
  }
  # get the filter
  if ($src eq 'meme') {
    ($d{FILTER_COUNT}, $d{FILTER_INDEXES}, $d{FILTER_NAMES}) = 
    $utils->param_filter($q, 'meme_ignore_filter', 'meme_filter', 'motif');
  } else { # no filters
    ($d{FILTER_COUNT}, $d{FILTER_INDEXES}, $d{FILTER_NAMES}) = (0, [], []);
  }
  # get the motif database
  ($d{DBTARGETS_NAME}, $d{DBTARGETS_DBS}, $d{UPTARGETS_ORIG_NAME}, 
    $d{UPTARGETS_NAME}, $d{UPTARGETS_DATA}, $d{UPTARGETS_ALPHA}, 
    $d{UPTARGETS_COUNT}, $d{UPTARGETS_COLS}) = $utils->param_motif_db($q);
  # get the comparison function to use
  $d{CMP_FN} = $utils->param_choice($q, 'comparison_function', 'pearson', 'ed', 'sandelin');
  # get the motif significance threshold
  if ($utils->param_bool($q, 'thresh_type')) { # E-value threshold
    $d{E_THRESH} = $utils->param_num($q, 'thresh', 'E-value threshold', 0);
  } else { # q-value threshold
    $d{Q_THRESH} = $utils->param_num($q, 'thresh', 'q-value threshold', 0, 1);
  }
  $d{COMPLETE_SCORING} = $utils->param_bool($q, 'complete_scoring');
  # get the job description
  $d{DESCRIPTION} = $utils->param_require($q, 'job_description');
  # determine if tomtom should be run on the webserver or queued to run as a webservice
  $d{ENQUEUE} = (defined($d{UPTARGETS_NAME}) || ($d{QUERIES_COUNT} > 1 && $d{FILTER_COUNT} != 1));
  # get the user's email address if it is required
  $d{EMAIL} = $utils->param_email($q) if ($d{ENQUEUE});

  return \%d;
}


#
# make the arguments list
#
sub make_arguments {
  $logger->trace("call make_arguments") if $logger;
  my ($data) = @_;
  my @args = ();
  push(@args, '-niced') unless ($data->{ENQUEUE});
  if ($data->{FILTER_COUNT}) {
    push(@args, (map {('-m', $_)} @{$data->{FILTER_NAMES}}));
    push(@args, (map {('-mi', $_)} @{$data->{FILTER_INDEXES}}));
  } elsif (not $data->{ENQUEUE}) {
    push(@args, '-mi', 1);
  }
  push(@args, '-ev', $data->{E_THRESH}) if (defined($data->{E_THRESH}));
  push(@args, '-qv', $data->{Q_THRESH}) if (defined($data->{Q_THRESH}));
  push(@args, '-dist', $data->{CMP_FN}) if (defined($data->{CMP_FN}));
  push(@args, '-incomplete_scores') unless($data->{COMPLETE_SCORING});
  push(@args, '-uptargets', $data->{UPTARGETS_NAME}) if (defined($data->{UPTARGETS_NAME}));
  push(@args, $data->{QUERIES_NAME});
  push(@args, $data->{DBTARGETS_DBS}) unless (defined($data->{UPTARGETS_NAME}));

  $logger->info(join(' ', @args)) if $logger;
  return @args;
}

#
# make the position into a string
# so 1 becomes 1st, 2 becomes 2nd, 3 becomes 3rd, etc...
#
sub pos2str {
  my ($num) = @_;
  my $digit = substr($num, -1, 1);
  if ($num == 11) {
    return "11th";
  } elsif ($num == 12) {
    return "12th";
  } elsif ($num == 13) {
    return "13th";
  } elsif ($digit eq '1') {
    return $digit.'st';
  } elsif ($digit eq '2') {
    return $digit.'nd';
  } elsif ($digit eq '3') {
    return $digit.'rd';
  } else {
    return $digit.'th';
  }
}

#
# make the verification message in HTML
#
sub make_verification {
  $logger->trace("call make_verification") if $logger;
  my ($data) = @_;

  my %cmp_fn_name = (pearson => 'Pearson correlation coefficient', ed => 'Euclidean distance', 
    sandelin => 'Sandelin-Wasserman similarity');

  #open the template
  my $template = HTML::Template->new(filename => 'tomtom_verify.tmpl');
  $template->param(description => $data->{DESCRIPTION});

  $template->param(ev => $data->{E_THRESH}) if (defined($data->{E_THRESH}));
  $template->param(qv => $data->{Q_THRESH}) if (defined($data->{Q_THRESH}));
  $template->param(cmp_fn_name => $cmp_fn_name{$data->{CMP_FN}});
  $template->param(complete_scoring => $data->{COMPLETE_SCORING});

  $template->param(queries_name => $data->{QUERIES_NAME});
  $template->param(queries_orig_name => $data->{QUERIES_ORIG_NAME});
  $template->param(queries_safe_name => $data->{QUERIES_NAME} ne $data->{QUERIES_ORIG_NAME});
  $template->param(queries_count => $data->{QUERIES_COUNT});
  $template->param(queries_cols => $data->{QUERIES_COLS});

  $template->param(filter_names => [map { {name => $_} } @{$data->{FILTER_NAMES}}]);
  $template->param(filter_positions => [map { {position => &pos2str($_)} } @{$data->{FILTER_INDEXES}}]);

  $template->param(dbtargets_name => $data->{DBTARGETS_NAME});
  $template->param(uptargets_name => $data->{UPTARGETS_NAME});
  $template->param(uptargets_orig_name => $data->{UPTARGETS_ORIG_NAME});
  $template->param(uptargets_safe_name => $data->{UPTARGETS_NAME} ne $data->{UPTARGETS_ORIG_NAME});
  $template->param(uptargets_count => $data->{UPTARGETS_COUNT});
  $template->param(uptargets_cols => $data->{UPTARGETS_COLS});
  return $template->output;
}

#
# submit_to_opal
#
# Submits the job to a queue which will probably run on a cluster.
#
sub submit_to_opal {
  my ($utils, $q, $data) = @_;
  
  my $verify = &make_verification($data);

  # get a handle to the webservice
  my $service = OpalServices->new(service_url => $TOMTOM_SERVICE_URL);

  # create OPAL requst
  my $req = JobInputType->new();
  $req->setArgs(join(' ', &make_arguments($data)));

  # create list of OPAL file objects
  my @infilelist = ();
  # Query file
  push(@infilelist, InputFileType->new($data->{QUERIES_NAME}, $data->{QUERIES_DATA}));
  # Uploaded target database
  push(@infilelist, InputFileType->new($data->{UPTARGETS_NAME}, $data->{UPTARGETS_DATA})) if $data->{UPTARGETS_NAME};
  # Description
  if ($data->{DESCRIPTION} =~ m/\S/) {
    push(@infilelist, InputFileType->new('description', $data->{DESCRIPTION}));
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
  my $result = $service->launchJob($req);

  # Give user the verification form and email message
  $utils->verify_opal_job($result, $data->{EMAIL}, $HELP_EMAIL, $verify);
}

#
# sends a redirect request
#
sub send_redirect {
  my ($q, $dir_rel) = @_;
  my $namelen = length($q->url(-relative=>1));
  #turn off buffering
  $| = 1;
  print $q->redirect(substr($q->url(-path_info=>1),-$namelen,-$namelen) .$dir_rel.'/index.html');
}

#
# creates a file in the current directory
#
sub create_file {
  my ($file_name, $data) = @_;
  my $fh;
  sysopen($fh, $file_name, O_CREAT | O_WRONLY, 0777); #too permissive?
  print $fh $data;
  close($fh);
}

#
# run_on_wserver
#
# Runs tomtom on the webserver.
#
sub run_on_wserver {
  my ($utils, $q, $data) = @_;

  # create a directory for the results
  my $dir_rel = tempdir('tomtom_XXXXXXXX', DIR => $TOMTOM_OUTPUT_DIR);
  my $dir = abs_path($dir_rel);
  chmod 0777, $dir; # I'm suspicious that the 0777 may be too permissive (but that's what we had before)
  chdir($dir);
  # create a file to view
  my $msg_list = [];
  update_status('index.html', 'Tomtom', 3, [], add_status_msg('Please wait', $msg_list));
  # create the motif file to be searched
  &create_file($data->{QUERIES_NAME}, $data->{QUERIES_DATA});
  # create the database file to be searched
  &create_file($data->{UPTARGETS_NAME}, $data->{UPTARGETS_DATA}) if (defined($data->{UPTARGETS_NAME}));
  # send redirect
  send_redirect($q, $dir_rel);
  # call the webservice script directly
  my $out = 'stdout.txt';
  my $err = 'stderr.txt';
  if (fork() == 0) {# child process
    # close file descriptors
    close(STDOUT);
    close(STDIN);
    close(STDERR);
    my @TOMTOM_WS_ARGS = &make_arguments($data);
    # run webservice, note can't use ExecUtils because need to close STDIN and STDERR
    my $exe = catfile($BIN_DIR, 'tomtom_webservice');
    open(STDOUT, '>', $out) or die('Failed to redirect stdout');
    open(STDERR, '>', $err) or die('Failed to redirect stderr');
    my $result = system($exe, @TOMTOM_WS_ARGS);
    close(STDOUT);
    close(STDERR);
    # check the result for errors
    if ($result != 0) {
      # make a copy of the index in case it had useful messages
      cp('index.html', 'index_old.html');
      # create a new index, describing the problem
      my $file_list = [{file => 'index_old.html', desc => 'Old index'},
          {file => $out, desc => 'Processing messages'},
          {file => $err, desc => 'Error messages'}];
      if ($result == -1) {
        add_status_msg('Tomtom webservice failed to run', $msg_list);
      } elsif ($result & 127) {
        add_status_msg("Tomtom webservice process died with signal ".
          ($result & 127).", ".(($result & 128) ? 'with' : 'without').
          " coredump", $msg_list);
      } else {
        add_status_msg("Tomtom webservice exited with error code ".($result >> 8), $msg_list);
      }
      update_status('index.html', 'Tomtom', 0, $file_list, $msg_list);
    }
 }
}
