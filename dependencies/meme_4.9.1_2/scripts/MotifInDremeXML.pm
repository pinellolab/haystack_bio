package MotifInDremeXML;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw();

use DremeSAX;

sub new {
  my $classname = shift;
  my $self = {};
  bless($self, $classname);
  $self->_init(@_);
  return $self;
}

sub parse_more {
  my $self = shift;
  my ($buffer) = @_;
  $self->{parser}->parse_more($buffer);
}

sub parse_done {
  my $self = shift;
  $self->{parser}->parse_done();
}

sub has_errors {
  my $self = shift;
  return $self->{parser}->has_errors();
}

sub get_errors {
  my $self = shift;
  return $self->{parser}->get_errors();
}

sub get_motifs {
  my $self = shift;
  my @motifs = @{$self->{queue}};
  $self->{queue} = [];
  return @motifs;
}

sub has_motif {
  my $self = shift;
  return scalar(@{$self->{queue}}) > 0;
}

sub next_motif {
  my $self = shift;
  return pop(@{$self->{queue}});
}

sub _init {
  my $self = shift;

  my $sax = new DremeSAX($self, 
    'handle_background' => \&_handle_background,
    'start_motif' => \&_start_motif,
    'end_motif' => \&_end_motif,
    'handle_pos' => \&_handle_pos
  );
  $self->{parser} = $sax;
  $self->{queue} = [];
  $self->{motif} = undef;
  $self->{bg} = {};

}

sub _handle_background {
  my $self = shift;
  my ($type, $A, $C, $G, $T, $from, $file, $last_mod) = @_;
  my $bg = $self->{bg};
  $bg->{dna} = 1;#TRUE
  $bg->{source} = ($from eq 'file' ? $file : $from);
  $bg->{A} = $A;
  $bg->{C} = $C;
  $bg->{G} = $G;
  $bg->{T} = $T;
}

sub _start_motif {
  my $self = shift;
  my ($id, $seq, $length, $nsites, $p, $n, $pvalue, $evalue, $unerased_evalue) = @_;
  
  my %pspm = ();
  my @syms = qw(A C G T);
  foreach my $sym (@syms) {
    $pspm{$sym} = [];
  }

  my $motif = {
    bg => $self->{bg},
    strands => 2,
    id => $seq, 
    width => $length, 
    sites => $nsites,
    pseudo => 0,
    evalue => $evalue,
    pspm => \%pspm,
  };

  $self->{motif} = $motif;
}

sub _end_motif {
  my $self = shift;
  push(@{$self->{queue}}, $self->{motif});
  $self->{motif} = undef;
}

sub _handle_pos {
  my $self = shift;
  my ($pos, $A, $C, $G, $T) = @_;
  my $i = $pos - 1;
  my $motif = $self->{motif};
  $motif->{pspm}->{A}->[$i] = $A;
  $motif->{pspm}->{C}->[$i] = $C;
  $motif->{pspm}->{G}->[$i] = $G;
  $motif->{pspm}->{T}->[$i] = $T;
}

1;
