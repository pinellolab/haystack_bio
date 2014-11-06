#!/usr/bin/perl

use strict;
use warnings;

unshift(@INC, $ENV{'RSATWS'}) if ($ENV{'RSATWS'});

print "\n";
print "Checking Required Perl Modules:\n";
check_module("Cwd",
  imports => [qw(getcwd abs_path)]
); # general

check_module("Data::Dumper"
); # general

check_module("Exporter"
); # general

check_module("Fcntl", 
  imports => [qw(:DEFAULT O_RDONLY O_WRONLY O_CREAT O_TRUNC SEEK_CUR SEEK_SET)]
); # general

check_module("File::Basename", 
  imports => [qw(dirname fileparse)]
); # general

check_module("File::Copy", 
  imports => [qw(copy cp)]
); # general

check_module("File::Path", 
  imports => [qw(rmtree)], 
); # general

check_module("File::Spec::Functions", 
  imports => [qw(abs2rel catfile catdir splitdir tmpdir)]
); # general

check_module("File::Temp", 
  qw(tempfile tempdir)
); # general

check_module("Getopt::Long", 
); # general

check_module("HTML::PullParser"
); # meme2meme

check_module("HTML::Template"
); # job status

check_module("List::Util", 
  imports => [qw(max min sum)]
); # general

check_module("Pod::Usage"
); # stand-alone scripts

check_module("POSIX", 
  imports => [qw(strtod floor)]
); # general

check_module("Scalar::Util", 
  imports => [qw(looks_like_number)]
); # general

check_module("XML::Simple"
); # general

check_module("XML::Parser::Expat", 
); # MEME, DREME parsers and DiffXML for smoke test

check_module("Sys::Hostname"
); # website and webservice

check_module("Time::HiRes", 
  imports => [qw(gettimeofday tv_interval)]
); # website and webservice

print "\n";

print "Checking Optional Modules:\n";
check_module("Log::Log4perl", 
  desc   =>  "Used for logging and debugging by developers."
); #general

check_module("Math::CDF", 
  imports => [qw(:all)], 
  desc    => "Only required for fasta-enriched-center script (which is not called by the web scripts)."
); # fasta-enriched-center

print "\n";

#print "Listing Loaded Modules:\n\n";
#print join("\n", map { s|/|::|g; s|\.pm$||; $_ } keys %INC), "\n";


exit(0);

sub check_module {
  my ($mod_name, %options) = @_;
  my $mod_ver = $options{version};
  my @mod_imports = ();
  @mod_imports = @{$options{imports}} if $options{imports};
  my $mod_desc = $options{desc};
  $mod_desc = "" unless $mod_desc;

  eval("require $mod_name;");
  if ($@) {
    print "$mod_name missing.";
    print " $mod_desc" if $mod_desc;
    print "\n";
    return;
  }
  
  my $real_ver = eval('return $' . $mod_name . '::VERSION;');
  $real_ver = '<$VERSION not given in module>' unless defined $real_ver;
  if (defined $mod_ver) {
    eval("VERSION $mod_name $mod_ver;");
    if ($@) {
      print "$mod_name version is $real_ver but need $mod_ver.";
      print " $mod_desc" if $mod_desc;
      print "\n";
      return;
    }
  }

  # import the methods, this is needed for POSIX because it doen't initilize
  # the EXPORT and EXPORT_OK variables before import is called
  if (@mod_imports) {
    eval("import $mod_name ('" . join("', '", @mod_imports) . "');");
  } else {
    eval("import $mod_name;");
  }

  return unless @mod_imports;


  my %exported = ();
  # get exported methods
  my @methods = eval('return (@'. $mod_name . '::EXPORT_OK, '.
                             '@' . $mod_name . '::EXPORT);');
  unless ($@) {
    for (@methods) {$exported{$_} = 1;}
  } else {
    print "$mod_name $real_ver does not seem to use Exporter so can't check methods.\n";
    return;
  }
  # get exported tags
  my %tags = eval('return %'. $mod_name . '::EXPORT_TAGS');
  unless ($@) {
    for (keys %tags){
      $_ =~ s/^://; # remove leading :
      $exported{$_} = 1;
    }
  } 
  
  my @missing = ();
  foreach my $import (@mod_imports) {
    $import =~ s/^!//; # remove leading ! (means don't load this name or tag)
    next if ($import =~ m|^/.*/$|); # don't try matching regular expressions
    next if ($import eq ":DEFAULT"); # default always means everything in EXPORT
    $import =~ s/^://; # remove leading :
    unless (exists $exported{$import}) {
      push(@missing, $import);
    }
  }
  if (@missing) {
      print "$mod_name $real_ver does not export ". join(", ", @missing).".";
      print " $mod_desc" if $mod_desc;
      print "\nAvaliable ( ". join(" ", (keys %exported)) . " )\n";
  }

}
