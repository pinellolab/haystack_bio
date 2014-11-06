#!@WHICHPERL@

use strict;
use lib qw(@PERLLIBDIR@);
use CGI qw(:standard);
use CGI::Carp qw(fatalsToBrowser);
use Data::Dumper;
use Fcntl qw(SEEK_SET);
use File::Spec::Functions qw(catdir catfile tmpdir);
use File::Temp qw(tempfile);
use HTTP::Request::Common qw(POST);
use LWP::UserAgent;

use ExecUtils qw(invoke);
use MemeWebUtils qw(eps_to_png);
use MotifUtils qw(intern_to_meme parse_double);

# Setup logging
my $logger = undef;
eval {
  require Log::Log4perl;
  Log::Log4perl->import();
};
unless ($@) {
  Log::Log4perl::init('@APPCONFIGDIR@/logging.conf');
  $logger = Log::Log4perl->get_logger('meme.cgi.meme_request');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting meme_request CGI") if $logger;

# globals
$CGI::POST_MAX = 1024 * 1024 * 5; # Limit post to 5MB
$CGI::DISABLE_UPLOADS = 1; # Disallow file uploads
my $tmpdir = '@TMP_DIR@';
$tmpdir = &tmpdir() if ($tmpdir eq '' || $tmpdir =~ m/^\@TMP[_]DIR\@$/);

&process_request();
exit(0);

sub process_request {
  my $blocks_url = '@BLOCKS_URL@';
  my $fimo_url = '@SITE_URL@/cgi-bin/fimo.cgi';
  my $gomo_url = '@SITE_URL@/cgi-bin/gomo.cgi';
  my $mast_url = '@SITE_URL@/cgi-bin/mast.cgi';
  my $tomtom_url = '@SITE_URL@/cgi-bin/tomtom.cgi';
  my $spamo_url = '@SITE_URL@/cgi-bin/spamo.cgi';


  my $q = CGI->new();

  # discover what the user wants to do
  my ($program, $motif);

  # optionally get the program and motif from the parameters with those names
  $program = $q->param('program');
  $motif = $q->param('motif');

  # otherwise look for the name of the submit button used (for MEME form)
  foreach my $param_name ($q->param()) {
    if ($param_name =~ m/^do_(MAST|FIMO|GOMO|BLOCKS|TOMTOM|LOGO)_(all|\d+)$/) {
      $program = $1;
      $motif = $2;
      last;
    }
  }

  # generate content
  my ($content, $content_len);
  if (&nmotifs($q) > 0) {
    if ($motif && $program) {
      if ($program eq "MAST") {
        $content = search($q, $mast_url, $motif, 0);
      } elsif ($program eq "FIMO") {
        $content = search($q, $fimo_url, $motif, 0);
      } elsif ($program eq "GOMO") {
        $content = search($q, $gomo_url, $motif, 0);
      } elsif ($program eq "TOMTOM") {
        $content = search($q, $tomtom_url, $motif, 0);
      } elsif ($program eq "SPAMO") {
        $content = search($q, $spamo_url, $motif, 0);
      } elsif ($program eq "BLOCKS") {
        $content = submit_block($q, $blocks_url, $motif);
      } elsif ($program eq "LOGO") {
        # this produces an image which requires different headers
        # it does not return unless it need to display an error page
        $content = generate_logo($q, $motif);
      } elsif ($program) {
        $content = error_page($q, "Unknown program", "The program \"$program\" can't be handled by this script.");
      }
    } else {
      $content = error_page($q, "No action?", "The script couldn't find a parameter of the form \"do_[Program]_[Motif#|all]\" and so can't perform an action."); 
    }
  } else {
    $content = error_page($q, "No motifs?", "The number of motifs parameter resolved to a non-positive number. Maybe it wasn't specified?");
  }
  $content_len = length($content);

  # output content
  print "Content-type: text/html", "\n";
  print "Content-length: $content_len", "\n\n";
  print $content;

}

#
# get the number of motifs
#
sub nmotifs {
  my ($q) = @_;
  return int($q->param('nmotifs'));
}

#
# Parse a string containing the background
#
sub parse_bg {
  $logger->trace("call parse_bg") if $logger;
  my ($alphabet, $bgsrc, $bgfreq) = @_;
  
  my %bg = ();

  $bg{dna} = $alphabet eq "ACGT";
  
  $bg{source} = (defined $bgsrc ? $bgsrc : "unknown source");

  my @bglines = split(/\n/, $bgfreq);
  my @bgleft = ();
  my $totalbg = 0;
  foreach my $bgline (@bglines) {
    next if $bgline =~ m/^\s*$/;
    $bgline =~ s/^\s+//;
    $bgline =~ s/\s+$//;
    my @bgparts = split(/\s+/, $bgline);
    push(@bgleft, @bgparts);
    while (scalar(@bgleft) >= 2) {
      my $a = shift(@bgleft);
      my $f = 0 + shift(@bgleft);
      $bg{$a} = $f;
      $totalbg += $f;
    }
  }
  $logger->debug("parse_bg returns " . Dumper(\%bg)) if $logger;
  return \%bg;
}

#
# Parse a string containing the number of strands
#
sub parse_strands {
  my ($strands_txt, $dna) = @_;
  my $strands = 0;
  if ($dna) {
    if ($strands_txt =~ m/\+\s*-/ or $strands_txt =~ m/-\s*\+/) {
      $strands = 2;
    } else {
      $strands = 1;
    }
  }  
  return $strands;
}

#
# Set the pspm component of a motif datastructure
# by parsing a minimal meme format pssm.
#
sub parse_pspm {
  $logger->trace("call parse_pspm") if $logger;
  my ($motif, $alphabet, $pspm) = @_;
  my @letters = split(//, $alphabet);
  my %pspm_matrix = ();
  foreach my $letter (@letters) {
    $pspm_matrix{$letter} = [];
  }
  $motif->{pspm} = \%pspm_matrix;
  
  my @pspm_lines = split(/\n/, $pspm);

  for (my $linei = 0; $linei < scalar(@pspm_lines); $linei++) {
    my $line = $pspm_lines[$linei];
    if ($line =~ m/^letter-probability matrix:\s+(.*)$/) {
      $line = $1;
      $line =~ s/\s+$//; #trim right
      my %motif_params = split(/\s+/, $line);
      #check that we have the parameters we require
      die("Pspm is missing required parameter(s).\n") unless (
        defined($motif_params{'alength='}) && defined($motif_params{'w='}) && 
        defined($motif_params{'nsites='}) &&  defined($motif_params{'E='})
      );
      $motif->{width} = $motif_params{'w='}; 
      $motif->{sites} = $motif_params{'nsites='};
      $motif->{evalue} = $motif_params{'E='};
      if (scalar(@pspm_lines) < ($linei + $motif->{width})) {
        die("Pspm is missing required rows\n");
      }
      for (my $row = 0; $row < $motif->{width}; $row++) {
        $line = $pspm_lines[$linei + $row + 1];
        $line =~ s/^\s+//; # trim left
        $line =~ s/\s+$//; # trim right
        my @probs = split(/\s+/, $line);
        die("Pspm has incorrect row $row.\n") unless (
          scalar(@probs) == scalar(@letters)
        );
        for (my $i = 0; $i < scalar(@probs); $i++) {
          $motif->{pspm}->{$letters[$i]}->[$row] = parse_double($probs[$i]);
        }
      }
      last;
    }
  }
}

#
# Set the pssm component of a motif datastructure
# by parsing a minimal meme format pssm.
#
sub parse_pssm {
  $logger->trace("call parse_pssm") if $logger;
  my ($motif, $alphabet, $pssm) = @_;
  my @letters = split(//, $alphabet);
  return unless defined $pssm;
  my %pssm_matrix = ();
  foreach my $letter (@letters) {
    $pssm_matrix{$letter} = [];
  }
  $motif->{pssm} = \%pssm_matrix;

  my @pssm_lines = split(/\n/, $pssm);

  for (my $linei = 0; $linei < scalar(@pssm_lines); $linei++) {
    my $line = $pssm_lines[$linei];
    if ($line =~ m/log-odds matrix:/) {
      for (my $row = 0; $row < $motif->{width}; $row++) {
        $line = $pssm_lines[$linei + $row + 1];
        $line =~ s/^\s+//; # trim left
        $line =~ s/\s+$//; # trim right
        my @scores = split(/\s+/, $line);
        die("Pssm has incorrect row $row.\n") unless (
          scalar(@scores) == scalar(@letters)
        );
        for (my $i = 0; $i < scalar(@scores); $i++) {
          $motif->{pssm}->{$letters[$i]}->[$row] = parse_double($scores[$i]);
        }
      }
    }
  }
}

#
# get a motif name for the passed ordinal
#
sub motif_name {
  my ($q, $i) = @_;
  # first see if the motif name is avaliable (it won't be for MEME)
  my $name = $q->param('motifname'.$i);
  unless (defined $name) {
    # try to extract the motif name from the block parameter
    my $block = $q->param('motifblock'.$i);
    if (defined $block) {
      if ($block =~ m/\bMOTIF\s+([^\s]+)\b/) {
        $name = $1;
      }
    }
  }
  # if all else fails use a number for the motif name
  $name = "m" . $i unless defined $name;
  return $name;
}

#
# combine the selected motifs into minimal meme format
#
sub motifs {
  $logger->trace("call motifs") if $logger;
  my ($q, @nums) = @_;

  my $alphabet = $q->param('alphabet');
  my $bg = parse_bg($alphabet, scalar $q->param('bgsrc'), $q->param('bgfreq'));
  my $strands = parse_strands(scalar $q->param('strands'), $bg->{dna});
  my $output = "";
  my $is_first = 1;
  foreach my $i (@nums) { 
    my $motif = {
      bg => $bg, 
      strands => $strands, 
      pseudo => 0, 
      id => &motif_name($q, $i)
    };
    parse_pspm($motif, $alphabet, $q->param('pspm'.$i));
    # note pssm is optional, this line will do nothing if it is missing
    parse_pssm($motif, $alphabet, $q->param('pssm'.$i));
  
    $logger->trace("motif $i: " . Dumper($motif)) if $logger;

    $output .= intern_to_meme($motif, 1, 1, $is_first);
    $is_first = 0;
  }
  return $output;
}

#
# Test if all parameters required for a search are avaliable
#
sub can_search {
  $logger->trace("call can_search") if $logger;
  my ($q, @nums) = @_;
  return 0 unless $q->param('version') && $q->param('alphabet') 
      && $q->param('bgfreq') && $q->param('name');
  foreach my $i (@nums) {
    return 0 unless $q->param('pspm'.$i);
  }
  return 1;
}

#
# Submit a search q
#
sub search {
  $logger->trace("call search") if $logger;
  my($q, $url, $selected, $all) = @_;
  my(%params, $content, $i);

  my @nums = ($selected);
  @nums = (1 .. &nmotifs($q)) if ($selected eq 'all' || $all);


  if (can_search($q, @nums)) {
    # set up the parameters to pass
    $params{'version'} = $q->param('version');
    $params{'alphabet'} = $q->param('alphabet');
    $params{'bgfreq'} = $q->param('bgfreq');
    $params{'inline_name'} = $q->param('name');
    $params{'inline_motifs'} = &motifs($q, @nums);
    $params{'inline_selected'} = $selected if ($selected ne 'all' && $all);
    
    # post the query
    my $ua  = LWP::UserAgent->new();
    my $req  = POST $url, [%params];
    my $request  = $ua->request($req);
    $content = $request->content;
  } else {
    $content = error_page($q, "Missing required variable","");
  }
  return $content;
} # search 

sub error_page {
  $logger->trace("call error_page") if $logger;
  my($q, $title, $message, $warnings) = @_;
  my $content = 
    "<html>\n".
    "\t<head>\n".
    "\t\t<title>$title</title>\n".
    "\t</head>\n".
    "\t<body>\n".
    "\t\t<h1>$title</h1>\n".
    "\t\t<p>$message</p>\n";
  if (defined($warnings)) {
    $content .=
      "\t\t<h2>Program Errors</h2>\n".
      "\t\t<textarea readonly='readonly'>$warnings</textarea>\n";
  }
  $content .=
    "\t\t<table border=\"1\">\n".
    "\t\t\t<tr>\n".
    "\t\t\t\t<th>Parameter Name</th>\n".
    "\t\t\t\t<th>Parameter Value</th>\n".
    "\t\t\t</tr>\n";
  my ($param_name, $param_value);
  foreach $param_name ($q->param()) {
    $param_value = $q->param($param_name);
    $content .=
        "\t\t\t<tr>\n".
        "\t\t\t\t<td>$param_name</td>\n".
        "\t\t\t\t<td>$param_value</td>\n".
        "\t\t\t</tr>\n";
  }
  $content .= 
      "\t\t</table>\n".
      "\t</body>\n".
      "</html>";
  return $content;
}

sub program_error {
  my ($q, $program, $status, $warnings) = @_;
  # check status
  my $message;
  if ($status == -1) {
    $message = "$program failed to run.";
  } elsif ($status & 127) {
    my $sig = $status & 127;
    my $with = $status & 128 ? 'with' : 'without';
    $message = "$program died with signal $sig $with coredump";
  } else {
    $message = "$program exited with value ".($? >> 8).".";
  }
  return error_page($q, "$program error", $message, $warnings);
}

#
# Submit a block to the blocks processor
#
sub submit_block {
  $logger->trace("call submit_block") if $logger;
  my($q, $blocks_url, $motif) = @_;
  my($content, $blocks, $i);

  # get the BLOCK(S)
  if ($motif eq 'all') {
    $blocks = "";
    my $end = &nmotifs($q);
    for ($i = 1; $i <= $end; $i++) {
      $blocks .= $q->param('BLOCKS'.$i);
    }
  } else {
    $blocks = $q->param('BLOCKS'.$motif);
  }

  my $ua = LWP::UserAgent->new();
  my $req = POST $blocks_url, [ sequences => $blocks ];
  my $request = $ua->request($req);
  my $response = $request->content;
  # put in the absolute url's : this is FRAGILE!
  $response =~ s#HREF=\"/#HREF=\"http://blocks.fhcrc.org/#g;
  $response =~ s#ACTION=\"/blocks-bin#ACTION=\"http://blocks.fhcrc.org/blocks-bin#g;
  $content = 
      "<html>\n".
      "\t<head>\n".
      "\t\t<title>MEME Suite - Submit block</title>\n".
      "\t</head>\n".
      "\t<body style=\"background-image:url('../images/bkg.jpg');\">\n".
      "$response\n".
      "\t</body>\n".
      "</html>";

  $content;
} # submit_block

sub get_param_with_default {
  $logger->trace("call get_param_with_default") if $logger;
  my ($q, $name, $num, $default) = @_;
  my $param;
  $param = $q->param($name .'_' . $num) if defined($num);
  $param = $q->param($name) unless defined $param;
  $param = $default unless defined $param;
  return $param;
}

sub get_pos_num {
  my ($txt) = @_;

  if ($txt =~ m/^\s*(\d+(.\d+)?)\s*$/) {
    return $1;
  } else {
    return undef;
  }
}

sub logo_name {
  my ($rc, $ssc, $png, $number) = @_;
  # generate the name that should be specified for downloading
  my $name = "logo";
  if ($rc) {
    $name .= "_rc";
  }
  if ($ssc) {
    $name .= "_ssc";
  }
  $name .= $number;
  $name .= ".";
  if ($png) {
    $name .= "png";
  } else {
    $name .= "eps";
  }
  return $name;
}

sub logo_desc {
  my ($program, $ssc) = @_;
  my $fineprint = $program . ' ';
  if ($ssc) {
    $fineprint = $fineprint.'(with SSC)';
  } else {
    $fineprint = $fineprint.'(no SSC)';
  }
  my (undef,$min,$hour,$day,$month,$year,undef,undef,undef) = localtime(time);
  $year += 1900; # year contains years since 1900
  $month += 1; # month counted from zero.
  $fineprint .= sprintf("%d.%d.%d %02d:%02d", $day, $month, $year, $hour, $min);
  return $fineprint;
}

sub unlink1 {
  my ($fh, $filename) = @_;
  close($fh);
  unlink($filename);
}

sub generate_logo {
  $logger->trace("call generate_logo") if $logger;
  my ($q, $number) = @_;
  my $bin = catdir('@MEME_DIR@', 'bin');

  # mime type
  my $mime_type = "application/postscript";

  # get logo settings
  my ($program, $dna, $ssc, $png, $rc, $width, $height);
  $program = $q->param('program');
  $program = 'MEME' unless defined $program;
  $dna = (length(get_param_with_default($q, 'alphabet', undef, '')) < 20);
  $ssc = (get_param_with_default($q, 'logossc', $number, 'false') eq 'true');
  $png = (get_param_with_default($q, 'logoformat', $number, 'png') eq 'png');
  $rc = (get_param_with_default($q, 'logorc', $number, 'false') eq 'true');
  $width = get_pos_num(get_param_with_default($q, 'logowidth', $number, ''));
  $height = get_pos_num(get_param_with_default($q, 'logoheight', $number, ''));

  # write the motif to a temporary file
  my ($motif_fh, $motif_nam) = &tempfile('motif_XXXXXXXXXX', DIR => $tmpdir, SUFFIX => '.meme', UNLINK => 1);
  print($motif_fh &motifs($q, $number));

  # create a temporary file as destination of ceqlogo
  my ($img_fh, $img_nam) = &tempfile('logo_XXXXXXXXXX', DIR => $tmpdir, SUFFIX => '.eps', UNLINK => 1);

  # create arguments to ceqlogo
  my @ceqlogo_args = ('-Y', '-N', '-t', '', '-x', ''); # enable Y axis, number X axis remove title, remove X axis label
  push(@ceqlogo_args, '-k', ($dna ? 'NA' : 'AA')); # set alphabet type
  push(@ceqlogo_args, '-d', &logo_desc($program, $ssc)); # set descriptive fine print
  push(@ceqlogo_args, '-E', '-S') if $ssc; # if ssc: enable error bars, enable SSC
  push(@ceqlogo_args, '-r') if ($rc && $dna); # if rc: enable reverse complement mode
  push(@ceqlogo_args, '-w', $width) if $width; # set the width
  push(@ceqlogo_args, '-h', $height) if $height; # set the height
  push(@ceqlogo_args, '-i', $motif_nam); # set the input file
  push(@ceqlogo_args, '-o', $img_nam);

  # run ceqlogo
  my ($status, $messages);
  $status = invoke(
    PROG => 'ceqlogo',
    BIN => $bin,
    ARGS => \@ceqlogo_args,
    ALL_VAR => \$messages,
    TMPDIR => $tmpdir);
  
  # remove motif temporary file
  &unlink1($motif_fh, $motif_nam);

  # check status
  return &program_error($q, 'ceqlogo', $status, $messages) if $status;

  if ($png) {
    # create a temporary file as destination of ghostscript
    my ($png_fh, $png_nam) = &tempfile('logo_XXXXXXXXXX', DIR => $tmpdir, SUFFIX => '.png', UNLINK => 1);

    $status = eps_to_png($img_nam, $png_nam, \$messages);

    # check status
    return &program_error($q, 'ghostscript/convert', $status, $messages) if $status;
    
    # remove eps temporary file
    &unlink1($img_fh, $img_nam);

    # update the image to be the png
    ($img_fh, $img_nam) = ($png_fh, $png_nam);
    $mime_type = "image/png";
  }

  # slurp in content
  seek($img_fh, 0, SEEK_SET);
  my $content = do { local( $/ ); <$img_fh>};

  # remove image file
  &unlink1($img_fh, $img_nam);

  # output the image
  my $outname = &logo_name($rc, $ssc, $png, $number);
  binmode STDOUT;
  print $q->header(-type=>$mime_type, -Content_Disposition=>"attachment; filename=$outname");
  print $content;
  exit(0);
}
