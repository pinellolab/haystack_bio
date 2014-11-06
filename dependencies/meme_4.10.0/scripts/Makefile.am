##
## Process this file with automake to produce Makefile.in
##

# Set installation location for doc files.
if WEBSITE
MEME_DOC_DIR = $(WEBDIR)/doc
else
MEME_DOC_DIR = $(prefix)/doc
endif

ARCHIVE_REVISION = $(shell cat ../ARCHIVE_REVISION)
ARCHIVE_DATE = $(shell cat ../ARCHIVE_DATE)

SEDSPEC = \
  -e 's%@ARCHIVE_REVISION@%$(ARCHIVE_REVISION)%' \
  -e 's%@ARCHIVE_DATE@%$(ARCHIVE_DATE)%' \
  -e 's%@ARCHIVE_VERSION@%$(VERSION)%' \
  -e 's%@TMP_DIR@%$(MEME_TEMP)%' \
  -e 's%@WHICHPERL@%$(PERL)%' \
  -e 's%@WHICHPYTHON@%$(PYTHON)%' \
  -e 's%@WHICHJAVA@%$(JAVA)%' \
  -e 's%@WHICHCONVERT@%$(CONVERT)%' \
  -e 's%@WHICHGHOSTSCRIPT@%$(GHOSTSCRIPT)%' \
  -e 's%@PERLLIBDIR@%$(libdir)/perl%' \
  -e 's%@PYTHONLIBDIR@%$(libdir)/python%' \
  -e 's%@JAVALIBDIR@%$(libdir)/java%' \
  -e 's%@LAM_BIN@%$(LAMBIN)%' \
  -e 's%@APPCONFIGDIR@%$(sysconfdir)%' \
  -e 's%@BINDIR@%$(bindir)%' \
  -e 's%@MEMELOGS@%$(MEME_LOGS)%' \
  -e 's%@MEMEDB@%$(MEME_DB)%' \
  -e 's%@SITE_URL@%$(URL)%' \
  -e 's%@MAX_TIME@%$(MAXTIME)%' \
  -e 's%@DOCDIR@%$(MEME_DOC_DIR)/%'

BUILT_PERL_MOD = \
  Globals.pm \
  MemeWebUtils.pm

NORMAL_PERL_MOD = \
  CatList.pm \
  DremeSAX.pm \
  ExecUtils.pm \
  HtmlMonolithWr.pm \
  JsonRdr.pm \
  JsonWr.pm \
  MemeSAX.pm \
  MotifInDremeXML.pm \
  MotifInMemeXML.pm \
  MotifUtils.pm \
  PriorUtils.pm \
  ReadFastaFile.pm 

OTHER_PERL_MOD = \
  CombDynProg.pm \
  DiffXML.pm \
  HypergeometricDynProg.pm

perlmoddir=$(libdir)/perl
perlmod_SCRIPTS = \
  $(BUILT_PERL_MOD) \
  $(NORMAL_PERL_MOD)

BUILT_PYTHON_LIB = \
  fasta-dinucleotide-shuffle.py \
  fasta-hamming-enrich.py

NORMAL_PYTHON_LIB = \
  hypergeometric.py \
  sequence.py

pythonlibdir=$(libdir)/python
pythonlib_SCRIPTS = \
  $(BUILT_PYTHON_LIB) \
  $(NORMAL_PYTHON_LIB)

MEME_TESTS = metatest

INPUT_FILES = $(wildcard *.in)

BUILT_UTILS = \
  ama-qvalues \
  beeml2meme \
  dreme \
  fasta-center \
  fasta-dinucleotide-shuffle \
  fasta-fetch \
  fasta-hamming-enrich \
  fasta-make-index \
  fasta-most \
  fasta-shuffle-letters \
  fasta-subsample \
  fasta-unique-names \
  glam2html \
  glam2psfm \
  glam2scan2html \
  chen2meme \
  jaspar2meme \
  hart2meme-bkg \
  hartemink2psp \
  iupac2meme \
  matrix2meme \
  meme-chip \
  meme-get-motif \
  meme-rename \
  meme_xml_to_html \
  meme2meme \
  nmica2meme \
  priority2meme \
  psp-gen \
  rna2meme \
  scpd2meme \
  sites2meme \
  taipale2meme \
  tamo2meme \
  transfac2meme \
  uniprobe2meme

UTILS = \
  plotgen \
  sd \
  $(BUILT_UTILS)

OBSOLETE_UTILS = \
  fasta-enriched-center \
  mhmm2html

# services
DUALUSE_SERVICES = \
  tomtom_webservice

WEBSERVICE_SERVICES = \
  ame_webservice \
  centrimo_webservice \
  dreme_webservice \
  fimo_webservice \
  glam2scan_webservice \
  glam2_webservice \
  gomo_webservice \
  mast_webservice \
  mcast_webservice \
  meme-chip_webservice \
  meme_webservice \
  spamo_webservice

#utilities
WEBSITE_BUILT_WEBUTILS = \
  lrun \
  texit \
  update-plot-usage \
  update-sequence-db

WEBSITE_WEBUTILS = \
  plot-usage \
  usage-reports \
  $(WEBSITE_BUILT_WEBUTILS)

MEME_SERVICES =
MEME_WEBUTILS =

# WEBSITE OR WEBSERVICE
if WEBSITE
MEME_SERVICES += $(DUALUSE_SERVICES)
else !WEBSITE
if WEBSERVICE
MEME_SERVICES += $(DUALUSE_SERVICES)
endif WEBSERVICE
endif !WEBSITE


if WEBSERVICE
MEME_SERVICES += $(WEBSERVICE_SERVICES)
endif

if WEBSITE
MEME_WEBUTILS += $(WEBSITE_WEBUTILS)
endif


# Describe dependencies and build instructions for scripts.
# (substitute in local path to Perl).

# input files with unusual build names
MemeWebUtils.pm: MemeWebUtils.pm.in Makefile
	$(SED) $(SEDSPEC) $< > $@
Globals.pm: Globals.pm.in Makefile
	$(SED) $(SEDSPEC) $< > $@

# need these to allow use as a Python library or for command line
fasta-hamming-enrich.py: fasta-hamming-enrich
	cp fasta-hamming-enrich fasta-hamming-enrich.py
fasta-dinucleotide-shuffle.py: fasta-dinucleotide-shuffle
	cp fasta-dinucleotide-shuffle fasta-dinucleotide-shuffle.py

# sh input files
%: %.sh.in
	$(SED) $(SEDSPEC) $< > $@; \
  chmod +x $@

# csh input files
%: %.csh.in
	$(SED) $(SEDSPEC) $< > $@; \
  chmod +x $@

# perl input files
%: %.pl.in
	$(SED) $(SEDSPEC) $< > $@; \
  chmod +x $@

# python input files
%: %.py.in sequence.py hypergeometric.py ../ARCHIVE_DATE ../ARCHIVE_REVISION
	$(SED) $(SEDSPEC) $< > $@; \
  chmod +x $@


# Set automake targets
TESTS_ENVIRONMENT = testdir=$(top_srcdir)/tests
TESTS = $(MEME_TESTS)
bin_SCRIPTS = $(UTILS) $(MEME_WEBUTILS) $(MEME_SERVICES)

# Define which files to delete if target is 'make clean'.
CLEANFILES = \
  $(BUILT_PERL_MOD) \
  $(BUILT_PYTHON_LIB) \
  $(BUILT_UTILS) \
  $(WEBSITE_BUILT_WEBUTILS) \
  $(MEME_SERVICES) \
  metatest

EXTRA_DIST = \
  $(NORMAL_PERL_MOD) \
  $(OTHER_PERL_MOD) \
  $(NORMAL_PYTHON_LIB) \
  dependencies.pl \
  obo2dag.jar \
  plotgen \
  plot-usage \
  sd \
  usage-reports \
  $(INPUT_FILES)

dist-hook:
	rm -f $(distdir)/metatest
