
SEDSPEC = -e 's%\@VERSION\@%$(VERSION)%g' \
          -e 's%\@CONTACT\@%$(CONTACT)%g' \
          -e 's%\@DEV_CONTACT\@%$(DEV_CONTACT)%g' \
          -e 's%\@URL\@%$(URL)%g' \
          -e 's%\@PREV_URL\@%$(PREV_URL)%g' \
          -e 's%\@PREV_VER\@%$(PREV_VER)%g' \
          -e 's%\@NOTICES_URL\@%$(NOTICES_URL)%g' \
          -e 's%\@NEWS_URL\@%$(NEWS_URL)%g'

menu-configure.js: menu-configure.js.in Makefile
	$(SED) $(SEDSPEC) $< > $@


# files linked from the etc directory
SYMLINK_FILES = \
 centrimo_graph.js \
 delay_draw.js \
 eps_context.js \
 motif_logo.js \
 spamo_graph.js \
 utilities.js

NORMAL_FILES = \
  Markdown.Converter.js \
  Markdown.Converter.LICENSE \
  menu-data.js \
  menu.js \
  menu-path.js \
  shared-doc.js

# Included in distribution, but not installed.
IN_FILES = \
  menu-configure.js.in

# Not included in distribution, but installed.
BUILT_FILES = \
  menu-configure.js


BUILT_SOURCES = $(BUILT_FILES)

if ! WEBSITE
docjsdir = $(prefix)/doc/js
docjs_DATA =  $(NORMAL_FILES) $(BUILT_FILES) $(SYMLINK_FILES)
endif

CLEANFILES = $(BUILT_FILES) 
EXTRA_DIST = $(NORMAL_FILES) $(IN_FILES) $(SYMLINK_FILES)
