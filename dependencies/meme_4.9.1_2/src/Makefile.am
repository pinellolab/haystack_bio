## Process this file with automake to produce Makefile.in

#####################################################################
#
#  $Id$
#
#####################################################################
# Note:
#
# To change compiler options at build time set the CFLAGS parameter
# on the make command line. For example, to turn on debugging symbols,
# turn off optimization and turn on the DEBUG preprocessor definition
# the command line would be:
#
#   make CFLAGS=" -ggdb -O0 -DDEBUG"
#
#####################################################################

ARCHIVE_REVISION = $(shell cat ../ARCHIVE_REVISION)
ARCHIVE_DATE = $(shell cat ../ARCHIVE_DATE)

if PARALLEL
PARALLEL_DIR = parallel
else
PARALLEL_DIR =
endif

SUBDIRS = filters libxml2 libxslt libexslt wiggle . $(PARALLEL_DIR)

BUILT_SOURCES = dir.h projrel.h

dir.h: Makefile
	echo '#define MEME_DIR "$(prefix)"' >$@
	echo '#define ETC_DIR "$(sysconfdir)" ' >> $@  
	echo '#define BIN_DIR "$(bindir)" ' >> $@

projrel.h: ../ARCHIVE_REVISION ../ARCHIVE_DATE
	echo '#define REVISION "$(ARCHIVE_REVISION)"' > $@
	echo '#define ARCHIVE_DATE "$(ARCHIVE_DATE)"' >> $@

OBSOLETE_PROGS = \
  beadstring \
  draw-mhmm \
  mhmm \
  mhmme \
  mhmms \
  mhmmscan \
  shadow \
  tree

USELESS_PROGS = \
  clustalw-io \
  fasta-io \
  mhmm-io

PROGS = \
  alphtype \
  ama \
  ame \
  ceqlogo \
  centrimo \
  clustalw2fasta \
  clustalw2phylip \
  compute-prior-dist \
  compute-uniform-priors \
  create-priors \
  fasta-get-markov \
  fimo \
  fisher_exact \
  fitevd \
  gendb \
  getsize \
  glam2 \
  glam2format \
  glam2mask \
  glam2scan \
  gomo \
  gomo_highlight \
  llr \
  mast \
  mast2txt \
  mcast \
  meme.bin \
  meme-xml-html \
  meme2images \
  motiph \
  pmp_bf \
  ramen \
  readseq \
  spamo \
  tomtom \
  qvalue \
  ranksum_test \
  reconcile-tree-alignment \
  reduce-alignment \
  remove-alignment-gaps \
  xsltproc_lite

bin_PROGRAMS = $(PROGS)
noinst_LTLIBRARIES = libcommon.la

# Build the source common to all the binaries into a library
libcommon_la_CFLAGS = \
	-fno-strict-aliasing \
	$(AM_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(LIBXSLT_CFLAGS) \
	$(LIBEXSLT_CFLAGS)

libcommon_la_LIBADD = \
	$(LIBXML2_LIBS) \
	$(LIBXSLT_LIBS) \
	$(LIBEXSLT_LIBS) \
	-lm

libcommon_la_SOURCES = \
  alignment.c \
  alphabet.c \
  array.c \
  array-list.c \
  background.c \
  binary-search.c \
  binomial.c \
  build-hmm.c \
  buffer.c \
  ceqlogo.c \
  cisml.c \
  clustalw-io.c \
  dp.c \
  dreme-sax.c \
  eps2png.c \
  empirical-qvalue.c \
  evomodel.c \
  evomodel_with_loss.c \
  fasta-io.c \
  fitevd.c \
  gendb.c \
  hash_alph.c \
  hash_table.c \
  heap.c \
  html-data.c \
  io.c \
  karlin.c \
  linked-list.c \
  log-hmm.c \
  logodds.c \
  mast-util.c \
  match.c \
  matrix.c \
  mcast_match.c \
  meme-sax.c \
  metameme.c \
  mhmm.c \
  mhmms.c \
  mhmmscan.c \
  mhmm-state.c \
  motif-in.c \
  motif-in-common.c \
  motif-in-dreme-xml.c \
  motif-in-meme-html.c \
  motif-in-meme-text.c \
  motif-in-meme-xml.c \
  motif.c \
  motif_regexp.c \
  motifs.c \
  motiph-scoring.c \
  object-list.c \
  order.c \
  data-block.c \
  data-block-reader.c \
  prior-reader-from-psp.c \
  prior-reader-from-wig.c \
  prior-dist.c \
  pssm.c \
  pssm-distr.c \
  qvalue.c \
  ranksum_test.c \
  ranksum_test.h \
  rdb-matrix.c \
  read-mhmm.c \
  read_sequence.c \
  red-black-tree.c \
  regex-utils.c \
  regress.c \
  reservoir.c \
  sax-parser-utils.c \
  scanned-sequence.c \
  scored-sites.c \
  seq.c \
  seq-reader-from-fasta.c \
  simple-getopt.c \
  spearman-rank-correlation.c \
  spearman-rank-correlation.h \
  string-builder.c \
  string-list.c \
  string-match.c \
  subst-matrix.c \
  substmatrix-table.c \
  transfac.c \
  tree.c \
  utils.c \
  wiggle-reader.c \
  write-mhmm.c \
  xml-out.c \
  xml-util.c

alphtype_CFLAGS = $(AM_CFLAGS)
alphtype_LDADD = libcommon.la
alphtype_SOURCES = alphtype.c

# Describe the source files and build options for each executable
ama_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS) $(OPENMP_CFLAGS)
ama_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
ama_SOURCES = \
  ama.c \
  ama_scan.c

ame_CFLAGS = -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS)
ame_SOURCES = ame.c ramen_scan.c html-monolith.c json-writer.c fisher_exact.c
ame_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS) -lm 

beadstring_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
beadstring_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
beadstring_SOURCES = beadstring.c

centrimo_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
centrimo_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
centrimo_SOURCES = centrimo.c html-monolith.c json-writer.c fisher_exact.c

ceqlogo_CFLAGS = -DCL_MAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
ceqlogo_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
ceqlogo_SOURCES = ceqlogo.c

clustalw_io_CFLAGS = -DMAIN $(AM_CFLAGS)
clustalw_io_LDADD = libcommon.la
clustalw_io_SOURCES = clustalw-io.c

clustalw2fasta_CFLAGS = $(AM_CFLAGS)
clustalw2fasta_LDADD = libcommon.la
clustalw2fasta_SOURCES = clustalw2fasta.c

clustalw2phylip_CFLAGS = $(AM_CFLAGS)
clustalw2phylip_LDADD = libcommon.la
clustalw2phylip_SOURCES = clustalw2phylip.c

compute_prior_dist_CFLAGS = $(AM_CFLAGS)
compute_prior_dist_LDADD = libcommon.la
compute_prior_dist_SOURCES = compute-prior-dist.c

compute_uniform_priors_CFLAGS = $(AM_CFLAGS)
compute_uniform_priors_LDADD = libcommon.la
compute_uniform_priors_SOURCES = compute-uniform-priors.c

create_priors_CFLAGS = $(AM_CFLAGS)
create_priors_LDADD = libcommon.la wiggle/libwiggle.la -lz
create_priors_SOURCES = create-priors.c

draw_mhmm_CFLAGS = -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
draw_mhmm_LDADD = libcommon.la
draw_mhmm_SOURCES = \
  draw-mhmm.c \
  read-mhmm.c

fasta_get_markov_CFLAGS = $(AM_CFLAGS)
fasta_get_markov_LDADD = 
fasta_get_markov_SOURCES = fasta-get-markov.c

fasta_io_CFLAGS = -DMAIN $(AM_CFLAGS)
fasta_io_LDADD = libcommon.la
fasta_io_SOURCES = fasta-io.c

fimo_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
fimo_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
fimo_SOURCES = fimo.c

fisher_exact_CFLAGS = -DFE_MAIN $(AM_CFLAGS)
fisher_exact_LDADD = libcommon.la 
fisher_exact_SOURCES = fisher_exact.c

fitevd_CFLAGS = -DMAIN $(AM_CFLAGS)
fitevd_LDADD = libcommon.la
fitevd_SOURCES = fitevd.c

gendb_CFLAGS = -DMAIN $(AM_CFLAGS)
gendb_LDADD = libcommon.la
gendb_SOURCES = gendb.c

getsisze_CFLAGS = $(AM_CFLAGS)
getsize_LDADD = libcommon.la
getsize_SOURCES = getsize.c

glam2_CFLAGS = $(AM_CFLAGS)
glam2_LDADD = libcommon.la
glam2_SOURCES = \
  glam2_alphabet.c \
  glam2_args.c \
  glam2_column_sample.c \
  glam2_dirichlet.c \
  glam2_dna_prior.c \
  glam2_fasta.c \
  glam2_glam2.c \
  glam2_glam2_aln.c \
  glam2_init.c \
  glam2_output.c \
  glam2_recode3_20comp.c \
  glam2_site_sample.c \
  glam2_util.c

glam2scan_CFLAGS = $(AM_CFLAGS)
glam2scan_LDADD = libcommon.la
glam2scan_SOURCES = \
  glam2_alignment.c \
  glam2_alphabet.c \
  glam2_dna_prior.c \
  glam2_fasta.c \
  glam2_dirichlet.c \
  glam2_heap.c \
  glam2_motif.c \
  glam2_scan.c \
  glam2_scan_args.c \
  glam2_scan_init.c \
  glam2_scan_output.c \
  glam2_recode3_20comp.c \
  glam2_util.c

glam2format_CFLAGS = $(AM_CFLAGS)
glam2format_LDADD =  libcommon.la
glam2format_SOURCES = \
  glam2_glam2format.c \
  glam2_alignment.c \
  glam2_fasta.c \
  glam2_util.c

glam2mask_CFLAGS = $(AM_CFLAGS)
glam2mask_LDADD =  libcommon.la
glam2mask_SOURCES = \
  glam2_glam2mask.c \
  glam2_alignment.c \
  glam2_fasta.c \
  glam2_util.c

gomo_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS) $(OPENMP_CFLAGS)
gomo_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
gomo_SOURCES = \
  gomo.c \
  gomo_highlight.c \
  merger.c \
  ranksum_test.c \
  read_csv.c

gomo_highlight_CFLAGS = -DMAIN $(AM_CFLAGS)
gomo_highlight_LDADD = libcommon.la
gomo_highlight_SOURCES = gomo_highlight.c

mast_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
mast_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
mast_SOURCES = \
  diagram.c \
  mast.c

mast2txt_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
mast2txt_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
mast2txt_SOURCES = \
  mast2txt.c

mcast_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
mcast_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
mcast_SOURCES = mcast.c

meme_bin_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(LIBXSLT_CFLAGS) \
	$(LIBEXSLT_CFLAGS)

meme_bin_LDADD = \
	libcommon.la \
	$(LIBXML2_LIBS) \
	$(LIBXSLT_LIBS) \
	$(LIBEXSLT_LIBS)

meme_bin_SOURCES = \
  banner.c \
  branching_search.c \
  calculate_p_y.c \
  discretize.c \
  display.c  \
  dpalign.c  \
  em.c  \
  init.c  \
  justlike.c \
  likelihood.c \
  llr.c \
  logs.c \
  meme.c \
  meme-print-html.c \
  meme_util.c \
  message.c \
  mp.c \
  oops.c \
  partition.c \
  prior.c \
  psp.c \
  read_seq_file.c \
  seed.c \
  seed_diffs.c \
  seq2theta.c \
  sp_matrix.c \
  starts.c \
  subseq7.c \
  tcm.c \
  verbosity.c

meme_xml_html_CFLAGS = ${meme_bin_CFLAGS}
meme_xml_html_LDADD = $(LIBXML2_LIBS) $(LIBXSLT_LIBS) $(LIBEXSLT_LIBS) -lm
meme_xml_html_SOURCES = \
  meme-xml-html.c \
  meme-print-html.c

meme2images_CFLAGS = $(AM_CFLAGS)
meme2images_LDADD = libcommon.la
meme2images_SOURCES = meme2images.c

mhmm_CFLAGS = -DMAIN $(AM_CFLAGS)
mhmm_LDADD = libcommon.la 
mhmm_SOURCES = mhmm.c

mhmm_io_CFLAGS = -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
mhmm_io_LDADD = libcommon.la 
mhmm_io_SOURCES = mhmm-io.c

mhmme_CFLAGS = -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
mhmme_LDADD = libcommon.la 
mhmme_SOURCES = mhmme.c

mhmms_CFLAGS = -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
mhmms_LDADD = libcommon.la 
mhmms_SOURCES = mhmms.c

mhmmscan_CFLAGS = -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
mhmmscan_LDADD = libcommon.la
mhmmscan_SOURCES = mhmmscan.c

motiph_CFLAGS = -DMOTIPH $(AM_CFLAGS) $(LIBXML2_CFLAGS)
motiph_LDADD = libcommon.la 
motiph_SOURCES = motiph.c

pmp_bf_CFLAGS = -DMOTIPH $(AM_CFLAGS) $(LIBXML2_CFLAGS)
pmp_bf_LDADD = libcommon.la 
pmp_bf_SOURCES = pmp_bf.c

ramen_CFLAGS = -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS)
ramen_SOURCES = ramen.c ramen_scan.c
ramen_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS) -lm 

readseq_CFLAGS =$(AM_CFLAGS)
readseq_SOURCES = \
  readseq.c \
  ureadseq.c

shadow_CFLAGS = -DSHADOW $(AM_CFLAGS) $(LIBXML2_CFLAGS)
shadow_LDADD = libcommon.la 
shadow_SOURCES = shadow.c

llr_CFLAGS = -DMAIN $(AM_CFLAGS)
llr_LDADD = libcommon.la 
llr_SOURCES = llr.c logs.c

spamo_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
spamo_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
spamo_SOURCES = \
  cisml-sax.c \
  spamo.c \
  spamo-cisml.c \
  spamo-matches.c \
  spamo-output.c \
  spamo-scan.c

tomtom_CFLAGS =  -DMAIN $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBXSLT_CFLAGS)
tomtom_LDADD = libcommon.la $(LIBXML2_LIBS) $(LIBXSLT_LIBS)
tomtom_SOURCES = tomtom.c

tree_CFLAGS = -DMAIN $(AM_CFLAGS)
tree_LDADD = libcommon.la 
tree_SOURCES = tree.c

qvalue_CFLAGS = -DMAIN $(AM_CFLAGS)
qvalue_LDADD = libcommon.la -lm
qvalue_SOURCES = qvalue.c

ranksum_test_CFLAGS = -DMAIN $(AM_CFLAGS)
ranksum_test_LDADD = libcommon.la -lm
ranksum_test_SOURCES = ranksum_test.c

reconcile_tree_alignment_CFLAGS = -DMAIN $(AM_CFLAGS)
reconcile_tree_alignment_LDADD = libcommon.la -lm
reconcile_tree_alignment_SOURCES = reconcile-tree-alignment.c

reduce_alignment_CFLAGS = -DMAIN $(AM_CFLAGS)
reduce_alignment_LDADD = libcommon.la -lm
reduce_alignment_SOURCES = reduce-alignment.c

remove_alignment_gaps_CFLAGS = -DMAIN $(AM_CFLAGS)
remove_alignment_gaps_LDADD = libcommon.la -lm
remove_alignment_gaps_SOURCES = remove-alignment-gaps.c

xsltproc_lite_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) $(LIBEXSLT_CFLAGS) $(LIBXSLT_CFLAGS)
xsltproc_lite_LDADD = $(LIBXML2_LIBS) $(LIBEXSLT_LIBS) $(LIBXSLT_LIBS) -lm
xsltproc_lite_SOURCES = xsltproc_lite.c

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

CLEANFILES = dir.h projrel.h $(OBSOLETE_PROGS) $(USELESS_PROGS)
DISTCLEANFILES = dir.h projrel.h $(OBSOLETE_PROGS) $(USELESS_PROGS)

EXTRA_PROGRAMS = $(OBSOLETE_PROGS) $(USELESS_PROGS)

#all the header files
EXTRA_DIST = \
  aa.h \
  align.h \
  alignment.h \
  alphabet.h \
  ame.h \
  ama_scan.h \
  array.h \
  array-list.h \
  background.h \
  banner.h \
  beadstring.h \
  beadstring-xml.h \
  binary-search.h \
	binomial.h \
  buffer.h \
  build-hmm.h \
  calculate_p_y.h \
  ceqlogo.h \
  cisml-dtd.h \
  cisml.h \
  cisml-sax.h \
  clustalw-io.h \
  curves.h \
  diagram.h \
  display_globals.h \
  display.h \
  dpalign.h \
  dp.h \
  dreme-sax.h \
  eps2png.h \
  em.h \
  empirical-qvalue.h \
  evomodel.h \
  evomodel_with_loss.h \
  fasta-io.h \
  fisher_exact.h \
  fcodon.h \
  fitevd.h \
  gendb.h \
  general.h \
  glam2_alignment.h \
  glam2_alphabet.h \
  glam2_args.h \
  glam2_column_sample.h \
  glam2_convolve.h \
  glam2_dirichlet.h \
  glam2_dna_prior.h \
  glam2_fasta.h \
  glam2_glam2_aln.h \
  glam2_glam2.h \
  glam2_heap.h \
  glam2_init.h \
  glam2_motif.h \
  glam2_output.h \
  glam2_recode3_20comp.h \
  glam2_scan_args.h \
  glam2_scan.h \
  glam2_scan_init.h \
  glam2_scan_output.h \
  glam2_site_sample.h \
  glam2_util.h \
  glam2_version.h \
  gomo_highlight.h \
  hash_alph.h \
  hash.h \
  hash_table.h \
  hash_table_string.h \
  hashu.h \
  heap.h \
  histogram.h \
  html-data.h \
	html-monolith.h \
  ic.h \
  io.h \
	json-writer.h \
  karlin.h \
  linked-list.h \
  llr.h \
  log-hmm.h \
  logodds.h \
  logs.h \
  macros.h \
  mast.h \
  match.h \
  matrix.h \
  mcast_match.h \
  meme-dtd.h \
  meme.h \
  meme-print-html.h \
  meme-sax.h \
  merger.h \
  message.h \
  metameme.h \
  meta-pssm-distr.h \
  mhmm-dtd.h \
  mhmm.h \
  mhmms.h \
  mhmmscan.h \
  mhmm-state.h \
  motif.h \
  motif-in.h \
  motif-in-common.h \
  motif-in-dreme-xml.h \
  motif-in-flags.h \
  motif-in-meme-html.h \
  motif-in-meme-text.h \
  motif-in-meme-xml.h \
  motif-spec.h \
  motif_regexp.h \
  motifs.h \
  motiph-scoring.h \
  mp.h \
  mtype.h \
  nrutil.h \
  object-list.h \
  order.h \
  params.h \
  partition.h \
  prior.h \
  prior-dist.h \
  data-block.h \
  data-block-reader.h \
  prior-reader-from-psp.h \
  prior-reader-from-wig.h \
  psp.h \
  pssm-distr.h \
  pssm.h \
  qvalue.h \
  ramen.h \
  ramen_scan.h \
  ranksum_test.h \
  reservoir.h \
  rdb-matrix.h \
  read_csv.h \
  read-mhmm.h \
  read_seq_file.h \
  read_sequence.h \
  red-black-tree.h \
  regex-utils.h \
  regress.h \
  sax-parser-utils.h \
  scanned-sequence.h \
  scored-sites.h \
  seed_diffs.h \
  seed.h \
  seq.h \
  seq-reader-from-fasta.h \
  shuffle_int.h \
  simple-getopt.h \
  spamo-cisml.h \
  spamo-matches.h \
  spamo-output.h \
  spamo-scan.h \
  split.h \
  sp_matrix.h \
  star.h \
  string-builder.h \
  string-list.h \
  string-match.h \
  strlcpy.h \
  subst-matrix.h \
  substmatrix-table.h \
  tomtom.h \
  transfac.h \
  tree.h \
  ureadseq.h \
  user.h \
  utest.h \
  utils.h \
  wiggle-reader.h \
  write-mhmm.h \
  xml-out.h \
  xml-util.h \
  zscore.h
