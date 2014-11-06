## $Id: Makefile.am 2599 2008-04-30 08:59:29Z tbailey $
##
## Process this file with automake to produce Makefile.in
##
## # $Log$
##

AUTOMAKE_OPTIONS = no-dependencies

EXAMPLES = \
  crp0.fasta \
  crp0.meme.xml \
  FimoClient.pl \
  FimoClient.py \
  MemeClient.pl \
  MemeClient.py

if ! WEBSITE
exdir = $(prefix)/doc/examples/sample_opal_scripts
ex_DATA = $(EXAMPLES)
endif

EXTRA_DIST = $(EXAMPLES)
