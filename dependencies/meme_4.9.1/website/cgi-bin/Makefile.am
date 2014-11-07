## $Id$
##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = no-dependencies

memecgidir = $(WEBDIR)/cgi-bin

SEDSPEC = -e 's%@sendmail@%$(SENDMAIL)%' \
          -e 's%@mail@%$(MAIL)%' \
	  -e 's%@WHICHPERL@%$(PERL)%' \
	  -e 's%@WHICHGHOSTSCRIPT@%$(GHOSTSCRIPT)%' \
	  -e 's%@PERLLIBDIR@%$(libdir)/perl%' \
	  -e 's%@contact@%$(CONTACT)%g' \
	  -e 's%@MEME_DIR@%$(MEME_DIRECTORY)%' \
	  -e 's%@APPCONFIGDIR@%$(sysconfdir)%' \
	  -e 's%@TMP_DIR@%$(MEME_TEMP)%' \
	  -e 's%@MEMEDB@%$(MEME_DB)%' \
	  -e 's%@S_VERSION@%$(VERSION)%' \
	  -e 's%@SITE_URL@%$(URL)%' \
	  -e 's%@OPAL@%$(OPAL_URL)%' \
	  -e 's%\@BLOCKS_URL\@%$(BLOCKS_URL)%' \
	  -e 's%@AMIGO_URL@%$(GOSERVER_URL)%'

SUFFIXES= .pl .cgi
%.cgi: %.pl Makefile
	$(SED) $(SEDSPEC) $< > $@
%.pm: %.pm.in Makefile
	$(SED) $(SEDSPEC) $< > $@

MEME_FILES = \
  centrimo.pl \
  dreme.pl \
  echo.pl \
  fimo.pl \
  gomo.pl \
  mast.pl \
  mcast.pl \
  meme.pl \
  meme-chip.pl \
  get_db_list.pl \
  glam2.pl \
  glam2scan.pl \
  meme_request.pl \
  process_request.pl \
  querystatus.pl \
  OpalServices.pm.in \
  OpalTypes.pm.in \
  spamo.pl \
  tomtom.pl \
  tomtom_request.pl

WEBSITE_TEMPLATES = \
  centrimo.tmpl \
  centrimo_verify.tmpl \
  db.tmpl \
  dreme.tmpl \
  dreme_verify.tmpl \
  db_list_view.tmpl \
  fimo_verify.tmpl \
  footer.tmpl \
  glam2_verify.tmpl \
  glam2scan_verify.tmpl \
  gomo_verify.tmpl \
  mast_verify.tmpl \
  mcast_verify.tmpl \
  meme_verify.tmpl \
  meme-chip.tmpl \
  meme-chip_verify.tmpl \
  motif_input.tmpl \
  spamo.tmpl \
  spamo_verify.tmpl \
  tomtom.tmpl \
  tomtom_verify.tmpl

# Templates on both sides of a split install
DUALUSE_TEMPLATES = \
  job_status.tmpl

WEBSERVICE_TEMPLATES = 

WEBSITE_CGI = \
  centrimo.cgi \
  dreme.cgi \
  echo.cgi \
  fimo.cgi \
  gomo.cgi \
  mast.cgi \
  mcast.cgi \
  meme.cgi \
  meme-chip.cgi \
  meme_request.cgi \
  get_db_list.cgi \
  glam2.cgi\
  glam2scan.cgi \
  OpalServices.pm \
  OpalTypes.pm \
  process_request.cgi \
  querystatus.cgi \
  spamo.cgi \
  tomtom.cgi \
  tomtom_request.cgi

MEME_TEMPLATES =
MEME_CGI =

# WEBSITE OR WEBSERVICE
if WEBSITE
MEME_TEMPLATES += $(DUALUSE_TEMPLATES)
else !WEBSITE
if WEBSERVICE
MEME_TEMPLATES += $(DUALUSE_TEMPLATES)
endif WEBSERVICE
endif !WEBSITE

if WEBSITE
MEME_TEMPLATES += $(WEBSITE_TEMPLATES)
MEME_CGI += $(WEBSITE_CGI)
endif

if WEBSERVICE
MEME_TEMPLATES += $(WEBSERVICE_TEMPLATES)
endif

memecgi_SCRIPTS = $(MEME_CGI)
memecgi_DATA = $(MEME_TEMPLATES)

# If .svnignore file doesn't exist
# create an empty one
.svnignore:
	if [ -e .svn ] ; then touch .svnignore ; fi

# Update SVN svn:ignore property
# If .svnignore exists and longer then 0 bytes.
.svnignore.old:.svnignore
	if [ -e .svn ] ; \
  then \
    if [ -s .svnignore ] ; \
    then \
      svn propset svn:ignore -F .svnignore .  ; \
      cp .svnignore .svnignore.old; \
    fi \
  fi

CLEANFILES = $(MEME_CGI)

# These files should be included in the distribution.
EXTRA_DIST = $(MEME_FILES) $(WEBSITE_TEMPLATES) $(DUALUSE_TEMPLATES) $(WEBSERVICE_TEMPLATES)
