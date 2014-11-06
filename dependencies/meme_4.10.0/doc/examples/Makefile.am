##
## Process this file with automake to produce Makefile.in
##

##############################################
# some examples are also used in tests: check
# scripts/metatest.pl.in before changing

AUTOMAKE_OPTIONS = no-dependencies


SUBDIRS = \
  compute_prior_dist_example_output_files \
  sample_opal_scripts

EXAMPLES = \
  At.fa \
  sample-kabat.seq \
  sample-glam2-aln.dna \
  sample-glam2-aln.protein \
  sample-dna-Klf1.fa \
  sample-dna-motif.meme-io \
  sample-protein-motif.meme-io

# load the EXAMPLE_OUTPUT_FILES variable
include examples.mk

if ! WEBSITE
exdir = $(prefix)/doc/examples
nobase_dist_ex_DATA = $(EXAMPLES) $(EXAMPLE_OUTPUT_FILES)
endif

EXTRA_DIST = examples.mk $(EXAMPLES) $(EXAMPLE_OUTPUT_FILES)
