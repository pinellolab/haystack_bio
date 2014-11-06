## Process this file with automake to produce Makefile.in

if BUILD_LIBXML2
SUBDIRS = include .

DIST_SUBDIRS = include .
noinst_LTLIBRARIES = libxml2.la

INCLUDES =  -I@srcdir@ -I@srcdir@/include

# We don't control this code so turn off warnings.
libxml2_la_CFLAGS =  $(AM_CFLAGS) $(INCLUDES) -w
libxml2_la_LIBADD =  $(ICONV_LIBS) 

libxml2_la_SOURCES = \
	chvalid.c\
	debugXML.c \
	dict.c \
	elfgcchack.h \
	encoding.c \
	entities.c \
	error.c \
	globals.c \
	hash.c \
	HTMLparser.c \
	HTMLtree.c \
	libxml.h \
	list.c \
	parserInternals.c  \
	parser.c \
	pattern.c \
	relaxng.c \
	tree.c \
	SAX2.c \
	threads.c \
	uri.c  \
	valid.c \
	xmlIO.c \
	xmlreader.c \
	xmlregexp.c \
	xmlsave.c \
	xmlstring.c \
	xmlschemas.c \
	xmlschemastypes.c \
	xmlunicode.c \
	xmlwriter.c \
	xmlmemory.c \
	xpath.c

EXTRA_DIST = \
	c14n.c \
	catalog.c \
	DOCBparser.c \
	legacy.c \
	nanohttp.c \
	nanoftp.c \
	schematron.c \
	SAX.c \
	xinclude.c \
	xlink.c \
	xmlmodule.c \
	xpointer.c \
	Copyright \
	triodef.h \
	trio.c \
	trio.h \
	trionan.c \
	trionan.h \
	triop.h \
	triostr.c \
	triostr.h

cleanup:
	-@(find . -name .\#\* -exec rm {} \;)

endif
