## $Id: Makefile.am 6037 2011-09-27 05:46:03Z james_johnson $
##
## Process this file with automake to produce Makefile.in
##
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
  -e 's%@WEB_DIR@%$(WEBDIR)%' \
  -e 's%@TMP_DIR@%$(MEME_TEMP)%' \
  -e 's%@WHICHPERL@%$(PERL)%' \
  -e 's%@WHICHCONVERT@%$(CONVERT)%' \
  -e 's%@WHICHGHOSTSCRIPT@%$(GHOSTSCRIPT)%' \
  -e 's%@PERLMODDIR@%$(perlmoddir)%' \
  -e 's%@PERLLIBDIR@%$(libdir)/perl%' \
  -e 's%@PYTHONLIBDIR@%$(libdir)/python%' \
  -e 's%@WHICHPYTHON@%$(PYTHON)%' \
  -e 's%@WHICHGHOSTSCRIPT@%$(GHOSTSCRIPT)%' \
  -e 's%@MACH_ID@%$(MACHID)%' \
  -e 's%@sendmail@%$(SENDMAIL)%' \
  -e 's%@mail@%$(MAIL)%' \
  -e 's%\@CONTACT\@%$(CONTACT)%' \
  -e 's%@LAM_BIN@%$(LAMBIN)%' \
  -e 's%@MPICMD@%$(MPI_CMD)%' \
  -e 's%@MEMEDIR@%$(MEME_DIRECTORY)%' \
  -e 's%@MEME_ETC_DIR@%$(top_srcdir)/etc%' \
  -e 's%@APPCONFIGDIR@%$(sysconfdir)%' \
  -e 's%@BINDIR@%$(bindir)%' \
  -e 's%@MEMELOGS@%$(MEME_LOGS)%' \
  -e 's%@MEMEDB@%$(MEME_DB)%' \
  -e 's%@HOST_NAME@%$(HOSTNAME)%' \
  -e 's%@MEMEUSER@%$(MEME_USER)%' \
  -e 's%@MPI_TYPE@%$(MPITYPE)%' \
  -e 's%@S_VERSION@%$(VERSION)%' \
  -e 's%@SITE_URL@%$(URL)%' \
  -e 's%@OPAL@%$(OPAL_URL)%' \
  -e 's%@DOCDIR@%$(MEME_DOC_DIR)/%'

BUILT_PERL_MOD = \
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

MEME_TESTS = runcheck metatest

if PARALLEL
PARALLEL_TESTS = runcheck_p
if PARALLEL_IS_LAM
runcheck_p: runcheck_p.sh.in Makefile
	sed -e "s%@mpirun@%$(top_srcdir)/scripts/lrun -n %" $< > runcheck_p; \
  chmod +x runcheck_p;
else
runcheck_p: runcheck_p.sh.in Makefile
	sed -e "s%@mpirun@%$(MPI_CMD) %" $< > runcheck_p; \
  chmod +x runcheck_p
endif
endif


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
  get_db_csv \
  chen2meme \
  jaspar2meme \
  hart2meme-bkg \
  hartemink2psp \
  iupac2meme \
  meme \
  meme-chip \
  meme-get-motif \
  meme2meme \
  nmica2meme \
  priority2meme \
  psp-gen \
  rna2meme \
  scpd2meme \
  taipale2meme \
  tamo2meme \
  transfac2meme \
  uniprobe2meme \
  update_db

UTILS = \
  meme-rename \
  plotgen \
  sd \
  $(BUILT_UTILS)

OBSOLETE_UTILS = \
  fasta-enriched-center \
  mhmm2html

DEV_UTILS = \
  update_meme_tests 

# services
DUALUSE_SERVICES = \
  tomtom_webservice

WEBSERVICE_SERVICES = \
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
  mailer \
  update-plot-usage

WEBSITE_WEBUTILS = \
  plot-usage \
  texit \
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

# need these to allow use as a Python library or for command line
fasta-hamming-enrich.py: fasta-hamming-enrich
	cp fasta-hamming-enrich fasta-hamming-enrich.py
fasta-dinucleotide-shuffle.py: fasta-dinucleotide-shuffle
	cp fasta-dinucleotide-shuffle fasta-dinucleotide-shuffle.py

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
TESTS = $(MEME_TESTS) $(PARALLEL_TESTS)
bin_SCRIPTS = $(UTILS) $(MEME_WEBUTILS) $(MEME_SERVICES)

# Define which files to delete if target is 'make clean'.
CLEANFILES = \
  $(BUILT_PERL_MOD) \
  $(BUILT_PYTHON_LIB) \
  $(BUILT_UTILS) \
  $(WEBSITE_BUILT_WEBUTILS) \
  $(MEME_SERVICES) \
  metatest

EXTRA_SCRIPTS = $(DEV_UTILS)

EXTRA_DIST = \
  $(NORMAL_PERL_MOD) \
  $(OTHER_PERL_MOD) \
  $(NORMAL_PYTHON_LIB) \
  dependencies.pl \
  meme-rename \
  obo2dag.jar \
  plotgen \
  plot-usage \
  runcheck \
  runtests \
  sd \
  texit \
  update_meme_tests \
  usage-reports \
  $(INPUT_FILES)

dist-hook:
	rm -f $(distdir)/metatest
	rm -f $(distdir)/runcheck_p
