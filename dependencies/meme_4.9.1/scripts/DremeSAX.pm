package DremeSAX;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw();

use XML::Parser::Expat;
use Scalar::Util qw(looks_like_number);

my $num_re = qr/^((?:[-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)|inf)$/;
my $num_trim_re = qr/^\s*((?:[-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)|inf)\s*$/;
my $int_re = qr/^(\d+)$/;
my $int_trim_re = qr/^\s*(\d+)\s*$/;
my $float_re = qr/^(\d+(?:\.\d+)?)$/;
my $float_trim_re = qr/^\s*(\d+(?:\.\d+)?)\s*$/;
my $trim_re = qr/^\s*(.*?)\s*$/;
my $text_re = qr/^(.*\S.*)$/;
my $nospace_re = qr/^(\S+)$/;

my $RPT_ONE = 0; # occur once only (default)
my $RPT_OPT = 1; # optionaly occur once
my $RPT_ALO = 2; # repeat one or more times
my $RPT_ANY = 3; # repeat any number of times

my $ST_IN_MOTIF = [
  {ELE => 'pos', RPT => $RPT_ALO, START => \&_start_ele_pos}, 
  {ELE => 'match', RPT => $RPT_ALO, ATRS => [
      {ATR => 'seq', VAL => qr/^([ACGTURYKMSWBDHVN]+)$/},
      {ATR => 'p', VAL => $int_re},
      {ATR => 'n', VAL => $int_re},
      {ATR => 'pvalue', VAL => $num_re},
      {ATR => 'evalue', VAL => $num_re}
    ]}
];
my $ST_IN_MOTIFS = [
  {ELE => 'motif', RPT => $RPT_ANY, TO => $ST_IN_MOTIF, ATRS => [
      {ATR => 'id', VAL => $nospace_re},
      {ATR => 'seq', VAL => qr/^([ACGTURYKMSWBDHVN]+)$/},
      {ATR => 'length', VAL => $int_re},
      {ATR => 'nsites', VAL => $int_re},
      {ATR => 'p', VAL => $int_re},
      {ATR => 'n', VAL => $int_re},
      {ATR => 'pvalue', VAL => $num_re},
      {ATR => 'evalue', VAL => $num_re},
      {ATR => 'unerased_evalue', VAL => $num_re}
    ]}];
my $ST_IN_MODEL = [
  {ELE => 'command_line', VAL => $text_re},
  {ELE => 'positives', ATRS => [
      {ATR => 'name'},
      {ATR => 'count', VAL => $int_re},
      {ATR => 'file'},
      {ATR => 'last_mod_date'}
    ]},
  {ELE => 'negatives', START => \&_start_ele_negatives},
  {ELE => 'background', START => \&_start_ele_background},
  {ELE => 'stop', ATRS => [
      {ATR => 'evalue', OPT => $num_re}, 
      {ATR => 'count', OPT => $int_re}, 
      {ATR => 'time', OPT => $int_re}]},
  {ELE => 'norc', VAL => qr/^\s*(TRUE|FALSE)\s*$/},
  {ELE => 'ngen', VAL => $int_trim_re},
  {ELE => 'add_pv_thresh', VAL => $num_trim_re},
  {ELE => 'seed', VAL => $int_trim_re},
  {ELE => 'host', VAL => $trim_re},
  {ELE => 'when', VAL => $trim_re},
  {ELE => 'description', RPT => $RPT_OPT, VAL => $trim_re}
];
my $ST_IN_DREME = [
  {ELE => 'model', TO => $ST_IN_MODEL}, 
  {ELE => 'motifs', TO => $ST_IN_MOTIFS},
  {ELE => 'run_time', ATRS => [
      {ATR => 'cpu', VAL => $trim_re}, 
      {ATR => 'real', VAL => $float_re}, 
      {ATR => 'stop', VAL => qr/^(evalue|count|time)$/}]}
];
my $ST_START = [
  {ELE => 'dreme', TO => $ST_IN_DREME, START => \&_start_ele_dreme}];


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
  $self->{alphabet_type} = undef;
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
  if (defined($top->{START})) {
    # the method for processing the attributes is complex or has some side effect
    # so it must be handled by a subroutine
    $top->{PARAMS} = [$top->{START}($self, @attr_pairs)];
  } else {
    my %values = @attr_pairs;
    my @expected = (defined($top->{ATRS}) ? @{$top->{ATRS}} : ());
    my @params = ();
    for (my $i = 0; $i < scalar(@expected); $i++) {
      my $expect = $expected[$i];
      my $attribute = $expect->{ATR};
      my $value = $values{$attribute};
      if (defined($value)) {
        my $pattern = (defined($expect->{VAL}) ? $expect->{VAL} : $expect->{OPT});
        if (defined($pattern)) {
          unless ($value =~ $pattern) {
            $self->_error("$element\@$attribute has invalid value \"$value\"");
            next;
          }
          $value = $1;
        }
      } else {
        $self->_error("$element\@$attribute missing") if (defined($expect->{VAL}));
      }
      push(@params, $value);
    }

    if (defined($top->{TO})) { #has sub states so need to call 'start_'
      my $call_name = 'start_' . (defined($top->{NAME}) ? $top->{NAME} : $element);
      $self->_call($call_name, @params);
    } else { # do the callback on end prefixed with the name 'handle_'
      $top->{PARAMS} = \@params;
    }
  }
  # expand states
  $self->_expand($top->{TO}) if (defined($top->{TO}));
  if ($top->{VAL}) {
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
  if (defined($top->{END})) {
    $top->{END}($self, $top, @{$top->{PARAMS}});
  } else {
    my $value = $self->{text};
    $self->{text} = undef;
    if (defined($top->{VAL})) {
      unless ($value =~ $top->{VAL}) {
        $self->_error("$element has invalid value \"$value\"");
      }
      $value = $1;
    }
    my @end_param = (defined($value) ? ($value) : ());
    if (defined($top->{TO})) { #has sub states so need to call 'end_'
      my $call_name = 'end_' . (defined($top->{NAME}) ? $top->{NAME} : $element);
      $self->_call($call_name, @end_param);
    } else { # call handle
      my $call_name = 'handle_' . (defined($top->{NAME}) ? $top->{NAME} : $element);
      $self->_call($call_name, @{$top->{PARAMS}}, @end_param);
    }
    
  }
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
  my ($attrs, $elem, $attr, $pattern, $optional) = @_;
  my $value;
  if (defined($attrs)) {
    $value = $attrs->{$attr};
  } else {
    $value = $self->{text};
  }
  unless (defined($value)) {
    $self->_error("$elem/\@$attr missing") unless $optional;
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

sub _start_ele_dreme {
  my ($self, %attrs) = @_;
  my $release = $self->_valid(\%attrs, 'dreme', 'release', $text_re);
  my ($vmajor, $vminor, $vpatch) = $self->_valid(\%attrs, 'dreme', 'version', qr/^(\d+)(?:\.(\d+)(?:\.(\d+))?)?$/);
  $vminor = 0 unless defined($vminor);
  $vpatch = 0 unless defined($vpatch);
  $self->_call('start_dreme', $vmajor, $vminor, $vpatch, $release);
}

sub _start_ele_negatives {
  my ($self, %attrs) = @_;
  my $name = $self->_valid(\%attrs, 'negatives', 'name', $text_re);
  my $count = $self->_valid(\%attrs, 'negatives', 'count', $int_re);
  my $from = $self->_valid(\%attrs, 'negatives', 'from', qr/^(file|shuffled)$/);
  my $file;
  my $last_mod;
  if ($from eq 'file') {
    $file = $self->_valid(\%attrs, 'negatives', 'file', $text_re);
    $last_mod = $self->_valid(\%attrs, 'negatives', 'last_mod_date', $text_re);
  }
  return ($name, $count, $from, $file, $last_mod);
}

sub _start_ele_background {
  my ($self, %attrs) = @_;
  my $type = $self->_valid(\%attrs, 'background', 'type', qr/^(dna|rna)$/);
  my $A = $self->_valid(\%attrs, 'background', 'A', $float_re);
  my $C = $self->_valid(\%attrs, 'background', 'C', $float_re);
  my $G = $self->_valid(\%attrs, 'background', 'G', $float_re);
  my $T;
  if ($type eq 'dna') {
    $T = $self->_valid(\%attrs, 'background', 'T', $float_re);
  } else { # $type eq 'rna'
    $T = $self->_valid(\%attrs, 'background', 'U', $float_re);
  }
  my $from = $self->_valid(\%attrs, 'background', 'from', qr/^(dataset|file)$/);
  my $file;
  my $last_mod;
  if ($from eq 'file') {
    $file = $self->_valid(\%attrs, 'background', 'file', $text_re);
    $last_mod = $self->_valid(\%attrs, 'background', 'last_mod_date', $text_re);
  }
  $self->{alphabet_type} = $type;
  return ($type, $A, $C, $G, $T, $from, $file, $last_mod);
}

sub _start_ele_pos {
  my ($self, %attrs) = @_;
  my $i = $self->_valid(\%attrs, 'pos', 'i', $int_re);
  my $A = $self->_valid(\%attrs, 'pos', 'A', $num_re);
  my $C = $self->_valid(\%attrs, 'pos', 'C', $num_re);
  my $G = $self->_valid(\%attrs, 'pos', 'G', $num_re);
  my $T;
  if ($self->{alphabet_type} eq 'dna') {
    $T = $self->_valid(\%attrs, 'pos', 'T', $num_re);
  } elsif ($self->{alphabet_type} eq 'rna') {
    $T = $self->_valid(\%attrs, 'pos', 'U', $num_re);
  }
  return ($i, $A, $C, $G, $T);
}
