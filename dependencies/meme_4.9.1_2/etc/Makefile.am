## $Id$
##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = no-dependencies

CENTRIMO_SERVICE = $(bindir)/centrimo_webservice
DREME_SERVICE = $(bindir)/dreme_webservice
FIMO_SERVICE = $(bindir)/fimo_webservice
GLAM2_SERVICE = $(bindir)/glam2_webservice
GLAM2SCAN_SERVICE = $(bindir)/glam2scan_webservice
GOMO_SERVICE = $(bindir)/gomo_webservice
MAST_SERVICE = $(bindir)/mast_webservice
MCAST_SERVICE = $(bindir)/mcast_webservice
MEME_SERVICE = $(bindir)/meme_webservice
MEMECHIP_SERVICE = $(bindir)/meme-chip_webservice
SPAMO_SERVICE = $(bindir)/spamo_webservice
TOMTOM_SERVICE = $(bindir)/tomtom_webservice

SEDSPEC = \
	-e 's%@MEMEDIR@%$(prefix)%' \
	-e 's%@MEMEDB@%$(MEME_DB)%' \
	-e 's%@MEMELOGS@%$(MEME_LOGS)%' \
	-e 's%@SITE_URL@%$(URL)%' \
	-e 's%@METAMEME_URL@%$(URL)%' \
	-e 's%@AMIGO_URL@%$(GOSERVER_URL)%'
	
SEDBIN = \
	-e 's%@WHICHPERL@%$(PERL)%' \
	-e 's%@PERLMODDIR@%$(perlmoddir)%' \
	-e 's%@PERLLIBDIR@%$(libdir)/perl%' \
	-e 's%@MEMEDIR@%$(prefix)%' \
	-e 's%@MEMEDB@%$(MEME_DB)%' \
	-e 's%@MEMELOGS@%$(MEME_LOGS)%' \
	-e 's%@BINARYDIR@%$(bindir)%' \
	-e 's%@APPCONFIGDIR@%$(sysconfdir)%' \
	-e 's%@SITE_URL@%$(URL)%'

SED_CENTRIMO_OPAL = -e 's%@CENTRIMO_SERVICE@%$(CENTRIMO_SERVICE)%'
SED_DREME_OPAL = -e 's%@DREME_SERVICE@%$(DREME_SERVICE)%'
SED_FIMO_OPAL = -e 's%@FIMO_SERVICE@%$(FIMO_SERVICE)%'
SED_GLAM2_OPAL = -e 's%@GLAM2_SERVICE@%$(GLAM2_SERVICE)%'
SED_GLAM2SCAN_OPAL = -e 's%@GLAM2SCAN_SERVICE@%$(GLAM2SCAN_SERVICE)%'
SED_GOMO_OPAL = -e 's%@GOMO_SERVICE@%$(GOMO_SERVICE)%'
SED_MAST_OPAL = -e 's%@MAST_SERVICE@%$(MAST_SERVICE)%'
SED_MCAST_OPAL = -e 's%@MCAST_SERVICE@%$(MCAST_SERVICE)%'
SED_MEME_OPAL = -e 's%@MEME_SERVICE@%$(MEME_SERVICE)%'
SED_MEMECHIP_OPAL = -e 's%@MEMECHIP_SERVICE@%$(MEMECHIP_SERVICE)%'
SED_SPAMO_OPAL = -e 's%@SPAMO_SERVICE@%$(SPAMO_SERVICE)%'
SED_TOMTOM_OPAL = -e 's%@TOMTOM_SERVICE@%$(TOMTOM_SERVICE)%'
SED_VER = -e 's%@SERVICE_VERSION@%$(VERSION)%'


ETCFILES = \
  ame_template.html \
  ame_template.css \
  ame_template.js \
  beadstring.xsl \
  bkg.jpg \
  block-diagram.xsl \
  centrimo_graph.js \
  centrimo_template.html \
  centrimo_template.css \
  centrimo_template.js \
  cisml.css \
  cisml-to-gff3.xsl \
  cisml-to-html.xsl \
  cisml-to-text.xsl \
  delay_draw.js \
  dreme-to-html.xsl \
  dreme-to-text.xsl \
  eps_context.js \
  fimo-to-gff3.xsl \
  fimo-to-html.xsl \
  fimo-to-text.xsl \
  gomo-to-html.xsl \
  help.gif \
  hmm-text.xsl \
  mast-to-html.xsl \
  mcast-to-gff3.xsl \
  mcast-to-html.xsl \
  mcast-to-text.xsl \
  mcast-to-wiggle.xsl \
  meme.css \
  meme-chip_template.html \
  meme-chip_template.css \
  meme-chip_template.js \
  meme-to-html.xsl \
  meme.doc \
  motif_logo.js \
  prior1.plib \
  prior30.plib \
  spamo_template.eps \
  spamo-to-html.xsl \
  template.eps \
  template.svg \
  tomtom-to-html.xsl \
  utilities.js

meme.doc: ../doc/meme.html
	elinks -dump ../doc/meme.html > meme.doc

mast.doc: ../doc/mast.html
	elinks -dump ../doc/mast.html > mast.doc

etcdir=$(sysconfdir)

etc_DATA = \
  $(DATABASE_FILES) \
  $(ETCFILES) \
  constants.js \
  constants.xsl \
  delay_draw.js.xsl \
  dreme-to-html.css.xsl \
  dreme-to-html.js.xsl \
  centrimo_config.xml \
  fimo_config.xml \
  glam2_config.xml \
  glam2scan_config.xml \
  dreme_config.xml \
  gomo_config.xml \
  logging.conf \
  mast_config.xml \
  mcast_config.xml \
  meme_config.xml \
  meme-chip_config.xml \
  meme.css.xsl \
  motif_logo.js.xsl \
  spamo-to-html.css.xsl \
  spamo-to-html.js.xsl \
  spamo_graph.js.xsl \
  spamo_config.xml \
  tomtom_config.xml \
  utilities.js.xsl

if WEBSITE
memehtmldir = $(WEBDIR)
# _DATA doesn't automaticaly include the file in the distribution so it needs to be in EXTRA_DIST too
memehtml_DATA = \
  motif_logo.js \
  meme.css
endif

constants.js: constants.js.in
	$(SED) $(SEDSPEC) $? > $@

constants.xsl: constants.xsl.in
	$(SED) $(SEDSPEC) $? > $@

logging.conf: logging.conf.in 
	$(SED) $(SEDSPEC) $? > $@

%.xsl: %
	$(SED) 's/tname/$?/' xsl-wrapper.in | $(SED) '5r $?' > $@

centrimo_config.xml: centrimo_config.xml.in
	$(SED) $(SED_CENTRIMO_OPAL) $(SED_VER) $? > $@

dreme_config.xml: dreme_config.xml.in
	$(SED) $(SED_DREME_OPAL) $(SED_VER) $? > $@

fimo_config.xml: fimo_config.xml.in
	$(SED) $(SED_FIMO_OPAL) $(SED_VER) $? > $@

glam2_config.xml: glam2_config.xml.in
	$(SED) $(SED_GLAM2_OPAL) $(SED_VER) $? > $@

glam2scan_config.xml: glam2scan_config.xml.in
	$(SED) $(SED_GLAM2SCAN_OPAL) $(SED_VER) $? > $@

gomo_config.xml: gomo_config.xml.in
	$(SED) $(SED_GOMO_OPAL) $(SED_VER) $? > $@

mast_config.xml: mast_config.xml.in
	$(SED) $(SED_MAST_OPAL) $(SED_VER) $? > $@

mcast_config.xml: mcast_config.xml.in
	$(SED) $(SED_MCAST_OPAL) $(SED_VER) $? > $@

meme_config.xml: meme_config.xml.in
	$(SED) $(SED_MEME_OPAL) $(SED_VER) $? > $@

meme-chip_config.xml: meme-chip_config.xml.in
	$(SED) $(SED_MEMECHIP_OPAL) $(SED_VER) $? > $@

spamo_config.xml: spamo_config.xml.in
	$(SED) $(SED_SPAMO_OPAL) $(SED_VER) $? > $@

tomtom_config.xml: tomtom_config.xml.in
	$(SED) $(SED_TOMTOM_OPAL) $(SED_VER) $? > $@

DATABASE_INDEXES = \
  fasta_db.index \
  gomo_db.index \
  motif_db.index

DATABASE_FILES = \
  db_general.csv \
  db_other_genomes.csv \
  fasta_db.csv \
  gomo_db.csv \
  motif_db.csv \
  $(DATABASE_INDEXES)

# if database does not exist, create an empty one.
%_db.csv:
	touch $@

# index the database
%_db.index: %_db.csv
	$(PERL) -I ../scripts/ -MCatList -e 'CatList::index_catlist_csv("$?", "$@")'


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

CLEANFILES = constants.js constants.xsl centrimo_config.xml delay_draw.js.xsl dreme-to-html.css.xsl \
             dreme-to-html.js.xsl dreme_config.xml fimo_config.xml \
             glam2_config.xml glam2scan_config.xml \
             gomo_config.xml gomo_db.index mast_config.xml \
             mcast_config.xml meme.css.xsl meme_config.xml meme-chip_config.xml \
             motif_logo.js.xsl spamo-to-html.css.xsl \
             spamo-to-html.js.xsl spamo_config.xml spamo_graph.js.xsl \
             tomtom_config.xml utilities.js.xsl \
             $(DATABASE_INDEXES)

EXTRA_DIST = $(ETCFILES) \
             constants.js.in constants.xsl.in centrimo_config.xml.in db_general.csv \
             db_other_genomes.csv delay_draw.js \
             dreme_config.xml.in dreme-to-html.css dreme-to-html.js \
             fimo_config.xml.in glam2_config.xml.in glam2scan_config.xml.in \
             gomo_config.xml.in logging.conf.in motif_logo.js mast_config.xml.in \
             mcast_config.xml.in meme_config.xml.in  meme-chip_config.xml.in \
             meme.css spamo-to-html.css spamo-to-html.js spamo_graph.js \
             spamo_config.xml.in tomtom_config.xml.in \
             xsl-wrapper.in


