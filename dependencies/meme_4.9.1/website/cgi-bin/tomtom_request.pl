#!@WHICHPERL@ -w

use strict;
use CGI;
use CGI::Carp qw(fatalsToBrowser);
use Fcntl qw(SEEK_SET);
use File::Spec::Functions qw(catfile tmpdir);
use File::Temp qw(tempfile);
use List::Util qw(sum);

use lib qw(@PERLLIBDIR@);

use ExecUtils qw(invoke);
use MemeWebUtils qw(eps_to_png);

# globals
$CGI::POST_MAX = 1024 * 1024 * 5; # 5MB upload limit
$CGI::DISABLE_UPLOADS = 1; # Disallow file uploads
my $tmpdir = '@TMP_DIR@';
$tmpdir = &tmpdir() if ($tmpdir eq '' || $tmpdir =~ m/^\@TMP[_]DIR\@$/);
my $bin = "@MEME_DIR@/bin";
my $gs = '@WHICHGHOSTSCRIPT@';
$gs = 'gs' if $gs =~ m/\@WHICH[G]HOSTSCRIPT\@/; # The makefile needs to be updated but I can't do that in a patch
my $q = CGI->new;
dispatch();

#
# dispatch the handling to the approprate
# subroutine dependent on the mode parameter
#
sub dispatch {
  my $mode = $q->param('mode');
  if (not defined $mode) {
    unknown_request();
  } else {
    if ($mode =~ m/^logo$/i) {
      tomtom_logo();
    } else {
      unknown_request();
    }
  }
}

sub unknown_request {
  print $q->header,
        $q->start_html('Unrecognised request'),
        $q->h1('Unrecognised request'),
        $q->p('The value specified for the parameter mode tells this script what to do. '.
            'The value that was specified was unrecognised and so this script doesn\'t know what to do.');
  print $q->h2('Parameters');
  my @rows = $q->Tr($q->th('name'), $q->th('value'));
  my $param;
  foreach $param ($q->param) {
    push @rows, $q->Tr($q->td($param), $q->td($q->pre($q->param($param))));
  }
  print $q->table(@rows);
  print $q->end_html;
}

sub get_param_with_default {
  my ($q, $name, $default) = @_;
  my $param = $q->param($name);
  if (defined $param) {
    return $param;
  } else {
    return $default;
  }
}

sub validate_param {
  my ($q, $name, $match) = @_;
  my $param = $q->param($name);
  die("Missing required parameter $name.\n") unless defined $param;
  die("Parameter $name failed validation.\n") unless $param =~ m/$match/;
  return $param;
}

sub program_error {
  my ($q, $program, $status) = @_;
  # check status
  if ($status == -1) {
    die("$program failed to run.");
  } elsif ($status & 127) {
    die( 
      sprintf("%s died with signal %d, %s coredump.",
        $program, ($status & 127), ($status & 128) ? 'with' : 'without'
      )
    );
  } elsif ($status != 0) {
    die( 
      sprintf("%s exited with value %d indicating failure.", $program, $? >> 8)
    );
  }
}

sub get_pos_num {
  my ($txt) = @_;
  if ($txt =~ m/^\s*(\d+(.\d+)?)\s*$/) {
    return $1;
  } else {
    return 0;
  }
}

sub unlink1 {
  my ($fh, $filename) = @_;
  close($fh);
  unlink($filename);
}

sub tomtom_logo {
  # version and background parameters
  my $version = &validate_param($q, 'version', qr/^\d+(\.\d+(\.\d+)?)?$/);
  my $bgline = &parse_background($q->param('background')); 

  # target motif parameters
  my $target_id = &validate_param($q, 'target_id', qr/^\S+$/);
  my $target_length = &validate_param($q, 'target_length', qr/^\d+$/);
  my $target_pspm = &validate_param($q, 'target_pspm', qr/.+/);
  my $target_rc = &validate_param($q, 'target_rc', qr/^[01]$/);
  
  # query motif parameters
  my $query_id = &validate_param($q, 'query_id', qr/^\S+$/);
  my $query_length = &validate_param($q, 'query_length', qr/^\d+$/);
  my $query_pspm = &validate_param($q, 'query_pspm', qr/.+/);
  my $query_offset = &validate_param($q, 'query_offset', qr/^[-]?\d+$/);

  # user selected parameters
  my $image_type = &validate_param($q, 'image_type', qr/^(png|eps)$/);
  my $error_bars = &validate_param($q, 'error_bars', qr/^[01]$/);
  my $ssc = &validate_param($q, 'small_sample_correction', qr/^[01]$/);
  my $flip = &validate_param($q, 'flip', qr/^[01]$/);
  my $image_width = &get_pos_num(&get_param_with_default($q, 'image_width', 0));
  my $image_height = &get_pos_num(&get_param_with_default($q, 'image_height', 0));

  # query offset
  my $q_off = ($flip ? $target_length - ($query_length + $query_offset) : $query_offset);
  # fine descriptive text
  my $fine_text = "Tomtom " . &date_time_string();
  # mime type
  my $mime_type = "application/postscript";

  # write the query and target motifs to temporary files
  my ($q_fh, $q_nam) = &tempfile('motif_query_XXXXXXXXXX', DIR => $tmpdir, SUFFIX => '.meme', UNLINK => 1);
  print $q_fh &meme_nucleotide_motif($version, $bgline, $query_id, $query_pspm);
  my ($t_fh, $t_nam) = &tempfile('motif_target_XXXXXXXXXX', DIR => $tmpdir, SUFFIX => '.meme', UNLINK => 1);
  print $t_fh &meme_nucleotide_motif($version, $bgline, $target_id, $target_pspm);
  
  # temporary file for output
  my ($img_fh, $img_nam) = &tempfile('eps_XXXXXXXXXX', DIR => $tmpdir, SUFFIX => '.eps', UNLINK => 1);

  # create arguments to ceqlogo
  my @ceqlogo_args = ('-Y', '-N'); # enable Y axis, number X axis
  push(@ceqlogo_args, '-E') if $error_bars;
  push(@ceqlogo_args, '-S') if $ssc;
  push(@ceqlogo_args, '-k', 'NA'); # nucleic acid alphabet
  push(@ceqlogo_args, '-w', $image_width) if $image_width;
  push(@ceqlogo_args, '-h', $image_height) if $image_height;
  push(@ceqlogo_args, '-d', $fine_text); 
  push(@ceqlogo_args, '-t', $target_id); # set title label
  push(@ceqlogo_args, '-x', $query_id); # set x axis label
  push(@ceqlogo_args, '-r') if $target_rc xor $flip; # rc target
  push(@ceqlogo_args, '-i', $t_nam); # target motif
  push(@ceqlogo_args, '-s', -$q_off) if ($q_off < 0); # shift target
  push(@ceqlogo_args, '-r') if $flip; # rc query
  push(@ceqlogo_args, '-i', $q_nam); # query motif
  push(@ceqlogo_args, '-s', $q_off) if ($q_off > 0); # shift query
  push(@ceqlogo_args, '-o', $img_nam);

  # run ceqlogo
  my ($status, $messages);
  $status = invoke(
    PROG => 'ceqlogo',
    BIN => $bin,
    ARGS => \@ceqlogo_args,
    ALL_VAR => \$messages,
    TMPDIR => $tmpdir);
  
  # remove temporary motif files
  &unlink1($q_fh, $q_nam);
  &unlink1($t_fh, $t_nam);

  # check status 
  return &program_error($q, 'ceqlogo', $status) if $status;

  if ($image_type eq 'png') {
    # create a temporary file as destination of ghostscript
    my ($png_fh, $png_nam) = &tempfile('png_XXXXXXXXXX', DIR => $tmpdir, SUFFIX => '.png', UNLINK => 1);

    $status = eps_to_png($img_nam, $png_nam, \$messages);

    # check status
    return &program_error($q, 'ghostscript/convert', $status) if $status;
    
    # remove eps temporary file
    &unlink1($img_fh, $img_nam);

    # update the image to be the png
    ($img_fh, $img_nam) = ($png_fh, $png_nam);
    $mime_type = "image/png";
  }

  # slurp the image
  seek($img_fh, 0, SEEK_SET);
  my $image = do { local( $/ ); <$img_fh>};

  # remove the image temporary file
  &unlink1($img_fh, $img_nam);

  # output the image
  my $outname = "logo_" . $query_id . "_" . $target_id . "." . $image_type;
  binmode STDOUT;
  print $q->header(-type=>$mime_type, -Content_Disposition=>"attachment; filename=$outname");
  print $image;
}

sub parse_background {
  my $bgtext = shift;
  die("parse_background: expected background probabilities string.\n") unless defined $bgtext;
  die("parse_background: expected 4 probabilities in a space delimited string.") unless ($bgtext =~ m/^0\.\d+\s+0\.\d+\s+0\.\d+\s+0\.\d+$/);
  my @bgprobs = split(/\s+/, $bgtext);
  die("parse_background: error expected the split to be of length 4 but it wasn't.") unless (@bgprobs == 4);
  my $probsum = sum(@bgprobs);
  die("parse_background: expected probabilities to sum to one (or close).") unless abs($probsum - 1) < 0.1;
  return "A $bgprobs[0] C $bgprobs[1] G $bgprobs[2] T $bgprobs[3]";
}

sub meme_nucleotide_motif {
  my ($version, $bgline, $id, $pspm) = @_;

  return "MEME version $version\n" .
         "ALPHABET= ACGT\n" .
         "strands: + -\n" .
         "Background letter frequencies (from an unknown source):\n" .
         $bgline . "\n" .
         "MOTIF $id\n" .
         $pspm;
}

sub date_time_string {
  my (undef,$min,$hour,$day,$month,$year,undef,undef,undef) = localtime(time);
  $year += 1900; # year contains years since 1900
  $month += 1; # month counted from zero.
  return sprintf("%d.%d.%d %02d:%02d", $day, $month, $year, $hour, $min);
}
