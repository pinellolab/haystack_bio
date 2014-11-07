package MotifInMemeXML;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw();

use MemeSAX;

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

  my $sax = new MemeSAX($self, 
    start_alphabet => \&_alphabet,
    handle_alphabet_letter => \&_alph_letter,
    handle_ambigs_letter => \&_alph_letter,
    handle_sequence => \&_sequence,
    handle_strands => \&_strands,
    start_background_frequencies => \&_bg_source,
    handle_bf_aa_value => \&_bg_value,
    start_motif => \&_start_motif,
    end_motif => \&_end_motif,
    start_scores => \&_start_matrix,
    handle_sc_am_aa_value => \&_score_value,
    end_sc_am_alphabet_array => \&_end_row,
    start_probabilities => \&_start_matrix,
    handle_pr_am_aa_value => \&_prob_value,
    end_pr_am_alphabet_array => \&_end_row,
    start_contributing_site => \&_contr_site,
    handle_left_flank => \&_contr_lflank,
    handle_right_flank => \&_contr_rflank,
    handle_letter_ref => \&_contr_letter
  );
  $self->{parser} = $sax;
  $self->{queue} = [];
  $self->{strands} = undef;
  $self->{motif} = undef;
  $self->{sym_map} = {}; # maps letter_id to symbol
  $self->{seq_map} = {}; # maps sequence ids to sequences
  $self->{bg} = {};
  $self->{counter} = undef;
}

sub _alphabet {
  my $self = shift;
  my ($is_dna, $length) = @_;
  $self->{bg}->{dna} = $is_dna;
}

sub _alph_letter {
  my $self = shift;
  my ($letter_id, $symbol) = @_;
  $self->{sym_map}->{$letter_id} = $symbol;
}

sub _sequence {
  my $self = shift;
  my ($sequence_id, $name, $length, $weight) = @_;
  die("No seq id") unless $sequence_id;
  $self->{seq_map}->{$sequence_id} = $name;
}

sub _strands {
  my $self = shift;
  my ($strands) = @_;
  $self->{strands} = ($strands eq 'both' ? 2 : ($strands eq 'forward' ? 1 : 0));
}

sub _bg_source {
  my $self = shift;
  my ($source) = @_;
  $self->{bg}->{source} = $source;
}

sub _bg_value {
  my $self = shift;
  my ($letter_id, $frequency) = @_;
  my $sym = $self->{sym_map}->{$letter_id};
  $self->{bg}->{$sym} = $frequency;
}

sub _start_motif {
  my $self = shift;
  my ($id, $name, $width, $sites, $llr, $ic, $re, 
    $bayes_threshold, $e_value, $elapsed_time, $url) = @_;

  my %pspm = ();
  my %pssm = ();
  my @syms = ($self->{bg}->{dna} ? qw(A C G T) : qw(A C D E F G H I K L M N P Q R S T V W Y));
  foreach my $sym (@syms) {
    $pspm{$sym} = [];
    $pssm{$sym} = [];
  }

  my $motif = {
    bg => $self->{bg},
    strands => $self->{strands},
    id => $name, 
    url => $url, 
    width => $width, 
    sites => $sites,
    pseudo => 0,
    evalue => $e_value,
    pspm => \%pspm,
    pssm => \%pssm,
    contributing_sites => []
  };

  $self->{motif} = $motif;
}

sub _end_motif {
  my $self = shift;
  push(@{$self->{queue}}, $self->{motif});
  $self->{motif} = undef;
}

sub _start_matrix {
  my $self = shift;
  $self->{counter} = 0;
}

sub _end_row {
  my $self = shift;
  $self->{counter} += 1;
}

sub _score_value {
  my $self = shift;
  my ($letter_id, $score) = @_;
  my $sym = $self->{sym_map}->{$letter_id};
  my $pos = $self->{counter};
  $self->{motif}->{pssm}->{$sym}->[$pos] = $score;
}

sub _prob_value {
  my $self = shift;
  my ($letter_id, $prob) = @_;
  my $sym = $self->{sym_map}->{$letter_id};
  my $pos = $self->{counter};
  $self->{motif}->{pspm}->{$sym}->[$pos] = $prob;
}

sub _contr_site {
  my $self = shift;
  my ($seqid, $pos, $strand, $pvalue) = @_;
  my %site = ();
  $site{sequence} = $self->{seq_map}->{$seqid};
  $site{pos} = $pos;
  $site{strand} = ($strand eq 'plus' ? 1 : ($strand eq 'minus' ? -1 : 0));
  $site{pvalue} = $pvalue;
  $site{lflank} = '';
  $site{rflank} = '';
  $site{site} = '';

  push(@{$self->{motif}->{contributing_sites}}, \%site); 
}

sub _contr_lflank {
  my $self = shift;
  my ($left_flank) = @_;
  $self->{motif}->{contributing_sites}->[-1]->{lflank} = $left_flank;
}

sub _contr_rflank {
  my $self = shift;
  my ($right_flank) = @_;
  $self->{motif}->{contributing_sites}->[-1]->{rflank} = $right_flank;
}

sub _contr_letter {
  my $self = shift;
  my ($letter_id) = @_;
  my $sym = $self->{sym_map}->{$letter_id};
  $self->{motif}->{contributing_sites}->[-1]->{site} .= $sym;
}

1;
