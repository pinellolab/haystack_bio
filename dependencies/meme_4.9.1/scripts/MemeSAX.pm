package MemeSAX;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw();

use XML::Parser::Expat;

my $RPT_ONE = 0; # occur once only (default)
my $RPT_OPT = 1; # optionaly occur once
my $RPT_ALO = 2; # repeat one or more times
my $RPT_ANY = 3; # repeat any number of times

my $ST_IN_SCORES_AM_AA = [{ELE => 'value', RPT => $RPT_ALO, CHARS => 1,
    START => \&_start_ele_sc_am_aa_value, END => \&_end_ele_sc_am_aa_value}];
my $ST_IN_PROBS_AM_AA = [{ELE => 'value', RPT => $RPT_ALO, CHARS => 1, 
    START => \&_start_ele_pr_am_aa_value, END => \&_end_ele_pr_am_aa_value}];
my $ST_IN_SITE = [{ELE => 'letter_ref', RPT => $RPT_ALO, START => \&_start_ele_letter_ref}];
my $ST_IN_SCORES_AM = [{ELE => 'alphabet_array', RPT => $RPT_ALO, TO => $ST_IN_SCORES_AM_AA, 
    START => \&_start_ele_sc_am_alphabet_array, END => \&_end_ele_sc_am_alphabet_array}];
my $ST_IN_PROBS_AM = [{ELE => 'alphabet_array', RPT => $RPT_ALO, TO => $ST_IN_PROBS_AM_AA, 
    START => \&_start_ele_pr_am_alphabet_array, END => \&_end_ele_pr_am_alphabet_array}];
my $ST_IN_CSITE = [
  {ELE => 'left_flank', CHARS => 1, END => \&_end_ele_left_flank},
  {ELE => 'site', TO => $ST_IN_SITE, START => \&_start_ele_site, END => \&_end_ele_site},
  {ELE => 'right_flank', CHARS => 1, END => \&_end_ele_right_flank}
];
my $ST_IN_SCORES = [{ELE => 'alphabet_matrix', TO => $ST_IN_SCORES_AM, 
    START => \&_start_ele_sc_alphabet_matrix, END => \&_end_ele_sc_alphabet_matrix}];
my $ST_IN_PROBS = [{ELE => 'alphabet_matrix', TO => $ST_IN_PROBS_AM, 
    START => \&_start_ele_pr_alphabet_matrix, END => \&_end_ele_pr_alphabet_matrix}];
my $ST_IN_CSITES = [{ELE => 'contributing_site', RPT => $RPT_ALO, TO => $ST_IN_CSITE, 
    START => \&_start_ele_contributing_site, END => \&_end_ele_contributing_site}];
my $ST_IN_BGFREQS_AA = [{ELE => 'value', RPT => $RPT_ALO, CHARS => 1, 
    START => \&_start_ele_bf_aa_value, END => \&_end_ele_bf_aa_value}];
my $ST_IN_LFREQS_AA = [{ELE => 'value', RPT => $RPT_ALO, CHARS => 1, 
    START => \&_start_ele_lf_aa_value, END => \&_end_ele_lf_aa_value}];
my $ST_IN_SSITES = [{ELE => 'scanned_site', RPT => $RPT_ANY, 
    START => \&_start_ele_scanned_site}];
my $ST_IN_MOTIF = [
  {ELE => 'scores', TO => $ST_IN_SCORES, START => \&_start_ele_scores, END => \&_end_ele_scores},
  {ELE => 'probabilities', TO => $ST_IN_PROBS, 
    START => \&_start_ele_probabilities, END => \&_end_ele_probabilities},
  {ELE => 'regular_expression', RPT => $RPT_OPT, CHARS => 1, END => \&_end_ele_regular_expression},
  {ELE => 'contributing_sites', TO => $ST_IN_CSITES, 
    START => \&_start_ele_contributing_sites, END => \&_end_ele_contributing_sites}
];
my $ST_BGFREQS = [{ELE => 'alphabet_array', TO => $ST_IN_BGFREQS_AA, 
    START => \&_start_ele_bf_alphabet_array, END => \&_end_ele_bf_alphabet_array}];
my $ST_IN_LFREQS = [{ELE => 'alphabet_array', TO => $ST_IN_LFREQS_AA, 
    START => \&_start_ele_lf_alphabet_array, END => \&_end_ele_lf_alphabet_array}];# letter frequencies
my $ST_IN_AMBIGS = [{ELE => 'letter', RPT => $RPT_ALO, START => \&_start_ele_ambigs_letter}];
my $ST_IN_ALPHABET = [{ELE => 'letter', RPT => $RPT_ALO, START => \&_start_ele_alphabet_letter}];
my $ST_IN_SSS = [{ELE => 'scanned_sites', RPT => $RPT_ALO, TO => $ST_IN_SSITES, 
    START => \&_start_ele_scanned_sites, END => \&_end_ele_scanned_sites}];
my $ST_IN_MOTIFS = [{ELE => 'motif', RPT => $RPT_ALO, TO => $ST_IN_MOTIF, 
    START => \&_start_ele_motif, END => \&_end_ele_motif}];
my $ST_IN_MODEL = [
  {ELE => 'command_line', CHARS => 1, END => \&_end_ele_command_line},
  {ELE => 'host', CHARS => 1, END => \&_end_ele_host},
  {ELE => 'type', CHARS => 1, END => \&_end_ele_type},
  {ELE => 'nmotifs', CHARS => 1, END => \&_end_ele_nmotifs},
  {ELE => 'evalue_threshold', CHARS => 1, END => \&_end_ele_evalue_threshold},
  {ELE => 'object_function', CHARS => 1, END => \&_end_ele_object_function},
  {ELE => 'min_width', CHARS => 1, END => \&_end_ele_min_width},
  {ELE => 'max_width', CHARS => 1, END => \&_end_ele_max_width},
  {ELE => 'minic', CHARS => 1, END => \&_end_ele_minic},
  {ELE => 'wg', CHARS => 1, END => \&_end_ele_wg},
  {ELE => 'ws', CHARS => 1, END => \&_end_ele_ws},
  {ELE => 'endgaps', CHARS => 1, END => \&_end_ele_endgaps},
  {ELE => 'minsites', CHARS => 1, END => \&_end_ele_minsites},
  {ELE => 'maxsites', CHARS => 1, END => \&_end_ele_maxsites},
  {ELE => 'wnsites', CHARS => 1, END => \&_end_ele_wnsites},
  {ELE => 'prob', CHARS => 1, END => \&_end_ele_prob},
  {ELE => 'spmap', CHARS => 1, END => \&_end_ele_spmap},
  {ELE => 'spfuzz', CHARS => 1, END => \&_end_ele_spfuzz},
  {ELE => 'prior', CHARS => 1, END => \&_end_ele_prior},
  {ELE => 'beta', CHARS => 1, END => \&_end_ele_beta},
  {ELE => 'maxiter', CHARS => 1, END => \&_end_ele_maxiter},
  {ELE => 'distance', CHARS => 1, END => \&_end_ele_distance},
  {ELE => 'num_sequences', CHARS => 1, END => \&_end_ele_num_sequences},
  {ELE => 'num_positions', CHARS => 1, END => \&_end_ele_num_positions},
  {ELE => 'seed', CHARS => 1, END => \&_end_ele_seed},
  {ELE => 'seqfrac', CHARS => 1, END => \&_end_ele_seqfrac},
  {ELE => 'strands', CHARS => 1, END => \&_end_ele_strands},
  {ELE => 'priors_file', CHARS => 1, END => \&_end_ele_priors_file},
  {ELE => 'reason_for_stopping', CHARS => 1, END => \&_end_ele_reason_for_stopping},
  {ELE => 'background_frequencies', TO => $ST_BGFREQS, 
    START => \&_start_ele_background_frequencies, END => \&_end_ele_background_frequencies}
];
my $ST_IN_TRSET = [ # training set
  {ELE => 'alphabet', TO => $ST_IN_ALPHABET, START => \&_start_ele_alphabet, END => \&_end_ele_alphabet},
  {ELE => 'ambigs', TO => $ST_IN_AMBIGS, START => \&_start_ele_ambigs, END => \&_end_ele_ambigs},
  {ELE => 'sequence', RPT => $RPT_ALO, START => \&_start_ele_sequence},
  {ELE => 'letter_frequencies', TO => $ST_IN_LFREQS, 
    START => \&_start_ele_letter_frequencies, STOP => \&_end_ele_letter_frequencies}
];
my $ST_IN_MEME = [
  {ELE => 'training_set', TO => $ST_IN_TRSET, START => \&_start_ele_training_set, END => \&_end_ele_training_set},
  {ELE => 'model', TO => $ST_IN_MODEL, START => \&_start_ele_model, END => \&_end_ele_model},
  {ELE => 'motifs', TO => $ST_IN_MOTIFS, START => \&_start_ele_motifs, END => \&_end_ele_motifs},
  {ELE => 'scanned_sites_summary', RPT => $RPT_OPT, TO => $ST_IN_SSS, 
    START => \&_start_ele_scanned_sites_summary, END => \&_end_ele_scanned_sites_summary}
];
my $ST_START = [{ELE => 'MEME', RPT => $RPT_ONE, TO => $ST_IN_MEME, START => \&_start_ele_meme, END => \&_end_ele_meme}];

my $num_re = qr/^\s*((?:[-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)|inf)\s*$/;

sub new {
  my $classname = shift;
  my $self = {};
  bless($self, $classname);
  $self->_init(@_);
  return $self;
}

sub setHandlers {
  my $self = shift;
  my %handlers = @_;
  my ($key, $value);
  while (($key, $value) = each(%handlers)) {
    $self->{handlers}->{$key} = $value;
  }
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
  return scalar(@{$self->{errors}});
}

sub get_errors {
  my $self = shift;
  return @{$self->{errors}};
}

sub _init {
  my $self = shift;
  my ($userdata, %handlers) = @_;

  my $parser = new XML::Parser::ExpatNB; # non blocking parser
  $parser->setHandlers(Start => sub {$self->_handle_start(@_);});
  $parser->setHandlers(End => sub {$self->_handle_end(@_);});
  $parser->setHandlers(Char => sub {$self->_handle_char(@_);});
  $self->{parser} = $parser;
  $self->{userdata} = $userdata;
  $self->{handlers} = {};
  $self->{expected} = [];
  $self->{errors} = [];
  $self->{text} = undef;
  $self->_expand($ST_START);
  $self->setHandlers(%handlers);
}

sub _expand {
  my $self = shift;
  my ($state) = @_;
  my @substates = @{$state};
  for (my $i = scalar(@substates) - 1; $i >= 0; $i--) {
    my $expect = {%{$substates[$i]}, SEEN => 0};
    $expect->{RPT} = 0 unless(defined($expect->{RPT}));
    push(@{$self->{expected}}, $expect);
  }
}

sub _find {
  my $self = shift;
  my ($element) = @_;
  my $top;
  # make sure the element is expected
  while (1) {
    $top = ${$self->{expected}}[-1];
    die("Unexpected element " . $element . "\n") unless (defined($top));
    # break out of loop if we've found the element
    last if ($top->{ELE} eq $element);
    # make sure this element can be discarded
    unless ($top->{SEEN} > 0 || $top->{RPT} == $RPT_OPT || $top->{RPT} == $RPT_ANY) {
      die("Missing element " . $top->{ELE} . "\n"); 
    }
    pop(@{$self->{expected}});
  }
  return $top;
}

sub _handle_start {
  my $self = shift;
  my ($expat, $element, @attr_pairs) = @_;

  my $top = $self->_find($element);
  # check that we can have more of this element
  unless ($top->{SEEN} == 0 || $top->{RPT} == $RPT_ALO || $top->{RPT} == $RPT_ANY) {
    die("Too many of element " . $top->{ELE} . "\n");
  }
  # increment the sighting count
  $top->{SEEN} += 1;
  # process
  $top->{START}($self, @attr_pairs) if (defined($top->{START}));
  # expand states
  $self->_expand($top->{TO}) if (defined($top->{TO}));
  if ($top->{CHARS}) {
    $self->{text} = '';
  } else {
    $self->{text} = undef;
  }
}

sub _handle_end {
  my $self = shift;
  my ($expat, $element) = @_;
  my $top = $self->_find($element);
  # process
  $top->{END}($self) if (defined($top->{END}));
}

sub _handle_char {
  my $self = shift;
  my ($expat, $string) = @_;
  $self->{text} .= $string if defined $self->{text};
}

sub _error {
  my $self = shift;
  my ($message) = @_;
  push(@{$self->{errors}}, $message);
}

sub _valid {
  my $self = shift;
  my ($attrs, $elem, $attr, $pattern) = @_;
  my $value;
  if (defined($attrs)) {
    $value = $attrs->{$attr};
  } else {
    $value = $self->{text};
  }
  unless (defined($value)) {
    $self->_error("$elem/\@$attr missing");
    return undef;
  }
  if (defined($pattern)) {
    unless ($value =~ m/$pattern/) {
      $self->_error("$elem/\@$attr has invalid value \"$value\"");
      return undef;
    }
    if (wantarray) {
      return ($1, $2, $3, $4, $5, $6, $7, $8, $9);
    } else {
      return $1;
    }
  } else {
    return $value;
  }
}

sub _call {
  my $self = shift;
  my ($call, @args) = @_;

  if (defined($self->{handlers}->{$call}) &&  !$self->has_errors()) {
    $self->{handlers}->{$call}($self->{userdata}, @args);
  }
}

##############################################################################
#                           HANDLE ELEMENTS
##############################################################################

sub _start_ele_meme {
  my ($state, %attrs) = @_;
  my $release = $state->_valid(\%attrs, 'MEME', 'release');
  my ($major_ver, $minor_ver, $patch_ver) = $state->_valid(
    \%attrs, 'MEME', 'version', qr/^(\d+)(?:\.(\d+)(?:\.(\d+))?)?$/);
  $state->_call('start_meme', $major_ver, $minor_ver, $patch_ver, $release);
}

sub _end_ele_meme {
  my ($state) = @_;
  $state->_call('end_meme');
}

sub _start_ele_training_set {
  my ($state, %attrs) = @_;
  my $datafile = $state->_valid(\%attrs, 'training_set', 'datafile');
  my $length = $state->_valid(\%attrs, 'training_set', 'length', qr/^(\d+)$/);
  $state->_call('start_training_set', $datafile, $length);
}

sub _end_ele_training_set {
  my ($state) = @_;
  $state->_call('end_training_set');
}

sub _start_ele_alphabet {
  my ($state, %attrs) = @_;
  my $id = $state->_valid(\%attrs, 'alphabet', 'id', qr/^(nucleotide|amino-acid)$/);
  my $is_nucleotide = ($id eq 'nucleotide');
  my $length = $state->_valid(\%attrs, 'alphabet', 'length', qr/^(\d+)$/);
  $state->_call('start_alphabet', $is_nucleotide, $length);
}

sub _end_ele_alphabet {
  my ($state) = @_;
  $state->_call('end_alphabet');
}

sub _start_ele_alphabet_letter {
  my ($state, %attrs) = @_;
  my $id = $state->_valid(\%attrs, 'letter', 'id');
  my $symbol = $state->_valid(\%attrs, 'letter', 'symbol', qr/^([A-Z*-])$/);
  $state->_call('handle_alphabet_letter', $id, $symbol);
}

sub _start_ele_ambigs {
  my ($state, %attrs) = @_;
  $state->_call('start_ambigs');
}

sub _end_ele_ambigs {
  my ($state) = @_;
  $state->_call('end_ambigs');
}

sub _start_ele_ambigs_letter {
  my ($state, %attrs) = @_;
  my $id = $state->_valid(\%attrs, 'letter', 'id');
  my $symbol = $state->_valid(\%attrs, 'letter', 'symbol', qr/^([A-Z*-])$/);
  $state->_call('handle_ambigs_letter', $id, $symbol);
}

sub _start_ele_sequence {
  my ($state, %attrs) = @_;
  my $id = $state->_valid(\%attrs, 'sequence', 'id');
  my $name = $state->_valid(\%attrs, 'sequence', 'name');
  my $length = $state->_valid(\%attrs, 'sequence', 'length', qr/^(\d+)$/);
  my $weight = $state->_valid(\%attrs, 'sequence', 'weight', $num_re);
  $state->_call('handle_sequence', $id, $name, $length, $weight);
}

sub _start_ele_letter_frequencies {
  my ($state, %attrs) = @_;
  $state->_call('start_letter_frequencies');
}

sub _end_ele_letter_frequencies {
  my ($state) = @_;
  $state->_call('end_letter_frequencies');
}

sub _start_ele_lf_alphabet_array {
  my ($state, %attrs) = @_;
  $state->_call('start_lf_alphabet_array');
}

sub _end_ele_lf_alphabet_array {
  my ($state) = @_;
  $state->_call('end_lf_alphabet_array');
}

sub _start_ele_lf_aa_value {
  my ($state, %attrs) = @_;
  my $letter_id = $state->_valid(\%attrs, 'value', 'letter_id');
  $state->{value_letter_id} = $letter_id;
}

sub _end_ele_lf_aa_value {
  my ($state) = @_;
  my $frequency = $state->_valid(undef, 'value', 'value', 
    qr/^\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*$/);
  $state->_call('handle_lf_aa_value', $state->{value_letter_id}, $frequency);
}

sub _start_ele_model {
  my ($state, %attrs) = @_;
  $state->_call('start_model');
}

sub _end_ele_model {
  my ($state) = @_;
  $state->_call('end_model');
}

sub _end_ele_command_line {
  my ($state) = @_;
  my $command_line = $state->_valid(undef, 'command_line', 'value', qr/^\s*(.+?)\s*$/);
  $state->_call('handle_command_line', $command_line);
}

sub _end_ele_host {
  my ($state) = @_;
  my $host = $state->_valid(undef, 'host', 'value', qr/^\s*(.+?)\s*$/);
  $state->_call('handle_host', $host);
}

sub _end_ele_type {
  my ($state) = @_;
  my $type = $state->_valid(undef, 'type', 'value', qr/^\s*(anr|oops|tcm|zoops)\s*$/);
  $type = 'tcm' if ($type eq 'anr');
  $state->_call('handle_type', $type);
}

sub _end_ele_nmotifs {
  my ($state) = @_;
  my $nmotifs = $state->_valid(undef, 'nmotifs', 'value', qr/^\s*(\d+)\s*$/);
  $state->_call('handle_nmotifs', $nmotifs);
}

sub _end_ele_evalue_threshold {
  my ($state) = @_;
  my $num = $state->_valid(undef, 'evalue_threshold', 'value', 
    qr/^\s*((?:[-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)|inf)\s*$/);
  $state->_call('handle_evalue_threshold', $num);
}

sub _end_ele_object_function {
  my ($state) = @_;
  my $ob = $state->_valid(undef, 'object_function', 'value', qr/^\s*(.+?)\s*$/);
  $state->_call('handle_object_function', $ob);
}

sub _end_ele_min_width {
  my ($state) = @_;
  my $min_width = $state->_valid(undef, 'min_width', 'value', qr/\s*(\d+)\s*/);
  $state->_call('handle_min_width', $min_width);
}

sub _end_ele_max_width {
  my ($state) = @_;
  my $max_width = $state->_valid(undef, 'max_width', 'value', qr/\s*(\d+)\s*/);
  $state->_call('handle_max_width', $max_width);
}

sub _end_ele_minic {
  my ($state) = @_;
  my $minic = $state->_valid(undef, 'minic', 'value', 
    qr/^\s*(\+?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*$/);
  $state->_call('handle_minic', $minic);
}

sub _end_ele_wg {
  my ($state) = @_;
  my $wg = $state->_valid(undef, 'wg', 'value', 
    qr/^\s*([+-]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*$/);
  $state->_call('handle_wg', $wg);
}

sub _end_ele_ws {
  my ($state) = @_;
  my $ws = $state->_valid(undef, 'ws', 'value', 
    qr/^\s*([+-]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*$/);
  $state->_call('handle_ws', $ws);
}

sub _end_ele_endgaps {
  my ($state) = @_;
  my $endgaps = $state->_valid(undef, 'endgaps', 'value', qr/^\s*(no|yes)\s*$/);
  my $allow_endgaps = ($endgaps eq 'yes');
  $state->_call('handle_endgaps', $allow_endgaps);
}

sub _end_ele_minsites {
  my ($state) = @_;
  my $minsites = $state->_valid(undef, 'minsites', 'value', qr/^\s*(\d+)\s*$/);
  $state->_call('handle_minsites', $minsites);
}

sub _end_ele_maxsites {
  my ($state) = @_;
  my $maxsites = $state->_valid(undef, 'maxsites', 'value', qr/^\s*(\d+)\s*$/);
  $state->_call('handle_maxsites', $maxsites);
}

sub _end_ele_wnsites {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'wnsites', 'value', $num_re);
  $state->_call('handle_wnsites', $value);
}

sub _end_ele_prob {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'prob', 'value', $num_re);
  $state->_call('handle_prob', $value);
}

sub _end_ele_spmap {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'spmap', 'value', qr/^\s*(pam|uni)\s*$/);
  $state->_call('handle_spmap', $value);
}

sub _end_ele_spfuzz {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'spfuzz', 'value', $num_re);
  $state->_call('handle_spfuzz', $value);
}

sub _end_ele_prior {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'prior', 'value', 
    qr/^\s*(addone|dirichlet|dmix|megadmix|megap)\s*$/);
  $state->_call('handle_prior', $value);
}

sub _end_ele_beta {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'beta', 'value', $num_re);
  $state->_call('handle_beta', $value);
}

sub _end_ele_maxiter {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'maxiter', 'value', qr/^\s*(\d+)\s*$/);
  $state->_call('handle_maxiter', $value);
}

sub _end_ele_distance {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'distance', 'value', $num_re);
  $state->_call('handle_distance', $value);
}

sub _end_ele_num_sequences {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'num_sequences', 'value', qr/^\s*(\d+)\s*$/);
  $state->_call('handle_num_sequences', $value);
}

sub _end_ele_num_positions {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'num_positions', 'value', qr/^\s*(\d+)\s*$/);
  $state->_call('handle_num_positions', $value);
}

sub _end_ele_seed {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'seed', 'value', qr/^\s*(\-?\d+)\s*$/);
  $state->_call('handle_seed', $value);
}

sub _end_ele_seqfrac {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'seqfrac', 'value', $num_re);
  $state->_call('handle_seqfrac', $value);
}

sub _end_ele_strands {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'strands', 'value', qr/^\s*(both|forward|none)\s*$/);
  $state->_call('handle_strands', $value);
}

sub _end_ele_priors_file {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'priors_file', 'value', qr/^\s*(.*?)\s*$/);
  $state->_call('handle_priors_file', $value);
}

sub _end_ele_reason_for_stopping {
  my ($state) = @_;
  my $value = $state->_valid(undef, 'reason_for_stopping', 'value', qr/^\s*(.+?)\s*$/);
  $state->_call('handle_reason_for_stopping', $value);
}

sub _start_ele_background_frequencies {
  my ($state, %attrs) = @_;
  my $source = $state->_valid(\%attrs, 'background_frequencies', 'source');
  $state->_call('start_background_frequencies', $source);
}

sub _end_ele_background_frequencies {
  my ($state) = @_;
  $state->_call('end_background_frequencies'); 
}

sub _start_ele_bf_alphabet_array {
  my ($state, %attrs) = @_;
  $state->_call('start_bf_alphabet_array'); 
}

sub _end_ele_bf_alphabet_array {
  my ($state) = @_;
  $state->_call('end_bf_alphabet_array'); 
}

sub _start_ele_bf_aa_value {
  my ($state, %attrs) = @_;
  my $letter_id = $state->_valid(\%attrs, 'value', 'letter_id');
  $state->{value_letter_id} = $letter_id;
}

sub _end_ele_bf_aa_value {
  my ($state) = @_;
  my $frequency = $state->_valid(undef, 'value', 'value', 
    qr/^\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*$/);
  $state->_call('handle_bf_aa_value', $state->{value_letter_id}, $frequency);
}

sub _start_ele_motifs {
  my ($state, %attributes) = @_;
  $state->_call('start_motifs');
}

sub _end_ele_motifs {
  my ($state) = @_;
  $state->_call('end_motifs');
}

sub _start_ele_motif {
  my ($state, %attrs) = @_;
  my $id = $state->_valid(\%attrs, 'motif', 'id');
  my $name = $state->_valid(\%attrs, 'motif', 'name');
  my $width = $state->_valid(\%attrs, 'motif', 'width', qr/^\s*(\d+)\s*$/);
  my $sites = $state->_valid(\%attrs, 'motif', 'sites', qr/^\s*(\d+)\s*$/);
  my $llr = $state->_valid(\%attrs, 'motif', 'llr', $num_re);
  my $ic = $state->_valid(\%attrs, 'motif', 'ic', $num_re);
  my $re = $state->_valid(\%attrs, 'motif', 're', $num_re);
  my $bayes_threshold = $state->_valid(\%attrs, 'motif', 'bayes_threshold', $num_re);
  my $e_value = $state->_valid(\%attrs, 'motif', 'e_value', $num_re);
  my $elapsed_time = $state->_valid(\%attrs, 'motif', 'elapsed_time', $num_re);
  my $url = $attrs{url}; # optional attribute

  $state->_call('start_motif',$id, $name, $width, $sites, $llr, $ic, $re, 
    $bayes_threshold, $e_value, $elapsed_time, $url);
}

sub _end_ele_motif {
  my ($state) = @_;
  $state->_call('end_motif');
}

sub _start_ele_scores {
  my ($state, %attrs) = @_;
  $state->_call('start_scores');
}

sub _end_ele_scores {
  my ($state) = @_;
  $state->_call('end_scores');
}

sub _start_ele_sc_alphabet_matrix {
  my ($state, %attrs) = @_;
  $state->_call('start_sc_alphabet_matrix');
}

sub _end_ele_sc_alphabet_matrix {
  my ($state) = @_;
  $state->_call('end_sc_alphabet_matrix');
}

sub _start_ele_sc_am_alphabet_array {
  my ($state, %attrs) = @_;
  $state->_call('start_sc_am_alphabet_array');
}

sub _end_ele_sc_am_alphabet_array {
  my ($state) = @_;
  $state->_call('end_sc_am_alphabet_array');
}

sub _start_ele_sc_am_aa_value {
  my ($state, %attrs) = @_;
  my $letter_id = $state->_valid(\%attrs, 'value', 'letter_id');
  $state->{value_letter_id} = $letter_id;
}

sub _end_ele_sc_am_aa_value {
  my ($state) = @_;
  my $score = $state->_valid(undef, 'value', 'value', 
    qr/^\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*$/);
  $state->_call('handle_sc_am_aa_value', $state->{value_letter_id}, $score);
}

sub _start_ele_probabilities {
  my ($state, %attrs) = @_;
  $state->_call('start_probabilities');
}

sub _end_ele_probabilities {
  my ($state) = @_;
  $state->_call('end_probabilities');
}

sub _start_ele_pr_alphabet_matrix {
  my ($state, %attrs) = @_;
  $state->_call('start_pr_alphabet_matrix');
}

sub _end_ele_pr_alphabet_matrix {
  my ($state) = @_;
  $state->_call('end_pr_alphabet_matrix');
}

sub _start_ele_pr_am_alphabet_array {
  my ($state, %attrs) = @_;
  $state->_call('start_pr_am_alphabet_array');
}

sub _end_ele_pr_am_alphabet_array {
  my ($state) = @_;
  $state->_call('end_pr_am_alphabet_array');
}

sub _start_ele_pr_am_aa_value {
  my ($state, %attrs) = @_;
  my $letter_id = $state->_valid(\%attrs, 'value', 'letter_id');
  $state->{value_letter_id} = $letter_id;
}

sub _end_ele_pr_am_aa_value {
  my ($state) = @_;
  my $score = $state->_valid(undef, 'value', 'value', 
    qr/^\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)\s*$/);
  $state->_call('handle_pr_am_aa_value', $state->{value_letter_id}, $score);
}

sub _end_ele_regular_expression {
  my ($state) = @_;
  my $re = $state->_valid(undef, 'regular_expression', 'value', qr/^\s*([A-Z\[\]]+)\s*$/);
  $state->_call('handle_regular_expression', $re);
}

sub _start_ele_contributing_sites {
  my ($state) = @_;
  $state->_call('start_contributing_sites');
}

sub _end_ele_contributing_sites {
  my ($state) = @_;
  $state->_call('end_contributing_sites');
}

sub _start_ele_contributing_site {
  my ($state, %attrs) = @_;
  my $sequence_id = $state->_valid(\%attrs, 'contributing_site', 'sequence_id');
  my $position = $state->_valid(\%attrs, 'contributing_site', 'position', qr/^\s*(\d+)\s*$/);
  my $strand = $state->_valid(\%attrs, 'contributing_site', 'strand', qr/^\s*(plus|minus|none)\s*$/);
  my $pvalue = $state->_valid(\%attrs, 'contributing_site', 'pvalue', $num_re);
  $state->_call('start_contributing_site', $sequence_id, $position, $strand, $pvalue);
}

sub _end_ele_contributing_site {
  my ($state) = @_;
  $state->_call('end_contributing_site');
}

sub _end_ele_left_flank {
  my ($state) = @_;
  my $left_flank = $state->_valid(undef, 'left_flank', 'value', qr/\s*([A-Z]*)\s*/);
  $state->_call('handle_left_flank', $left_flank);
}

sub _start_ele_site {
  my ($state, %attrs) = @_;
  $state->_call('start_site');
}

sub _end_ele_site {
  my ($state) = @_;
  $state->_call('end_site');
}

sub _start_ele_letter_ref {
  my ($state, %attrs) = @_;
  my $letter_id = $state->_valid(\%attrs, 'letter_ref', 'letter_id');
  $state->_call('handle_letter_ref', $letter_id);
}

sub _end_ele_right_flank {
  my ($state) = @_;
  my $right_flank = $state->_valid(undef, 'right_flank', 'value', qr/\s*([A-Z]*)\s*/);
  $state->_call('handle_right_flank', $right_flank);
}

sub _start_ele_scanned_sites_summary {
  my ($state, %attrs) = @_;
  my $p_thresh = $state->_valid(\%attrs, 'scanned_sites_summary', 'p_thresh', $num_re);
  $state->_call('start_scanned_sites_summary', $p_thresh);
}

sub _end_ele_scanned_sites_summary {
  my ($state) = @_;
  $state->_call('end_scanned_sites_summary');
}

sub _start_ele_scanned_sites {
  my ($state, %attrs) = @_;
  my $sequence_id = $state->_valid(\%attrs, 'scanned_sites', 'sequence_id');
  my $pvalue = $state->_valid(\%attrs, 'scanned_sites', 'pvalue', $num_re);
  my $num_sites = $state->_valid(\%attrs, 'scanned_sites', 'num_sites', qr/^\s*(\d+)\s*$/);
  $state->_call('start_scanned_sites', $sequence_id, $pvalue, $num_sites);

}

sub _end_ele_scanned_sites {
  my ($state) = @_;
  $state->_call('end_scanned_sites');
}

sub _start_ele_scanned_site {
  my ($state, %attrs) = @_;
  my $motif_id = $state->_valid(\%attrs, 'scanned_site', 'motif_id');
  my $strand = $state->_valid(\%attrs, 'scanned_site', 'strand', qr/^(plus|minus|none)$/);
  my $position = $state->_valid(\%attrs, 'scanned_site', 'position', qr/^(\d+)$/);
  my $pvalue = $state->_valid(\%attrs, 'scanned_site', 'pvalue', $num_re);
  $state->_call('handle_scanned_site', $motif_id, $strand, $position, $pvalue);
}
