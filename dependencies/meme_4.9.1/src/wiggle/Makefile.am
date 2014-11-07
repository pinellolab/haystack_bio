## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libwiggle.la

INCLUDES = -I@srcdir@

# We don't control this code so turn off warnings.
libwiggle_la_CFLAGS =  $(AM_CFLAGS) -w
libwiggle_la_LIBADD =  $(ICONV_LIBS) 
libwiggle_la_SOURCES = \
	bPlusTree.c \
	base64.c \
	basicBed.c \
	bbiRead.c \
	bbiWrite.c \
	bigBed.c \
	bits.c \
	bwgCreate.c \
	bwgQuery.c \
	cheapcgi.c \
	cirTree.c \
	common.c \
	dlist.c \
	dystring.c \
	errabort.c \
	hash.c \
	hmmstats.c \
	https.c \
	intExp.c \
	internet.c \
	kxTok.c \
	linefile.c \
	localmem.c \
	memalloc.c \
	metaWig.c \
	mime.c \
	net.c \
	obscure.c \
	options.c \
	osunix.c \
	pipeline.c \
	portimpl.c \
	rangeTree.c \
	servcl.c \
	servcis.c \
	servmsII.c \
	servBrcMcw.c \
	servCrunx.c \
	servpws.c \
	sqlNum.c \
	udc.c \
	verbose.c \
	wildcmp.c \
	zlibFace.c

EXTRA_DIST = \
	aliType.h \
	asParse.h \
	bPlusTree.h \
	base64.h \
	basicBed.h \
	bbiFile.h \
	bigBed.h \
	bigWig.h \
	binRange.h \
	bits.h \
	bwgInternal.h \
	cheapcgi.h \
	cirTree.h \
	common.h \
	dlist.h \
	dnaseq.h \
	dnautil.h \
	dystring.h \
	errabort.h \
	fuzzyFind.h \
	gfxPoly.h \
	hash.h \
	hmmstats.h \
	htmshell.h \
	https.h \
	internet.h \
	kxTok.h \
	linefile.h \
	localmem.h \
	memalloc.h \
	memgfx.h \
	metaWig.h \
	mime.h \
	net.h \
	obscure.h \
	options.h \
	pipeline.h \
	portable.h \
	portimpl.h \
	psl.h \
	rangeTree.h \
	rbTree.h \
	sig.h \
	sqlList.h \
	sqlNum.h \
	udc.h \
	verbose.h \
	zlibFace.h
