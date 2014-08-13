#!@WHICHPERL@ -w

use strict;
use CGI;
use CGI::Carp qw(fatalsToBrowser);

# Setup logging
my $logger = undef;
eval {
  require Log::Log4perl;
  Log::Log4perl->import();
};
unless ($@) {
  Log::Log4perl::init('@APPCONFIGDIR@/logging.conf');
  $logger = Log::Log4perl->get_logger('meme.cgi.echo');
  $SIG{__DIE__} = sub {
    return if ($^S);
    $logger->fatal(@_);
    die @_;
  };
}
$logger->trace("Starting echo CGI") if $logger;

# globals
$CGI::POST_MAX = 1024 * 1024 * 5; # 5MB upload limit
$CGI::DISABLE_UPLOADS = 1; # Disallow file uploads

my $q = CGI->new();
my $name = $q->param('name');
my $content = $q->param('content');
my $mime_type = $q->param('mime_type');
die("missing name") unless defined $name;
die("missing content") unless defined $content;
die("missing mime_type") unless defined $mime_type;
binmode STDOUT;
print $q->header(-type=>$mime_type, -Content_Disposition=>"attachment; filename=$name");
print $content;
exit(0);
