## $Id$
##
## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = no-dependencies

## Call additional Makefiles in child directories
SUBDIRS = ame clustalw2fasta centrimo common create-priors draw-mhmm dreme fasta-center fimo \
          gomo mast mcast meme mhmm mhmms mhmmscan motiph psp-gen qvalue scaffold spamo \
          tomtom web

DATAFILES = \
  adh.s \
  crp0.s \
  farntrans5.s \
  INO_up800.s \
  Jaspar-subset.meme \
  Klf1-200.s \
  Klf1-200-100.s \
  Klf1-200-100-shuffled.s \
  lex0.s \
  lipocalin.s \
  mini-drosoph.s \
  noq.s

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

EXTRA_DIST = $(DATAFILES)
