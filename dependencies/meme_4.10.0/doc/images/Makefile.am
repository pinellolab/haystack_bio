##
## Automake uses this file to generate a Makefile
##

AUTOMAKE_OPTIONS = no-dependencies

%.png: %.fig
	fig2dev -L png -S 4 $< > $@

PROGRAM_ICONS = \
  ame_icon.png \
  centrimo_icon.png \
  dreme_icon.png \
  fimo_icon.png \
  glam2_icon.png \
  glam2scan_icon.png \
  gomo_icon.png \
  gtscan_icon.png \
  mast_icon.png \
  mcast_icon.png \
  meme_icon.png \
  memechip_icon.png \
  spamo_icon.png \
  tomtom_icon.png

OTHER_IMAGES = \
  cbrc_logo.png \
  memesuite_icon.ico \
  memesuite_icon.png \
  nbcr_logo_trans.gif \
  ncrr_logo.gif \
  sdsc_logo.png \
  uq_logo.png \
  ucsd_logo.png \
  UWlogo150p.gif \
  webserver_schema.png

MEME_IMAGES = $(PROGRAM_ICONS) $(OTHER_IMAGES)

if ! WEBSITE
memeimgdir = $(prefix)/doc/images
memeimg_DATA = $(MEME_IMAGES)
endif

EXTRA_DIST = $(MEME_IMAGES)
