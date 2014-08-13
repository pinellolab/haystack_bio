package HtmlMonolithWr;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw();

use Fcntl qw(O_RDONLY O_WRONLY O_CREAT O_TRUNC);

use File::Spec::Functions qw(catfile);

use JsonWr;

sub new {
  my $classname = shift;
  my $self = {};
  bless($self, $classname);
  $self->_init(@_);
  return $self;
}

sub set_logger {
  my $self = shift;
  my ($logger) = @_;
  $self->{logger} = $logger;
}

sub output {
  my $self = shift;
  my $src = $self->{src};
  my $dest = $self->{dest};
  if ($self->{json_writer}) {
    $self->{json_writer}->done();
    $self->{json_writer} = undef;
    print $dest ";\n    </script>\n";
    print $dest $self->{following};
  }

  while (<$src>) {
    my $line = $_;
    my ($preceeding, $file, $following, $is_script, $replace, $before, $after);
    if ($line =~ m%^(.*)<script src="(.+)"></script>(.*)$%) {
      $preceeding = $1; $file = $2; $following = $3; $is_script = 1;
      $replace = $self->{scripts}->{$file};
      $before = "<script>\n"; $after = "</script>\n";
    } elsif ($line =~ m%^(.*)<link rel="stylesheet" type="text/css" href="(.+)">(.*)$%) {
      $preceeding = $1; $file = $2; $following = $3; $is_script = 0;
      $before = "<style>\n"; $after = "</style>\n";
    }
    if ($replace) {
      print $dest $preceeding;
      print $dest "<script>\n";
      print $dest '      // @JSON_VAR '. $replace . "\n";
      print $dest '      var ' . $replace . ' = ';
      $self->{json_writer} = new JsonWr($dest, 6, 2, 80);
      $self->{json_writer}->set_logger($self->{logger});
      $self->{following} = $following;
      return $self->{json_writer};
    } elsif ($file) {
      print $dest $preceeding;
      my $path = catfile($self->{search_dir}, $file);
      my $fh;
      die("Required file $path does not exist!\n") unless (-e $path);
      sysopen($fh, $path, O_RDONLY);
      print $dest $before;
      while(<$fh>) {
        print $dest $_;
      }
      close($fh);
      print $dest $after;
      print $dest $following;
    } else {
      print $dest $line;
    }
  }
  close($dest);
  close($src);
}

sub _init {
  my $self = shift;
  my ($search_dir, $primary_source, $dest_path, %scripts_to_replace) = @_;
  $self->{search_dir} = $search_dir;
  my $primary_path = catfile($search_dir, $primary_source);
  sysopen($self->{src}, $primary_path, O_RDONLY) or die("Failed to open file \"$primary_path\".");
  sysopen($self->{dest}, $dest_path, O_WRONLY | O_CREAT | O_TRUNC) or die("Failed to open file \"$dest_path\".");
  binmode($self->{dest}, ":utf8");
  $self->{scripts} = \%scripts_to_replace;
}

1;
