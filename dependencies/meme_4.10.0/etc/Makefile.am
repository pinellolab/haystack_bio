##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = no-dependencies

DATABASE_FILES = \
  db_general.csv \
  db_other_genomes.csv

SOURCE_CONFIG_FILES = \
  AME_config.xml.in \
  CENTRIMO_config.xml.in \
  DREME_config.xml.in \
  FIMO_config.xml.in \
  GLAM2_config.xml.in \
  GLAM2SCAN_config.xml.in \
  GOMO_config.xml.in \
  MAST_config.xml.in \
  MCAST_config.xml.in \
  MEMECHIP_config.xml.in \
  MEME_config.xml.in \
  SPAMO_config.xml.in \
  TOMTOM_config.xml.in

BUILT_CONFIG_FILES = \
  AME_$(VERSION).xml \
  CENTRIMO_$(VERSION).xml \
  DREME_$(VERSION).xml \
  FIMO_$(VERSION).xml \
  GLAM2_$(VERSION).xml \
  GLAM2SCAN_$(VERSION).xml \
  GOMO_$(VERSION).xml \
  MAST_$(VERSION).xml \
  MCAST_$(VERSION).xml \
  MEMECHIP_$(VERSION).xml \
  MEME_$(VERSION).xml \
  SPAMO_$(VERSION).xml \
  TOMTOM_$(VERSION).xml

SOURCE_ETC_FILES = \
  constants.js.in \
  constants.xsl.in \
  dreme-to-html.css \
  dreme-to-html.js \
  logging.conf.in \
  mast-to-html.css \
  mast-to-html.js \
  spamo-to-html.css \
  spamo-to-html.js \
  spamo_graph.js \
  tomtom-to-html.css \
  tomtom-to-html.js \
  xsl-wrapper.in

NORMAL_ETC_FILES = \
  ame_template.html \
  ame_template.css \
  ame_template.js \
  beadstring.xsl \
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
  hmm-text.xsl \
  job_status.tmpl \
  mast-to-html.xsl \
  mcast-to-gff3.xsl \
  mcast-to-html.xsl \
  mcast-to-text.xsl \
  mcast-to-bed.xsl \
  mcast-to-wiggle.xsl \
  meme.css \
  meme_template.html \
  meme_template.css \
  meme_template.js \
  meme-chip_template.html \
  meme-chip_template.css \
  meme-chip_template.js \
  motif_logo.js \
  prior1.plib \
  prior30.plib \
  spamo_template.eps \
  spamo-to-html.xsl \
  template.eps \
  tomtom-to-html.xsl \
  utilities.js

BUILT_ETC_FILES = \
  constants.js \
  constants.xsl \
  delay_draw.js.xsl \
  dreme-to-html.css.xsl \
  dreme-to-html.js.xsl \
  logging.conf \
  mast-to-html.css.xsl \
  mast-to-html.js.xsl \
  meme.css.xsl \
  motif_logo.js.xsl \
  spamo-to-html.css.xsl \
  spamo-to-html.js.xsl \
  spamo_graph.js.xsl \
  tomtom-to-html.css.xsl \
  tomtom-to-html.js.xsl \
  utilities.js.xsl

%.xsl: %
	$(SED) 's/tname/$?/' xsl-wrapper.in | $(SED) '5r $?' > $@

%_$(VERSION).xml: %_config.xml.in
	$(SED) -e 's%@SERVICE_VERSION@%$(VERSION)%' -e 's%@SERVICE_DIR@%$(bindir)%' $? > $@

SED_URLS = -e 's%@SITE_URL@%$(URL)%' -e 's%@AMIGO_URL@%$(GOSERVER_URL)%'

constants.js: constants.js.in
	$(SED) $(SED_URLS) $? > $@

constants.xsl: constants.xsl.in
	$(SED) $(SED_URLS) $? > $@

logging.conf: logging.conf.in 
	$(SED) $(SED_URLS) $? > $@

etcdir=$(sysconfdir)

etc_DATA = $(DATABASE_FILES) $(NORMAL_ETC_FILES) $(BUILT_ETC_FILES)

if WEBSERVICE
deploydir=$(OPAL_DEPLOY)

deploy_DATA = $(BUILT_CONFIG_FILES)
endif
 
CLEANFILES = $(BUILT_CONFIG_FILES) $(BUILT_ETC_FILES)

EXTRA_DIST = $(DATABASE_FILES) $(SOURCE_ETC_FILES) $(NORMAL_ETC_FILES) $(SOURCE_CONFIG_FILES)


