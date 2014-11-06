package JsonWr;

use strict;
use warnings;

use Carp qw(cluck confess);
use Data::Dumper;
use Scalar::Util qw(looks_like_number);

my $STATE_ERROR = 0;
my $STATE_EMPTY_OBJECT = 1;
my $STATE_OBJECT = 2;
my $STATE_EMPTY_ARRAY = 3;
my $STATE_SL_ARRAY = 4; # single line array
my $STATE_ML_ARRAY = 5; # multi line array
my $STATE_PROPERTY = 6;
my $STATE_DONE = 7;

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

sub done {
  my $self = shift;
  $self->_unwind_stack();
}

sub property {
  my $self = shift;
  $self->{logger}->trace('call JsonWr::property( '. Dumper(@_) . ' )') if $self->{logger};
  my ($property) = @_;
  confess("JSON property name was not defined!") unless (defined($property));
  my $dest = $self->{dest};
  $self->_enforce_state($STATE_EMPTY_OBJECT, $STATE_OBJECT);
  if ($self->{state} != $STATE_EMPTY_OBJECT) {
    print $dest ',';
  }
  $self->_write_nl_indent();
  print $dest $self->_convert_string($property), ': ';
  push(@{$self->{stack}}, $STATE_OBJECT);
  $self->{state} = $STATE_PROPERTY;
}

sub start_object_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::start_object_value") if $self->{logger};
  my $dest = $self->{dest};
  $self->_write_start($STATE_EMPTY_OBJECT);
  print $dest '{';
  $self->{column} += 1;
}

sub end_object_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::end_object_value") if $self->{logger};
  my $dest = $self->{dest};
  $self->_enforce_state($STATE_EMPTY_OBJECT, $STATE_OBJECT);
  $self->{indent} -= $self->{tab_cols};
  if ($self->{state} != $STATE_EMPTY_OBJECT) {
    $self->_write_nl_indent();
  }
  print $dest '}';
  $self->{column} += 1;
  die("Empty state stack") unless @{$self->{stack}};
  $self->{state} = pop(@{$self->{stack}});
}

sub start_array_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::start_array_value") if $self->{logger};
  $self->_write_start($STATE_EMPTY_ARRAY);
  # note we don't write the [ yet
}

sub end_array_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::end_array_value") if $self->{logger};
  my $dest = $self->{dest};
  $self->_enforce_state($STATE_EMPTY_ARRAY, $STATE_SL_ARRAY, $STATE_ML_ARRAY);
  $self->{indent} -= $self->{tab_cols};
  if ($self->{state} == $STATE_ML_ARRAY) {
    $self->_write_nl_indent();
  } else {
    my $line_len = ($self->{state} == $STATE_SL_ARRAY ? length($self->{line_buf}) : 0);
    if (($self->{column} + 1 + $line_len + 2) >= $self->{line_cols}) {
      $self->_write_nl_indent()
    }
    print $dest '[';
    $self->{column} += 1;
  }
  if ($self->{state} == $STATE_SL_ARRAY) {
    print $dest $self->{line_buf};
    $self->{column} += length($self->{line_buf});
  }
  print $dest ']';
  $self->{column} += 1;
  die("Empty state stack") unless @{$self->{stack}};
  $self->{state} = pop(@{$self->{stack}});
}

sub null_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::null_value( ". Dumper(@_).' )') if $self->{logger};
  $self->_write_value("null");
}

sub str_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::str_value( ". Dumper(@_).' )') if $self->{logger};
  my ($value) = @_;
  unless (defined($value)) {
    cluck("JSON string value was not defined!");
    $value = "";
  }
  $self->_write_value($self->_convert_string($value));
}

sub nstr_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::nstr_value( ". Dumper(@_).' )') if $self->{logger};
  my ($value) = @_;
  if (defined($value)) {
    $self->_write_value($self->_convert_string($value));
  } else {
    $self->null_value();
  }
}

sub num_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::num_value( ". Dumper(@_).' )') if $self->{logger};
  my ($value) = @_;
  die("Value $value is not a number!") unless looks_like_number($value);
  $self->_write_value($value);
}

sub bool_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::bool_value( ". Dumper(@_).' )') if $self->{logger};
  my ($value) = @_;
  $self->_write_value(($value ? "true" : "false"));
}

sub str_array_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::str_array_value( ". Dumper(@_).' )') if $self->{logger};
  my (@values) = @_;
  $self->start_array_value();
  for (my $i = 0; $i < scalar(@values); $i++) {
    $self->str_value($values[$i]);
  }
  $self->end_array_value();
}

sub nstr_array_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::nstr_array_value( ". Dumper(@_).' )') if $self->{logger};
  my (@values) = @_;
  $self->start_array_value();
  for (my $i = 0; $i < scalar(@values); $i++) {
    $self->nstr_value($values[$i]);
  }
  $self->end_array_value();
}

sub num_array_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::num_array_value( ".Dumper(@_).' )') if $self->{logger};
  my (@values) = @_;
  $self->start_array_value();
  for (my $i = 0; $i < scalar(@values); $i++) {
    $self->num_value($values[$i]);
  }
  $self->end_array_value();
}

sub bool_array_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::bool_array_value( ".Dumper(@_).' )') if $self->{logger};
  my (@values) = @_;
  $self->start_array_value();
  for (my $i = 0; $i < scalar(@values); $i++) {
    $self->bool_value($values[$i]);
  }
  $self->end_array_value();
}

sub null_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::null_prop( ".Dumper(@_).' )') if $self->{logger};
  my ($property) = @_;
  $self->property($property);
  $self->null_value();
}

sub str_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::str_prop( ".Dumper(@_).' )') if $self->{logger};
  my ($property, $value) = @_;
  $self->property($property);
  $self->str_value($value);
}

sub nstr_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::nstr_prop( ".Dumper(@_).' )') if $self->{logger};
  my ($property, $value) = @_;
  $self->property($property);
  $self->nstr_value($value);
}

sub num_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::num_prop( ". Dumper(@_).' )') if $self->{logger};
  my ($property, $value) = @_;
  $self->property($property);
  $self->num_value($value);
}

sub bool_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::bool_prop( ". Dumper(@_).' )') if $self->{logger};
  my ($property, $value) = @_;
  $self->property($property);
  $self->bool_value($value);
}

sub str_array_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::str_array_prop( ". Dumper(@_).' )') if $self->{logger};
  my ($property, @values) = @_;
  $self->property($property);
  $self->str_array_value(@values);
}

sub nstr_array_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::nstr_array_prop( ". Dumper(@_).' )') if $self->{logger};
  my ($property, @values) = @_;
  $self->property($property);
  $self->nstr_array_value(@values);
}

sub num_array_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::num_array_prop( ". Dumper(@_).' )') if $self->{logger};
  my ($property, @values) = @_;
  $self->property($property);
  $self->num_array_value(@values);
}

sub bool_array_prop {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::bool_array_prop( ". Dumper(@_).' )') if $self->{logger};
  my ($property, @values) = @_;
  $self->property($property);
  $self->bool_array_value(@values);
}

sub _init {
  my $self = shift;
  my ($dest, $min_cols, $tab_cols, $line_cols) = @_;
  $self->{dest} = $dest;
  $self->{min_cols} = $min_cols;
  $self->{tab_cols} = $tab_cols;
  $self->{line_cols} = $line_cols;
  $self->{indent} = $min_cols + $tab_cols;
  $self->{column} = 0;
  $self->{state} = $STATE_EMPTY_OBJECT;
  $self->{stack} = [];
  push(@{$self->{stack}}, $STATE_DONE);
  print $dest '{';
}

sub _enforce_state {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::_enforce_state( ". Dumper(@_).' )') if $self->{logger};
  my (@states) = @_;
  for (my $i = 0; $i < scalar(@states); $i++) {
    return if ($self->{state} == $states[$i]);
  }
  die("Illegal state");
}

sub _unwind_stack {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::_unwind_stack") if $self->{logger};
  while ($self->{state} != $STATE_DONE) {
    if ($self->{state} == $STATE_EMPTY_OBJECT || 
        $self->{state} == $STATE_OBJECT) {
      $self->end_object_value();  
    } elsif ($self->{state} == $STATE_EMPTY_ARRAY || 
        $self->{state} == $STATE_SL_ARRAY || $self->{state} == $STATE_ML_ARRAY) {
      $self->end_array_value();
    } elsif ($self->{state} == $STATE_PROPERTY) {
      $self->null_value();
    } else {
      die("Illegal state");
    }
  }
}

sub _convert_string {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::_convert_string( ". Dumper(@_).' )') if $self->{logger};
  my ($string) = @_;
  my $storage = '"';
  for (my $i = 0; $i < length($string); $i++) {
    my $letter = substr($string, $i, 1);
    my $num = ord($letter);
    if ($letter eq '"') {
      $storage .= "\\\"";
    } elsif ($letter eq "\\") {
      $storage .= "\\\\";
    } elsif ($letter eq '/') {
      $storage .= "\\/";
    } elsif ($num >= 0 && $num <= 0x1F || $num >= 0x7F && $num <= 0x9F ||
      $num == 0x2028 || $num == 0x2029) {
      # control character, so encode
      # Note that techinically U+2028 and U+2029 are valid JSON but they are
      # not valid Javascript as they encode 'LINE SEPARATOR' (U+2028) and
      # 'PARAGRAPH SEPARATOR' (U+2029) which are treated by Javascript as
      # newline characters and are hence not valid in strings.
      if ($num == 8) { #backspace
        $storage .= "\\b";
      } elsif ($num == 9) { #tab
        $storage .= "\\t";
      } elsif ($num == 10) { # line feed (newline)
        $storage .= "\\n";
      } elsif ($num == 12) { # form feed
        $storage .= "\\f";
      } elsif ($num == 13) { # carriage return
        $storage .= "\\r";
      } else {
        $storage .= sprintf("\\u%.04x", $num);
      }
    } else {
      $storage .= $letter;
    }
  }
  $storage .= '"';
  return $storage;
}

sub _write_nl_indent {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::_write_nl_indent") if $self->{logger};
  my $dest = $self->{dest};
  print $dest "\n" . (' ' x $self->{indent});
  $self->{column} = $self->{indent};
}

sub _write_start {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::_write_start(". Dumper(@_).' )') if $self->{logger};
  my ($new_state) = @_;
  my $dest = $self->{dest};
  $self->_enforce_state($STATE_PROPERTY, $STATE_EMPTY_ARRAY, $STATE_SL_ARRAY, 
      $STATE_ML_ARRAY);
  if ($self->{state} != $STATE_PROPERTY) { # an array
    if ($self->{state} != $STATE_ML_ARRAY) {
      print $dest '[';
      $self->{column} += 1;
      $self->_write_nl_indent();
    }
    if ($self->{state} == $STATE_SL_ARRAY) {
      print $dest $self->{line_buf};
      $self->{column} += length($self->{line_buf});
    }
    if ($self->{state} != $STATE_EMPTY_ARRAY) {
      print $dest ', ';
      $self->{column} += 2;
    }
    push(@{$self->{stack}}, $STATE_ML_ARRAY);
    $self->_write_nl_indent() if (($self->{column} + 1) >= $self->{line_cols});
  }
  $self->{state} = $new_state;
  $self->{column} += 1;
  $self->{indent} += $self->{tab_cols};
}

sub _write_value {
  my $self = shift;
  $self->{logger}->trace("call JsonWr::_write_value( ". Dumper(@_).' )') if $self->{logger};
  my ($value) = @_;
  my $dest = $self->{dest};
  $self->_enforce_state($STATE_PROPERTY, $STATE_EMPTY_ARRAY, $STATE_SL_ARRAY, 
    $STATE_ML_ARRAY);
  my $val_len = length($value);
  if ($self->{state} == $STATE_EMPTY_ARRAY) {
    if (($self->{indent} + 1 + length($value) + 2) < $self->{line_cols}) {
      $self->{line_buf} = $value;
      $self->{state} = $STATE_SL_ARRAY;
      return; # don't write anything yet
    } else {
      print $dest '[';
      $self->{column} += 1;
      $self->_write_nl_indent();
    }
  } elsif ($self->{state} == $STATE_SL_ARRAY) {
    if (($self->{indent} + 1 + length($self->{line_buf}) + 2 + length($value) + 2) < $self->{line_cols}) {
      $self->{line_buf} .= ', ' . $value;
      return;
    } else {
      print $dest '[';
      $self->{column} += 1;
      $self->_write_nl_indent();
      print $dest $self->{line_buf};
      $self->{column} += length($self->{line_buf});
      $self->{state} = $STATE_ML_ARRAY;
    }
  }
  if ($self->{state} == $STATE_ML_ARRAY) {
    print $dest ',';
    $self->{column} += 1;
    if (($self->{column} + 1 + length($value) + 2) < $self->{line_cols}) {
      print $dest ' ';
      $self->{column} += 1;
    } else {
      $self->_write_nl_indent();
    }
  }
  print $dest $value;
  $self->{column} += length($value);
  if ($self->{state} == $STATE_PROPERTY) {
    die("Empty state stack") unless @{$self->{stack}};
    $self->{state} = pop(@{$self->{stack}});
  } else { # ARRAY
    $self->{state} = $STATE_ML_ARRAY;
  }
}

1;
