package JsonRdr;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw();

use Fcntl qw(O_RDONLY);

my $LANDMARK = '@JSON_VAR ';

# When an error has occurred.
my $STATE_ERROR = 0;
# Occurs at the begining and after any JSON variables have been finished.
# The parser looks for the landmark "@JSON_VAR " and if one is found 
# transitions to the STATE_READ_VAR.
my $STATE_FIND_LANDMARK = 1;
# Occurs when reading the variable name of a JSON object.
# The parser reads the next space delimitered word and stores it as the 
# variable name. After the name has been read the parser transitions
# to STATE_FIND_START.
my $STATE_READ_VAR = 2;
# Occurs when scanning for the start of the JSON datastructure.
# The parser reads until it encouters a '{' and then pushes STATE_FIND_LANDMARK
# on the stack and transitions to STATE_PROPERTY_OR_ENDOBJ.
my $STATE_FIND_START = 3;
# Occurs when looking for a property or the end of the current object.
# The parser reads a token and if it finds a STRING it transitions to
# STATE_COLON (as that is the start of a property). Alternatively if
# the token is ENDOBJ it pops the stack.
my $STATE_PROPERTY_OR_ENDOBJ = 4;
# Occurs when looking for the colon that separates a key and value in a
# property. When it reads the COLON token it transitions to STATE_PROPERTY_VALUE.
my $STATE_COLON = 5;
# Occurs when looking for the value of a property and can accept the value
# tokens STRING, NUMBER, BOOL, NULL, STARTOBJ or STARTLST. After the 
# value has been read (complex for nested objects and lists) it transitions
# to STATE_COMMA_OR_ENDOBJ. Note that if the value is STARTOBJ or STARTLST it
# pushes STATE_COMMA_OR_ENDOBJ onto the stack before transitioning. For the
# STARTOBJ token it will then transition to STATE_PROPERTY_OR_ENDOBJ. For the
# STARTLST token it will then transition to START_VALUE_OR_ENDLST.
my $STATE_PROPERTY_VALUE = 6;
# Occurs after an object property has been read and the parser is looking
# for comma separating properties or the end of the object. If the next token
# is a COMMA then it transitions to STATE_PROPERTY, otherwise the next token
# should be ENDOBJ in which case it pops the stack.
my $STATE_COMMA_OR_ENDOBJ = 7;
# Occurs after a previous property has been read and a separating comma has
# been found implying another property to come. When it reads the STRING token
# it transitions to STATE_COLON.
my $STATE_PROPERTY = 8;
# Occurs when reading a value from a list.
# The parser reads a token and if it finds a value it transitions to 
# STATE_COMMA_OR_ENDLST. Alternatively if the TOKEN is ENDLST it pops the stack.
# Note that if the value is STARTOBJ or STARTLST it pushes the state 
# STATE_COMMA_OR_ENDLST on the stack before transitioning. If the token was 
# STARTOBJ it will transition to STATE_PROPERTY_OR_ENDOBJ. If the token was
# STARTLST it will transition to STATE_VALUE_OR_ENDLST.
my $STATE_VALUE_OR_ENDLST = 9;
# Occurs after reading a value from a list.
# The parser reads a token and if it finds a COMMA it transitions to 
# STATE_LIST_VALUE. Alternatively it should find a ENDLST and will then
# pop the stack.
my $STATE_COMMA_OR_ENDLST = 10;
# Occurs after finding a COMMA in a list implying another value to come.
# The parser reads a value and then will transition to STATE_COMMA_OR_ENDLST.
# If the value is STARTOBJ or STARTLST then STATE_COMMA_OR_ENDLST will be
# pushed onto the stack before the transition. STARTOBJ will cause a transition
# to STATE_PROPERTY_OR_ENDOBJ and STARTLST will transition to 
# STATE_VALUE_OR_ENDLST.
my $STATE_LIST_VALUE = 11;

# TOKENS
my $TOKEN_ILLEGAL = 0;
my $TOKEN_STARTOBJ = 1; # {
my $TOKEN_ENDOBJ = 2;   # }
my $TOKEN_STARTLST = 3; # [
my $TOKEN_ENDLST = 4;   # ]
my $TOKEN_COLON = 5;    # :
my $TOKEN_COMMA = 6;    # ,
my $TOKEN_NULL = 7;     # null
my $TOKEN_BOOLEAN = 8;  # true false
my $TOKEN_NUMBER = 9; 
my $TOKEN_STRING = 10;

my @TOKEN_NAMES = ("ILLEGAL", "STARTOBJ", "ENDOBJ", "STARTLST", "ENDLST", 
  "COLON", "COMMA", "NULL", "BOOLEAN", "NUMBER", "STRING");


sub new {
  my $classname = shift;
  my $self = {};
  bless($self, $classname);
  $self->_init(@_);
  return $self;
}

sub set_handlers {
  my $self = shift;
  my %handlers = @_;
  if (defined($handlers{start_data})) {
    $self->{start_data} = $handlers{start_data};
  }
  if (defined($handlers{end_data})) {
    $self->{end_data} = $handlers{end_data};
  }
  if (defined($handlers{property})) {
    $self->{property} = $handlers{property};
  }
  if (defined($handlers{atom})) {
    $self->{atom} = $handlers{atom};
  }
  if (defined($handlers{start_object})) {
    $self->{start_object} = $handlers{start_object};
  }
  if (defined($handlers{end_object})) {
    $self->{end_object} = $handlers{end_object};
  }
  if (defined($handlers{start_array})) {
    $self->{start_array} = $handlers{start_array};
  }
  if (defined($handlers{end_array})) {
    $self->{end_array} = $handlers{end_array};
  }
}

sub parse_more {
  my $self = shift;
  my ($buffer) = @_;
  $self->_parse($buffer, 0);
}

sub parse_done {
  my $self = shift;
  $self->_parse('', 1);
}

#
# load_file
#
# Read tagged embeded JSON data from a file and store it in
# Perl datastructures.
#
# filters is a list of compiled regular expression which match
# strings which give the hierarchy to that point in the data.
#
# An example string is given below, of course this assumes that 
# the property name does not contain the seperator character >.
#
# section>property>property>property
#
sub load_file {
  my $self = shift;
  my ($json_file, @filters) = @_;
  $self->{load} = 1;
  $self->{lsections} = {};
  $self->{filters} = \@filters;
  my $fh;
  sysopen($fh, $json_file, O_RDONLY) or die("Failed to open file \"$json_file\".");
  while (<$fh>) {
    $self->parse_more($_);
  }
  $self->parse_done();
  close($fh) or die("Failed to close file \"$json_file\".");
  my $sections = $self->{lsections};
  $self->{lsections} = undef;
  return $sections;
}

sub _init {
  my $self = shift;
  my ($userdata, %handlers) = @_;
  $self->{load} = 0;
  $self->{filter_on} = 0;
  $self->{filter_level} = 0;
  $self->{lstack} = [];
  $self->{lvar} = undef;
  $self->{lkeys} = [];
  $self->{lsections} = undef;
  $self->{userdata} = $userdata;
  $self->{buffer} = '';
  $self->{eof} = 0;
  $self->{name} = '';
  $self->{stack} = [];
  $self->{state} = $STATE_FIND_LANDMARK;
  $self->set_handlers(%handlers);
}

sub _parse {
  my $self = shift;
  return if ($self->{state} == $STATE_ERROR);
  my ($text, $eof) = @_;
  $self->{buffer} .= $text;
  $self->{eof} = $eof;
  my $prev_state = 0;
  while ($prev_state != $self->{state}) {
    $prev_state = $self->{state};
    if ($self->{state} == $STATE_FIND_LANDMARK) {
      $self->_find_landmark();
    } elsif ($self->{state} == $STATE_READ_VAR) {
      $self->_read_var();
    } elsif ($self->{state} == $STATE_FIND_START) {
      $self->_find_start();
    } elsif ($self->{state} == $STATE_PROPERTY_OR_ENDOBJ) {
      $self->_key_or_endobj();
    } elsif ($self->{state} == $STATE_COLON) {
      $self->_colon();
    } elsif ($self->{state} == $STATE_PROPERTY_VALUE) {
      $self->_key_value();
    } elsif ($self->{state} == $STATE_COMMA_OR_ENDOBJ) {
      $self->_comma_or_endobj();
    } elsif ($self->{state} == $STATE_PROPERTY) {
      $self->_key();
    } elsif ($self->{state} == $STATE_VALUE_OR_ENDLST) {
      $self->_value_or_endlst();
    } elsif ($self->{state} == $STATE_COMMA_OR_ENDLST) {
      $self->_comma_or_endlst();
    } elsif ($self->{state} == $STATE_LIST_VALUE) {
      $self->_list_value();
    } else {
      die("Unknown state!");
    }
    last if ($self->{state} == $STATE_ERROR);
  }
}

sub _find_landmark {
  my $self = shift;
  my $pos = index($self->{buffer}, $LANDMARK);
  if ($pos == -1) {
    # did not find landmark, keep the end of the string
    $self->{buffer} = substr($self->{buffer}, -(length($LANDMARK) - 1));
  } else {
    # found landmark so skip over it
    $self->{buffer} = substr($self->{buffer}, $pos + length($LANDMARK));
    # change state to reading the variable name
    $self->{state} = $STATE_READ_VAR;
  }
}

sub _read_var {
  my $self = shift;
  if ($self->{buffer} =~ m/^\s*(\S+)\s/) {
    $self->{var_name} = $1;
    $self->{buffer} = substr($self->{buffer}, $+[1]+1);
    $self->{state} = $STATE_FIND_START;
  }
}

sub _find_start {
  my $self = shift;
  my $pos = index($self->{buffer}, '{');
  if ($pos == -1) {
    $self->{buffer} = '';
  } else {
    $self->{buffer} = substr($self->{buffer}, $pos + 1);
    push(@{$self->{stack}}, $STATE_FIND_LANDMARK);
    $self->{state} = $STATE_PROPERTY_OR_ENDOBJ;
    $self->_start_data();
    $self->_start_object();
  }
}

sub _next_token_debug {
  my $self = shift;
  my $token = $self->_next_token2(@_);
  if ($token) {
    if ($token->{type} == $TOKEN_ILLEGAL) {
      print "Illegal data: " . $self->{buffer} . "\n";
      print($token->{reason}) if ($token->{reason});
    } else {
      print "Token " . $TOKEN_NAMES[$token->{type}] . "\n";
    }
  } else {
    print "Need more: " . $self->{buffer} . "\n";
  }
  return $token;
}

sub _next_token {
  my $self = shift;
  # Tokens:
  # {         STARTOBJ
  # }         ENDOBJ
  # [         STARTLST
  # ]         ENDLST
  # :         COLON
  # ,         COMMA
  # true      TRUE
  # false     FALSE
  # null      NULL
  # <string>  STRING
  # <number>  NUMBER
  #
  # It is assumed that non-symbolic tokens will not be terminated by EOF
  # as the last token in a file will be a ENDOBJ and even then in most
  # use cases it should be followed by more non-JSON data.
  #

  # skip whitespace
  $self->{buffer} =~ s/^\s+//;
  return undef if (length($self->{buffer}) == 0);
  my $letter = substr($self->{buffer}, 0, 1);
  my $type;
  if ($letter eq '{') {
    $type = $TOKEN_STARTOBJ;
  } elsif ($letter eq '}') {
    $type = $TOKEN_ENDOBJ;
  } elsif ($letter eq '[') {
    $type = $TOKEN_STARTLST;
  } elsif ($letter eq ']') {
    $type = $TOKEN_ENDLST;
  } elsif ($letter eq ':') {
    $type = $TOKEN_COLON;
  } elsif ($letter eq ',') {
    $type = $TOKEN_COMMA;
  }
  if ($type) {
    $self->{buffer} = substr($self->{buffer}, 1);
    return {type => $type};
  }
  if ($letter eq 'n') {
    return $self->_word_token('null', $TOKEN_NULL);
  } elsif ($letter eq 't') {
    return $self->_word_token('true', $TOKEN_BOOLEAN, 1);
  } elsif ($letter eq 'f') {
    return $self->_word_token('false', $TOKEN_BOOLEAN, 0);
  } elsif ($letter =~ m/^[0-9-]$/) {
    return $self->_num_token();
  } elsif ($letter eq '"') {
    return $self->_str_token();
  }
  return {type => $TOKEN_ILLEGAL, 
    reason => "letter does not match an allowed token start: $letter\n"};
}

sub _word_token {
  my $self = shift;
  my ($w, $type, $value) = @_;
  my $b = $self->{buffer};
  # when the buffer is too short for a full word check the first part
  my $c = (length($w) > length($b) ? length($b) : length($w));
  if (substr($b, 0, $c) ne substr($w, 0, $c)) {
    return {type => $TOKEN_ILLEGAL, 
      reason => "Substring ".substr($b, 0, $c)." does not match expected ".
          substr($w, 0, $c)."\n"};
  }
  # do we need more information to be definitive?
  return undef if (length($b) <= length($w));
  # word must be followed by either a comma, whitespace, end of list or end of object
  if (substr($b, length($w), 1) !~ m/^[,\s\]\}]$/) {
    return {type => $TOKEN_ILLEGAL, 
      reason => "Word $w must be followed by an approprate end, got: '".
          substr($b, length($w), 1)."'\n"};
  }
  # found word, so remove from buffer
  $self->{buffer} = substr($b, length($w));
  # return the specified type and value
  return {type => $type, value => $value} if (defined($value));
  return {type => $type};
}

sub _num_token {
  my $self = shift;
  my $b = $self->{buffer};
  my $offset = 0;
  return undef if (length($b) <= $offset);
  if (substr($b, $offset, 1) eq '-') {
    $offset++;
    return undef if (length($b) <= $offset);
  }
  if (substr($b, $offset, 1) eq '0') {
    $offset++;
    return undef if (length($b) <= $offset);
  } elsif (substr($b, $offset, 1) =~ m/^[1-9]$/) {
    $offset++;
    return undef if (length($b) <= $offset);
    while (substr($b, $offset, 1) =~ m/^\d$/) {
      $offset++;
      return undef if (length($b) <= $offset);
    }
  } else {
    #not a legal number token!
    return {type => $TOKEN_ILLEGAL};
  }
  if (substr($b, $offset, 1) eq '.') {
    $offset++;
    return undef if (length($b) <= $offset);
    if (substr($b, $offset, 1) =~ m/^\d$/) {
      $offset++;
      return undef if (length($b) <= $offset);
    } else {
      #not a legal number token!
      return {type => $TOKEN_ILLEGAL};
    }
    while(substr($b, $offset, 1) =~ m/^\d$/) {
      $offset++;
      return undef if (length($b) <= $offset);
    }
  }
  if (substr($b, $offset, 1) =~ m/^[eE]$/) {
    $offset++;
    return undef if (length($b) <= $offset);
    if (substr($b, $offset, 1) =~ m/^[+-]$/) {
      $offset++;
      return undef if (length($b) <= $offset);
    }
    if (substr($b, $offset, 1) =~ m/^\d$/) {
      $offset++;
      return undef if (length($b) <= $offset);
    } else {
      #not a legal number token!
      return {type => $TOKEN_ILLEGAL};
    }
    while(substr($b, $offset, 1) =~ m/^\d$/) {
      $offset++;
      return undef if (length($b) <= $offset);
    }
  }
  # check that the following character is acceptable
  # must be either comma, whitespace, end of object or end of list.
  unless (substr($b, $offset, 1) =~ m/^[,\s\}\]]$/) {
    return {type => $TOKEN_ILLEGAL};
  }
  my $num = substr($b, 0, $offset);
  $self->{buffer} = substr($b, $offset);
  return {type => $TOKEN_NUMBER, value => $num};
}

sub _str_token {
  my $self = shift;
  my $b = $self->{buffer};
  return undef if (length($b) == 0);
  # check that we start with a double quote as expected
  return {type => $TOKEN_ILLEGAL} unless (substr($b, 0, 1) eq '"');
  # now loop over every character and check: 
  # 1) not a control character (in range U+0000..U+001F and U+007F..U+009F)
  # 2) for \ and set flag to treat next char specially
  # 3) for " to enable the end of the string
  my $i;
  my $escaped = 0;
  my $hexcode = 0;
  my $hexval = '';
  my $start = 1;
  my $text = '';
  for ($i = 1; $i < length($b); $i++) {
    my $letter = substr($b, $i, 1);
    my $num = ord($letter);
    if ($num >= 0 && $num <= 0x1F || $num >= 0x7F && $num <= 0x9F) {
      #Illegal control character!
      return {type => $TOKEN_ILLEGAL};
    }
    if ($escaped) {
      if ($letter eq '"') {
        $text .= '"';
      } elsif ($letter eq "\\") {
        $text .= "\\";
      } elsif ($letter eq '/') {
        $text .= '/';
      } elsif ($letter eq 'b') {
        $text .= "\b";
      } elsif ($letter eq 'f') {
        $text .= "\f";
      } elsif ($letter eq 'n') {
        $text .= "\n";
      } elsif ($letter eq 'r') {
        $text .= "\r";
      } elsif ($letter eq 't') {
        $text .= "\t";
      } elsif ($letter eq 'u') {
        $hexcode = 1;
        $hexval = '';
      } else {
        # disallowed escaped character
        return {type => $TOKEN_ILLEGAL};
      }
      $escaped = 0;
      $start = $i + 1;
    } elsif ($hexcode) {
      if ($letter =~ m/^[0-9a-fA-F]$/) {
        $hexval .= $letter;
        if ($hexcode == 4) {
          $text .= chr(hex($hexval));
          $hexcode = 0;
          $start = $i + 1;
        } else {
          $hexcode++;
        }
      } else {
        # not a hexadecimal digit
        return {type => $TOKEN_ILLEGAL};
      }
    } else {
      if ($letter eq '"') {
        $text .= substr($b, $start, $i - $start) if ($i > $start);
        $i++; # skip over the "
        last;
      } elsif ($letter eq "\\") {
        $text .= substr($b, $start, $i - $start) if ($i > $start);
        $escaped = 1;
      }
    }
  }
  # ask for more unless we found the end
  return undef unless ($i < length($b));
  # check if the next character is acceptable
  return {type => $TOKEN_ILLEGAL} unless (substr($b, $i, 1) =~ m/^[:,\s\}\]]$/);
  # remove the string from the buffer
  $self->{buffer} = substr($b, $i);
  # return the string
  return {type => $TOKEN_STRING, value => $text};
}

sub _key_or_endobj {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_STRING) {
    # found property key
    $self->_property($token->{value});
    $self->{state} = $STATE_COLON;
  } elsif ($token->{type} == $TOKEN_ENDOBJ) {
    $self->_end_object();
    $self->{state} = pop(@{$self->{stack}});
    $self->_end_data() if ($self->{state} == $STATE_FIND_LANDMARK);
  } else {
    $self->{state} = $STATE_ERROR;
    die("Expected key or endobj\n");
  }
}
sub _colon {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_COLON) {
    $self->{state} = $STATE_PROPERTY_VALUE;
  } else {
    $self->{state} = $STATE_ERROR;
    die("Expected colon\n");
  }
}
sub _key_value {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_STARTOBJ) {
    push(@{$self->{stack}}, $STATE_COMMA_OR_ENDOBJ);
    $self->{state} = $STATE_PROPERTY_OR_ENDOBJ;
    $self->_start_object();
  } elsif ($token->{type} == $TOKEN_STARTLST) {
    push(@{$self->{stack}}, $STATE_COMMA_OR_ENDOBJ);
    $self->{state} = $STATE_VALUE_OR_ENDLST;
    $self->_start_array();
  } else {
    if ($token->{type} == $TOKEN_NULL) {
      $self->_null_atom();
    } elsif ($token->{type} == $TOKEN_BOOLEAN) {
      $self->_bool_atom($token->{value});
    } elsif ($token->{type} == $TOKEN_NUMBER) {
      $self->_number_atom($token->{value});
    } elsif ($token->{type} == $TOKEN_STRING) {
      $self->_string_atom($token->{value});
    } else {
      $self->{state} = $STATE_ERROR;
      die("Expected value but got token " . $TOKEN_NAMES[$token->{type}] . "\n");
      return;
    }
    $self->{state} = $STATE_COMMA_OR_ENDOBJ;
  }
}
sub _comma_or_endobj {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_COMMA) {
    $self->{state} = $STATE_PROPERTY;
  } elsif ($token->{type} == $TOKEN_ENDOBJ) {
    $self->_end_object();
    $self->{state} = pop(@{$self->{stack}});
    $self->_end_data() if ($self->{state} == $STATE_FIND_LANDMARK);
  } else {
    $self->{state} = $STATE_ERROR;
    die("Expected comma or endobj\n");
  }
}
sub _key {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_STRING) {
    # found property key
    $self->_property($token->{value});
    $self->{state} = $STATE_COLON;
  } else {
    $self->{state} = $STATE_ERROR;
    die("Expected Property\n");
  }
}
sub _value_or_endlst {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_ENDLST) {
    $self->_end_array();
    $self->{state} = pop(@{$self->{stack}});
  } elsif ($token->{type} == $TOKEN_STARTOBJ) {
    push(@{$self->{stack}}, $STATE_COMMA_OR_ENDLST);
    $self->{state} = $STATE_PROPERTY_OR_ENDOBJ;
    $self->_start_object();
  } elsif ($token->{type} == $TOKEN_STARTLST) {
    push(@{$self->{stack}}, $STATE_COMMA_OR_ENDLST);
    $self->{state} = $STATE_VALUE_OR_ENDLST;
    $self->_start_array();
  } else {
    if ($token->{type} == $TOKEN_NULL) {
      $self->_null_atom();
    } elsif ($token->{type} == $TOKEN_BOOLEAN) {
      $self->_bool_atom($token->{value});
    } elsif ($token->{type} == $TOKEN_NUMBER) {
      $self->_number_atom($token->{value});
    } elsif ($token->{type} == $TOKEN_STRING) {
      $self->_string_atom($token->{value});
    } else {
      $self->{state} = $STATE_ERROR;
      die("Expected value or Endlst\n");
      return;
    }
    $self->{state} = $STATE_COMMA_OR_ENDLST;
  }
}
sub _comma_or_endlst {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_COMMA) {
    $self->{state} = $STATE_LIST_VALUE;
  } elsif ($token->{type} == $TOKEN_ENDLST) {
    $self->_end_array();
    $self->{state} = pop(@{$self->{stack}});
  } else {
    $self->{state} = $STATE_ERROR;
    die("Expected comma or Endlst\n");
  }
}
sub _list_value {
  my $self = shift;
  my $token = $self->_next_token();
  return unless $token;
  if ($token->{type} == $TOKEN_STARTOBJ) {
    push(@{$self->{stack}}, $STATE_COMMA_OR_ENDLST);
    $self->{state} = $STATE_PROPERTY_OR_ENDOBJ;
    $self->_start_object();
  } elsif ($token->{type} == $TOKEN_STARTLST) {
    push(@{$self->{stack}}, $STATE_COMMA_OR_ENDLST);
    $self->{state} = $STATE_VALUE_OR_ENDLST;
    $self->_start_array();
  } else {
    if ($token->{type} == $TOKEN_NULL) {
      $self->_null_atom();
    } elsif ($token->{type} == $TOKEN_BOOLEAN) {
      $self->_bool_atom($token->{value});
    } elsif ($token->{type} == $TOKEN_NUMBER) {
      $self->_number_atom($token->{value});
    } elsif ($token->{type} == $TOKEN_STRING) {
      $self->_string_atom($token->{value});
    } else {
      $self->{state} = $STATE_ERROR;
      die("Expected List value");
      return;
    }
    $self->{state} = $STATE_COMMA_OR_ENDLST;
  }
}

sub _start_data {
  my $self = shift;
  if (defined($self->{start_data})) {
    $self->{start_data}($self->{userdata}, $self->{var_name});
  }
  if ($self->{load}) {
    $self->{lvar} = $self->{var_name};
    $self->{lstack} = [$self->{lsections}];
  }
}

sub _end_data {
  my $self = shift;
  if (defined($self->{end_data})) {
    $self->{end_data}($self->{userdata});
  }
  if ($self->{load}) {
    pop(@{$self->{lstack}});
  }
}

sub _property {
  my $self = shift;
  my ($name) = @_;
  if (defined($self->{property})) {
    $self->{property}($self->{userdata}, $name);
  }
  if ($self->{load}) {
    unless ($self->{filter_on}) {
      my $id = join(@{$self->{lkeys}}, ">") . ">" . $name;
      my $filter;
      foreach $filter (@{$self->{filters}}) {
        if ($id =~ /$filter/) {
          $self->{filter_on} = 1; #TRUE
          return;
        }
      }
      $self->{lvar} = $name;
    }
  }
}

sub _atom {
  my $self = shift;
  my ($value, $is_null, $is_bool, $is_string, $is_num, $is_int, $is_sci) = @_;
  if (defined($self->{atom})) {
    $self->{atom}($self->{userdata}, $value, $is_null, $is_bool, $is_string, 
      $is_num, $is_int, $is_sci);
  }
  if ($self->{load}) {
    unless ($self->{filter_on}) {
      if (ref($self->{lstack}->[-1]) eq "ARRAY") {
        push(@{$self->{lstack}->[-1]}, $value);
      } else {
        $self->{lstack}->[-1]->{$self->{lvar}} = $value;
      }
    } elsif ($self->{filter_level} == 0) {
      $self->{filter_on} = 0; # FALSE
    }
  }
}

sub _null_atom {
  my $self = shift;
  $self->_atom(undef, 1, 0, 0, 0, 0, 0);
}

sub _bool_atom {
  my $self = shift;
  my ($bool) = @_;
  $self->_atom($bool, 0, 1, 0, 0, 0, 0);
}

sub _string_atom {
  my $self = shift;
  my ($string) = @_;
  $self->_atom($string, 0, 0, 1, 0, 0, 0);
}

sub _number_atom {
  my $self = shift;
  my ($num) = @_;
  my $is_int = ($num =~ m/\./);
  my $is_sci = ($num =~ m/[e|E]/);
  $self->_atom($num, 0, 0, 0, 1, $is_int, $is_sci);
}

sub _start_object {
  my $self = shift;
  if (defined($self->{start_object})) {
    $self->{start_object}($self->{userdata});
  }
  if ($self->{load}) {
    unless ($self->{filter_on}) {
      if (ref($self->{lstack}->[-1]) eq "ARRAY") {
        # loading stack contains an array as the current entry
        # get that array and push an object onto the end of it
        push(@{$self->{lstack}->[-1]}, {});
        # push the new object (created above) on to the loading stack
        push(@{$self->{lstack}}, $self->{lstack}->[-1]->[-1]);
      } else {
        my $key = $self->{lvar};
        push(@{$self->{lkeys}}, $key);
        # loading stack contains a object as the current entry
        # get the object and add a new property-object value
        $self->{lstack}->[-1]->{$key} = {};
        # push the new object (created above) on to the loading stack
        push(@{$self->{lstack}}, $self->{lstack}->[-1]->{$key});
      }
    } else {
      $self->{filter_level} += 1;
    }
  }
}

sub _end_object {
  my $self = shift;
  if (defined($self->{end_object})) {
    $self->{end_object}($self->{userdata});
  }
  if ($self->{load}) {
    unless ($self->{filter_on}) {
      pop(@{$self->{lstack}});
      if (ref($self->{lstack}->[-1]) ne "ARRAY") {
        pop(@{$self->{lkeys}});
      }
    } else {
      $self->{filter_level} -= 1;
      if ($self->{filter_level} == 0) {
        $self->{filter_on} = 0; #FALSE
      }
    }
  }
}

sub _start_array {
  my $self = shift;
  if (defined($self->{start_array})) {
    $self->{start_array}($self->{userdata});
  }
  if ($self->{load}) {
    unless ($self->{filter_on}) {
      if (ref($self->{lstack}->[-1]) eq "ARRAY") {
        push(@{$self->{lstack}->[-1]}, []);
        push(@{$self->{lstack}}, $self->{lstack}->[-1]->[-1]);
      } else {
        my $key = $self->{lvar};
        push(@{$self->{lkeys}}, $key);
        $self->{lstack}->[-1]->{$key} = [];
        push(@{$self->{lstack}}, $self->{lstack}->[-1]->{$key});
      }
    } else {
      $self->{filter_level} += 1;
    }
  }
}

sub _end_array {
  my $self = shift;
  if (defined($self->{end_array})) {
    $self->{end_array}($self->{userdata});
  }
  if ($self->{load}) {
    unless ($self->{filter_on}) {
      pop(@{$self->{lstack}});
      if (ref($self->{lstack}->[-1]) ne "ARRAY") {
        pop(@{$self->{lkeys}});
      }
    } else {
      $self->{filter_level} -= 1;
      if ($self->{filter_level} == 0) {
        $self->{filter_on} = 0; #FALSE
      }
    }
  }
}
