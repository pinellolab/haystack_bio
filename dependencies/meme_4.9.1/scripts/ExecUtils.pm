# File: ExecUtils.pm
# Project: Anything
# Description: Helper functions for executing external programs and scripts from perl.

package ExecUtils;

use strict;
use warnings;

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(invoke stringify_arg stringify_args stringify_args2);

use Fcntl qw(SEEK_SET);
use File::Spec::Functions qw(catfile tmpdir);
use File::Temp qw(tempfile);
use Time::HiRes qw(gettimeofday tv_interval);

#
# Run a command and optionally save stderr/stdout
# to a variable/file and optionally read stdin
# from a variable/file
#
# Makes use of temporary files to do the reading/writing
# from/to variables.
#
# PROG => program name
# BIN => program directory
# ARGS => reference to array of program arguments
# IN_FILE => file name or handle to set as stdin
# IN_VAR => variable (or reference to variable) to feed in as stdin
# IN_NAME => the displayed name for the source of stdin
# ALL_FILE => file name or handle to store stdout and stderr
# ALL_VAR => reference to variable to store stdout and stderr
# ALL_NAME => the displayed name for the destination of output
# OUT_FILE => file name or handle to store stdout
# OUT_VAR => reference to variable to store stdout
# OUT_NAME => the displayed name for the destination of stdout
# ERR_FILE => file name or handle to store stderr
# ERR_VAR => reference to variable to store stderr
# ERR_NAME => the displayed name for the destination of stderr
# TRUNCATE => true to truncate output files if they exist (default true!)
# TMPDIR => directory to create temporary files
# NICE => the level of nicing to use (niceing disabled by default)
# TIMEOUT => time (in seconds) to run child process before timing out
# TIME => reference to store the running time in seconds (floating point)
# CMD => reference to store a human readable form of the command run
# OOT => reference to store if the command ran out of time (hit timeout)
# CHECK_STATUS => true to die on bad status codes
# 
sub invoke {
  my %opts = @_;
  my $logger = $opts{LOGGER};
  $logger->trace("call invoke") if $logger;
  # check that the IO redirection options make sense
  die("Both IN_FILE and IN_VAR specified!") if (defined($opts{IN_FILE}) && defined($opts{IN_VAR}));
  die("Both ALL_FILE and ALL_VAR specified!") if (defined($opts{ALL_FILE}) && defined($opts{ALL_VAR}));
  if (defined($opts{ALL_FILE}) || defined($opts{ALL_VAR})) {
    die("OUT_FILE incompatible with ALL_xxx") if (defined($opts{OUT_FILE}));
    die("OUT_VAR incompatible with ALL_xxx") if (defined($opts{OUT_VAR}));
    die("ERR_FILE incompatible with ALL_xxx") if (defined($opts{ERR_FILE}));
    die("ERR_VAR incompatible with ALL_xxx") if (defined($opts{ERR_VAR}));
  }
  die("Both OUT_FILE and OUT_VAR specified!") if (defined($opts{OUT_FILE}) && defined($opts{OUT_VAR}));
  die("Both ERR_FILE and ERR_VAR specified!") if (defined($opts{ERR_FILE}) && defined($opts{ERR_VAR}));
  # nice level
  my @nice = ();
  if (defined($opts{NICE})) {
    if ($opts{NICE} !~ m/^[+-]?\d+$/ || 
      $opts{NICE} < -20 || $opts{NICE} > 19) {
      die("Nice level not in range -20 to 19");
    }
    push(@nice, 'nice', '-n', int($opts{NICE}));
  }
  # program
  die("No program passed to invoke") unless defined($opts{PROG});
  my $exe = (defined($opts{BIN}) ? &catfile($opts{BIN}, $opts{PROG}) : $opts{PROG});
  # program arguments
  my @args = (defined($opts{ARGS}) ? @{$opts{ARGS}} : ());
  # timeout
  my $timeout = (defined($opts{TIMEOUT}) ? $opts{TIMEOUT} : 0);
  # pick temporary directory
  my $temp_dir = ($opts{TMPDIR} ? $opts{TMPDIR} : &tmpdir());
  # should files be truncated before writing?
  my $truncate = (defined($opts{TRUNCATE}) ? $opts{TRUNCATE} : 1);
  my $mode = ($truncate ? '>' : '>>');
  # make a temporary file to hold the variable for standard input
  my ($in_var_fh, $in_var_fn);
  if (defined($opts{IN_VAR})) {
    ($in_var_fh, $in_var_fn) = &tempfile('stdin_XXXXXXXXXX', DIR => $temp_dir, UNLINK => 1);
    print $in_var_fh (ref($opts{IN_VAR}) ? ${$opts{IN_VAR}} : $opts{IN_VAR});
    seek($in_var_fh, 0, SEEK_SET); # rewind file
  }
  # make a temporary file to hold the variable for standard output
  my ($out_var_fh, $out_var_fn);
  if (defined($opts{OUT_VAR})) {
    ($out_var_fh, $out_var_fn) = &tempfile('stdout_XXXXXXXXXX', DIR => $temp_dir, UNLINK => 1);
  }
  # make a temporary file to hold the variable for standard error
  my ($err_var_fh, $err_var_fn);
  if (defined($opts{ERR_VAR})) {
    ($err_var_fh, $err_var_fn) = &tempfile('stderr_XXXXXXXXXX', DIR => $temp_dir, UNLINK => 1);
  }
  # make a temporary file to hold the variable for standard out + err
  my ($all_var_fh, $all_var_fn);
  if (defined($opts{ALL_VAR})) {
    ($all_var_fh, $all_var_fn) = &tempfile('stdall_XXXXXXXXXX', DIR => $temp_dir, UNLINK => 1);
  }
  # record the start time
  my $t0 = [&gettimeofday()];
  # now split into two processes
  my $pid = fork();
  if ($pid == 0) { # child process
    # setup standard input redirect
    my $in_file = (defined($in_var_fh) ? $in_var_fh : $opts{IN_FILE});
    if (defined($in_file)) {
      open(STDIN, (ref($in_file) ? '<&' : '<'), $in_file) or die("Can't redirect STDIN: $!");
    }
    # the file to redirect all output to
    my $all_file = (defined($all_var_fh) ? $all_var_fh : $opts{ALL_FILE});
    if (defined($all_file) && $truncate) {
      # can not use the truncate mode (as file opened twice)
      $mode = '>>';
      # so use the truncate method instead.
      truncate($all_file, 0);
    }
    # setup standard output redirect
    my $out_file = (defined($all_file) ? $all_file : (defined($out_var_fh) ? $out_var_fh : $opts{OUT_FILE}));
    if (defined($out_file)) {
      open(STDOUT, (ref($out_file) ? $mode.'&' : $mode), $out_file) or die("Can't redirect STDOUT: $!");
    }
    # setup standard error redirect
    my $err_file = (defined($all_file) ? $all_file : (defined($err_var_fh) ? $err_var_fh : $opts{ERR_FILE}));
    if (defined($err_file)) {
      open(STDERR, (ref($err_file) ? $mode.'&' : $mode), $err_file) or die("Can't redirect STDERR: $!");
    }
    # disable buffering if redirecting both to the same file so output order will remain the same
    if (defined($all_file)) {
      my $old_fh = select(STDOUT); $| = 1; select(STDERR); $| = 1;
      select($old_fh);
    }
    # run the program, this call shouldn't return unless it fails
    exec(@nice, $exe, @args);
    die("Exec failed");
  }
  # parent process continues
  # now try to wait for the child process
  my $status = -1;
  my $oot = 0;#FALSE
  if (defined($pid)) { # fork worked
    # setup alarm handler
    local $SIG{ALRM} = sub {
      die("Timeout!\n");
    };
    eval {
      alarm($timeout); # set alarm to stop us if the process takes too long
      waitpid($pid, 0); # wait for the child process to exit
      alarm(0); # clear the alarm
    };
    if ($@) {
      if ($@ =~ /Timeout!/) {
        $oot = 1;#TRUE
        # tell child process to terminate
        my $SIGTERM = 15;
        kill($SIGTERM, $pid);
        # wait for it to quit
        waitpid($pid, 0);
      } else {
        die($@); # throw non-timeout related errors
      }
    }
    $status = $?
  }
  # record the end time
  my $t1 = [&gettimeofday()];
  # check if the caller wants the elapsed time
  if (defined($opts{TIME})) {
    ${$opts{TIME}} = &tv_interval($t0, $t1);
  }
  # close and unlink input temporary file
  if (defined($in_var_fh)) {
    close($in_var_fh);
    unlink($in_var_fn);
  }
  # rewind, slurp, close and unlink output temporary files
  ${$opts{ALL_VAR}} = &rewind_slurp_close($all_var_fh, $all_var_fn) if $all_var_fh;
  ${$opts{OUT_VAR}} = &rewind_slurp_close($out_var_fh, $out_var_fn) if $out_var_fh;
  ${$opts{ERR_VAR}} = &rewind_slurp_close($err_var_fh, $err_var_fn) if $err_var_fh;
  # now check the status (if the caller requests it)
  if ($opts{CHECK_STATUS}) {
    if ($status == -1) {
      die("Failed to execute command '". &stringify_args2(%opts) . "'\n");
    } elsif ($status & 127) {
      die(sprintf("Process executing command '%s' died with signal %d, %s coredump.",
          &stringify_args2(%opts), ($status & 127), ($status & 128) ? 'with' : 'without'));
    } elsif ($status != 0) {
      die(sprintf("Process executing command '%s' exited with value %d indicating failure.", 
          &stringify_args2(%opts), $? >> 8));
    }
  }
  # store a human readable version of the command
  if (defined($opts{CMD})) {
    ${$opts{CMD}} = &stringify_args2(%opts);
  }
  # store if the timeout occured
  if (defined($opts{OOT})) {
    ${$opts{OOT}} = $oot;
  }
  # finally return the status of the called program
  return $status;
}

#
# stringify_arg
# 
# Escapes and quotes an argument
#
sub stringify_arg {
  my ($argcpy) = @_;
  # escape shell characters (Bourne shell specific)
  $argcpy =~ s/([;<>\*\|`&\$!#\(\)\[\]\{\}:'"])/\\$1/g;
  # quote string if it contains spaces
  $argcpy = "\"$argcpy\"" if $argcpy =~ m/\s/;
  return $argcpy;
}

#
# stringify_args
#
# Convert an arguments array into a string in a way that should
# not be ambiguous. Intended for logging. If you are invoking a
# program you should still use the extended version of system
# that takes an argument array.
#
sub stringify_args {
  my @dest = ();
  foreach my $arg (@_) {
    push(@dest, &stringify_arg($arg));
  }
  return join(' ', @dest);
}

#
# stringify_args2
#
# Convert arguments and redirects into a string
#
sub stringify_args2 {
  my %opts = @_;
  my $cmd = &stringify_args($opts{PROG}, @{$opts{ARGS}});
  my $truncate = (defined($opts{TRUNCATE}) ? $opts{TRUNCATE} : 1);
  my $dir = ($truncate ? '>' : '>>');
  if (defined($opts{IN_FILE}) || defined($opts{IN_VAR})) {
    my $name = '$input';
    if (defined($opts{IN_FILE})) {
      $name = (ref($opts{IN_FILE}) ? 'input_file' : &stringify_arg($opts{IN_FILE}));
    }
    $name = $opts{IN_NAME} if defined($opts{IN_NAME});
    $cmd .= ' < ' . $name;
  }
  if (defined($opts{ALL_FILE}) || defined($opts{ALL_VAR})) {
    my $name = '$all_messages';
    if (defined($opts{ALL_FILE})) {
      $name = (ref($opts{ALL_FILE}) ? 'output_file' : &stringify_arg($opts{ALL_FILE}));
    }
    $name = $opts{ALL_NAME} if defined($opts{ALL_NAME});
    $cmd .= ' &'. $dir . ' ' . $name;
  } else {
    if (defined($opts{OUT_FILE}) || defined($opts{OUT_VAR})) {
      my $name = '$output_messages';
      if (defined($opts{OUT_FILE})) {
        $name = (ref($opts{OUT_FILE}) ? 'output_file' : &stringify_arg($opts{OUT_FILE}));
      } 
      $name = $opts{OUT_NAME} if defined($opts{OUT_NAME});
      $cmd .= ' 1'. $dir . ' ' . $name;
    }
    # check if we're redirecting stderr
    if (defined($opts{ERR_FILE}) || defined($opts{ERR_VAR})) {
      my $name = '$error_messages';
      if (defined($opts{ERR_FILE})) {
        $name = (ref($opts{ERR_FILE}) ? 'error_file' : &stringify_arg($opts{ERR_FILE}));
      }
      $name = $opts{ERR_NAME} if defined($opts{ERR_NAME});
      $cmd .= ' 2' . $dir . ' ' . $name;
    }
  }
  return $cmd;
}


sub rewind_slurp_close {
  my ($fh, $fn) = @_;
  return unless defined($fh);
  seek($fh, 0, SEEK_SET);
  my $content = do {local $/ = undef; <$fh>};
  if (defined($fn)) {
    close($fh);
    unlink($fn);
  } else {
    close($fh);
  }
  return $content;
}
