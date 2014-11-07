if BUILD_LIBXSLT
noinst_LTLIBRARIES = libexslt.la

libexslt_la_SOURCES = \
	common.c \
	date.c \
	dynamic.c \
	exslt.c \
	exslt.h \
	exsltconfig.h \
	exsltexports.h \
	functions.c \
	libexslt.h \
	math.c \
	saxon.c \
	sets.c \
	strings.c

libexslt_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(LIBXML2_CFLAGS) \
	$(LIBXSLT_CFLAGS) \
	-DHAVE_MATH_H \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/libxml2/include \
	-I$(top_srcdir)/src/libxslt

libexslt_la_LIBADD = $(LIBXML2_LIBS) $(LIBXSLT_LIBS)

EXTRA_DIST = \
	crypto.c

endif
