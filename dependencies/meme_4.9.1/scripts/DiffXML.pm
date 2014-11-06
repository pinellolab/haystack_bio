package DiffXML;

use warnings;
use strict;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(diff_xml);

use Fcntl qw(O_RDONLY);
use XML::Parser::Expat;

my $TYPE_CHAR = 1;
my $TYPE_START = 2;
my $TYPE_END = 3;
my $TYPE_ATTR = 4;


#
# Take 2 xml files (xml1, xml2) and a patterns to ignore.
#
# If xml1 or xml2 is invalid xml it should return a sentence describing
# the first error it finds including where in the file it is.
#
# If xml1 is identical to xml2 it should return an empty string.
#
# If xml1 is different to xml2 is should return a sentence describing the 
# first difference it finds including where in the file it is.
#
# It should allow attributes to be reordered but their values should be
# the same including space. If an attribute is missing or added then that
# counts as a difference. Elements should be ordered identically. Text
# within elements will have leading and trailing space ignored but non-
# whitespace must match identically.
#
# Every part of the xml file is given a name and users can pass regular
# expressions which are matched against these names to determine if an
# element and all its sub elements should be ignored.
#
# In the naming system elements and attributes are separated by colons ':'.
# Attribute names are begun with a at symbol '@' and contained text is 
# named '#value'.
#
# So for example:
# dreme:runtime:@cpu
#
# Is the name of the cpu attribute of the runtime element.
#
# Also for example:
# dreme:model:seed:#value
#
# Is the name of the text contained within the seed element.
#
#


sub diff_xml {
  my ($xmlfilename1, $xmlfilename2, @patterns) = @_;
  my ($fh1, $fh2);

  # make regular expressions from the patterns to ignore
  my @ignore_regex = ();
  for my $pattern (@patterns) {
    push(@ignore_regex, qr/$pattern/);
  }

  sysopen($fh1, $xmlfilename1, O_RDONLY) or die("Failed to open $xmlfilename1");
  sysopen($fh2, $xmlfilename2, O_RDONLY) or die("Failed to open $xmlfilename2");

  my ($s1, $p1) = &make_parser(@ignore_regex);
  my ($s2, $p2) = &make_parser(@ignore_regex);

  my $message = '';
  while (!$message && &has_more($s1) && &has_more($s2)) {
    $message = &load_more_events($s1, $p1, $fh1, $s2, $p2, $fh2);
    last if $message;
    my ($e1, $e2);
    while (&has_event($s1) && &has_event($s2)) {
      $e1 = &take_event($s1);
      $e2 = &take_event($s2);
      if ($e1->{type} != $e2->{type} || $e1->{name} ne $e2->{name}) {
        $message = "A difference was detected between the reference " . 
            "and tested xml files.\n" .
            "The reference xml at line " . $e1->{line} . ", column " . 
            ($e1->{column} + 1) . " has a " . &type_str($e1->{type}) . 
            " with the name " . $e1->{name} . ".\n" .
            "But the tested xml at line " . $e2->{line} . ", column ". 
            ($e2->{column} + 1) . " has a " . &type_str($e2->{type}) . 
            " with the name " . $e2->{name} . ".\n";
        last;
      }
      # type and name match so check data if applicable
      if ($e1->{type} == $TYPE_ATTR) {
        if ($e1->{data} ne $e2->{data}) {
          $message = "A difference was detected between the reference " . 
              "and tested xml files.\n" .
              "The element attribute named " . $e1->{name} . 
              " is not consistant.\n" .
              "The reference xml at line " . $e1->{line} . ", column " . 
              ($e1->{column} + 1) . " has the value \"" . $e1->{data} . "\".\n".
              "But the tested xml at line " . $e2->{line} . ", column ". 
              ($e2->{column} + 1) . " has the value \"" . $e2->{data} . "\".\n";
          last;
        }
      } elsif ($e1->{type} == $TYPE_CHAR) {
        # this gets messy as we now need to compare across lists of events
        # that may not be equal in length
        &replace_event($s1, $e1);
        &replace_event($s2, $e2);
        # do the work in another method to make things cleaner
        $message = &diff_xml_chars($s1, $p1, $fh1, $s2, $p2, $fh2);
      }
    }
  }

  if (!$message && (&has_more($s1) || &has_more($s2))) {
    # I don't think this state can occur because the end tag
    # would not match
    $message = "Unmatched events\n";
  }

  close($fh1);
  close($fh2);

  return $message;
}

sub diff_xml_chars {
  my ($s1, $p1, $fh1, $s2, $p2, $fh2) = @_;
  my $message = '';
  my ($line1, $col1, $line2, $col2);
  my $trailing_space_match = 1;
  while (!$message && (&has_chars($s1) || &has_chars($s2))) {
    $message = &load_more_events($s1, $p1, $fh1, $s2, $p2, $fh2);
    last if $message;
    if (&has_chars($s1) && &has_chars($s2)) {
      my $e1 = &take_event($s1);
      my $e2 = &take_event($s2);
      if ($trailing_space_match) {
        $line1 = $e1->{line};
        $col1 = $e1->{column};
        $line2 = $e2->{line};
        $col2 = $e2->{column};
      }
      my $len1 = length($e1->{data});
      my $len2 = length($e2->{data});
      my $min = ($len1 < $len2 ? $len1 : $len2);
      for (my $i = 0; $i < $min; $i++) {
        my $c1 = substr($e1->{data}, $i, 1);
        my $c2 = substr($e2->{data}, $i, 1);
        # If either of the characters is not a space then they must match
        # and there must not have been any mismatches in preceeding whitespace
        if (!(&is_spaces($c1) && &is_spaces($c2)) && ($c1 ne $c2 || !$trailing_space_match)) {
          $message = "A difference was detected between the reference " . 
              "and tested xml files.\n" .
              "The element text content named " . $e1->{name} . 
              " is not consistant.\n" .
              "The reference xml at line " . $line1 . ", column " . 
              ($col1 + 1) . " is not consistant with " .
              "the tested xml at line " . $line2 . ", column ". 
              ($col2 + 1) . ".\n";
          last;
        } elsif ($c1 eq $c2 && $trailing_space_match) {
          # update last match location
          if (&is_nl($c1)) {
            $col1 = 0;
            $col2 = 0;
            $line1++;
            $line2++;
          } else {
            $col1++;
            $col2++;
          }
        } else {
          $trailing_space_match = 0;
        }
      }
      # push back left over characters as events
      if ($len1 < $len2) {
        my $event = &make_event($TYPE_CHAR, $e2->{name}, 
          substr($e2->{data}, $min), $line2, $col2);
        &replace_event($s2, $event);
      } elsif ($len2 < $len1) {
        my $event = &make_event($TYPE_CHAR, $e1->{name}, 
          substr($e1->{data}, $min), $line1, $col1);
        &replace_event($s1, $event);
      }
    } else {
      my $event;
      # at most only one of these will have an event
      $event = &take_event($s1) if (&has_chars($s1));
      $event = &take_event($s2) if (&has_chars($s2));
      if ($event && !&is_spaces($event->{data})) {
        $message = "A difference was detected between the reference " . 
            "and tested xml files.\n" .
            "The element text content named " . $event->{name} . 
            " is not consistant.\n" .
            "The reference xml at line " . $line1 . ", column " . 
            ($col1 + 1) . " is not consistant with " .
            "the tested xml at line " . $line2 . ", column ". 
            ($col2 + 1) . ".\n";
        last;
      }
    }
  }
  return $message;
}

sub type_str {
  my ($val) = @_;
  if ($val == $TYPE_CHAR) {
    return "element text content";
  } elsif ($val == $TYPE_START) {
    return "element start";
  } elsif ($val == $TYPE_END) {
    return "element end";
  } elsif ($val == $TYPE_ATTR) {
    return "element attribute";
  }
}

sub is_spaces {
  my ($c) = @_;
  return $c =~ m/^[ \t\n]*$/;
}

sub is_blank {
  my ($c) = @_;
  return $c =~ m/^[ \t]$/;
}

sub is_nl {
  my ($c) = @_;
  return $c =~ m/^\n$/;
}

sub make_event {
  my ($type, $name, $data, $line, $column) = @_;
  return {
      type => $type, 
      name => $name, 
      data => $data, 
      line => $line,
      column => $column
    };
}

sub queue_event {
  my ($state, $type, $name, $data, $line, $column) = @_;
  push(@{$state->{events}},
    &make_event($type, $name, $data, $line, $column)
  );
}

sub take_event {
  my ($state) = @_;
  my $event = shift(@{$state->{events}});
  return $event;
}

sub replace_event {
  my ($state, $event) = @_;
  unshift(@{$state->{events}}, $event);
}

sub has_event {
  my ($state) = @_;
  return scalar(@{$state->{events}}) > 0;
}

sub has_more {
  my ($state) = @_;
  return !$state->{finished} || scalar(@{$state->{events}}) > 0
}

sub has_chars {
  my ($state) = @_;
  if (scalar(@{$state->{events}}) > 0) {
    my $event = @{$state->{events}}[0];
    return $event->{type} == $TYPE_CHAR;
  } else {
    return !$state->{finished};
  }
}

sub should_ignore {
  my ($name, @ignore_re_list) = @_;
  for my $ignore_re (@ignore_re_list) {
    return 1 if ($name =~ m/$ignore_re/);
  }
  return 0;
}

sub full_name {
  my @elements = @_;
  return '' unless @elements;
  my $full_name = $elements[0]->{name};
  for (my $i = 1; $i < scalar(@elements); $i++) {
    my $element = $elements[$i];
    $full_name .= ':' . $element->{name};
  }
  return $full_name;
}

sub load_more_events {
  my ($s1, $p1, $fh1, $s2, $p2, $fh2) = @_;
  &parse_file_part($s1, $p1, $fh1);
  return &xml_parse_error($s1, "reference") if $s1->{error};
  &parse_file_part($s2, $p2, $fh2);
  return &xml_parse_error($s2, "reference") if $s2->{error};
  return '';
}

sub xml_parse_error {
  my ($state, $file_desc) = @_;
  my $message = "The " . $file_desc . " xml was malformed ".
      "so parsing it gave the error \"" . $state->{error} . 
      "\" at the line " . $state->{line} . ", column " . $state->{column} . 
      ".\n";
  return $message;
}

sub parse_file_part {
  my ($state, $parser, $fh) = @_;
  my $buffer;

  return if ($state->{finished});

  eval {
    # read until the state contains some data to compare
    while (scalar(@{$state->{events}}) == 0 && !$state->{error}) {
      my $count = read($fh, $buffer, 100);
      die("Error reading file") unless defined($count);
      if ($count == 0) {
        $state->{finished} = 1;
        $parser->parse_done();
        last;
      }
      $parser->parse_more($buffer);
    }
  };
  if ($@) {
    my $message = $@;
    if ($message =~ m/(.*) at line (\d+), column (\d+), byte \d+ at /) {
      $state->{error} = $1;
      $state->{line} = $2;
      $state->{column} = $3;
    } else {
      die($@);
    }
  }
}

sub make_parser {
  my $state = {
    ignore => \@_,
    finished => 0,
    text => 0,
    error => "",
    line => 1,
    column => 0,
    elements => [], # stack of {
                    #   name => ELEM_NAME, index => INDEX, 
                    #   position => INDEX_OF_SAME, children => COUNT,  
                    #   counts => {sub_element_name => COUNT}
                    #   ignore => [0|1]
                    # }
    events => []  # queue of {type => TYPE, name => FULL_NAME, data => TEXT_OR_UNDEF}
  };
  my $parser = new XML::Parser::ExpatNB; # non blocking parser
  $parser->setHandlers(Start => sub {handle_start($state, @_);});
  $parser->setHandlers(End => sub {handle_end($state, @_);});
  $parser->setHandlers(Char => sub {handle_char($state, @_);});

  return ($state, $parser);
}

sub handle_start {
  my ($state, $expat, $element, @attr_pairs) = @_;
  # don't bother if we've seen any errors
  return if $state->{error};

  if ($state->{text}) {# TODO add position information
    $state->{error} = "character data and elements intermixed";
    $state->{line} = $expat->current_line;
    $state->{column} = $expat->current_column;
    return;
  }

  # update elements
  my $index = 1;
  my $position = 1;
  my $ignore = 0;
  if (scalar(@{$state->{elements}}) > 0) {
    my $parent = @{$state->{elements}}[-1];
    $parent->{children} += 1;
    $index = $parent->{children};
    $position = $parent->{counts}->{$element};
    $position = 0 unless defined($position);
    $position += 1;
    $parent->{counts}->{$element} = $position;
    $ignore = $parent->{ignore};
  }
  my $elem = {
    name => $element, 
    index => $index, 
    position => $position, 
    children => 0, 
    counts => {},
    ignore => $ignore
  };
  push(@{$state->{elements}}, $elem);

  # before we push any events, check if we're ignoring some parent element
  return if $ignore;

  my $name = &full_name(@{$state->{elements}});

  $ignore = &should_ignore($name, @{$state->{ignore}});

  $elem->{ignore} = $ignore;

  return if $ignore;

  # push element start event
  &queue_event($state, $TYPE_START, $name, undef, $expat->current_line, 
    $expat->current_column);

  # now get the attributes, sort them and check each one to see if it should 
  # be ignored before adding an attribute event for it.
  
  my %attributes = @attr_pairs;

  my @attr_names = sort(keys(%attributes));
  
  for my $attr_name (@attr_names) {
    my $attr_full_name = $name . '@' . $attr_name;
    if (! &should_ignore($attr_full_name, @{$state->{ignore}})) {
      my $value = '';
      # check that we should send the attribute value as well
      # as we give the option of just ignoring the value but
      # not the existance of the attribute.
      if (! &should_ignore($attr_full_name . '#value', @{$state->{ignore}})) {
        $value = $attributes{$attr_name};
      }
      &queue_event($state, $TYPE_ATTR, $attr_full_name, $value,
        $expat->current_line, $expat->current_column);
    }
  }

}

sub handle_end {
  my ($state, $expat, $element) = @_;
  # don't bother if we've seen any errors
  return if $state->{error};

  $state->{text} = 0;

  my $current = pop(@{$state->{elements}});
  return if ($current->{ignore});

  # push event
  my $name = &full_name(@{$state->{elements}}, $current);
  &queue_event($state, $TYPE_END, $name, undef, $expat->current_line, 
    $expat->current_column);
}

sub handle_char {
  my ($state, $expat, $string) = @_;
  # don't bother if we've seen any errors
  return if $state->{error};

  my $line = $expat->current_line;
  my $column = $expat->current_column;

  if (!$state->{text}) {
    # note that this assumes that we're not using binary mode
    return if &is_spaces($string);
    for (my $i = 0; $i < length($string); $i++) {
      my $c = substr($string, $i, 1);
      if (&is_blank($c)) {
        $column++;
      } elsif (&is_nl($c)) {
        $line++;
        $column = 0;
      } else {
        $string = substr($string, $i);
        last;
      }
    }
  }
  $state->{text} = 1;

  my $current = @{$state->{elements}}[-1];

  # this isn't done first because we need to be
  # sure the text isn't just spaces, tabs and newlines.
  if ($current->{children} > 0) {
    $state->{error} = "character data and elements intermixed";
    $state->{line} = $line;
    $state->{column} = $column;
    return;
  }

  return if ($current->{ignore});

  my $name = &full_name(@{$state->{elements}}) . '#value';

  return if (&should_ignore($name, @{$state->{ignore}}));

  # push new event
  &queue_event($state, $TYPE_CHAR, $name, $string, $line, $column);
}


