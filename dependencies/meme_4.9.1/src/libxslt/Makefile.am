if BUILD_LIBXSLT
noinst_LTLIBRARIES = libxslt.la

libxslt_la_SOURCES = \
	attributes.c \
	attributes.h \
	attrvt.c \
	documents.c \
	documents.h \
	extensions.c \
	extensions.h \
	extra.c \
	extra.h \
	functions.c \
	functions.h \
	imports.c \
	imports.h \
	keys.c \
	keys.h \
	libxslt.h \
	numbers.c \
	numbersInternals.h \
	namespaces.c \
	namespaces.h \
	pattern.c \
	pattern.h \
	preproc.c \
	preproc.h \
	security.c \
	security.h \
	templates.c \
	templates.h \
	transform.c \
	transform.h \
	variables.c \
	variables.h \
	win32config.h \
	xslt.c \
	xslt.h \
	xsltconfig.h \
	xsltexports.h \
	xsltInternals.h \
	xsltlocale.c \
	xsltlocale.h \
	xsltutils.c \
	xsltutils.h \
	xsltwin32config.h \
	xsltwin32config.h.in

libxslt_la_CFLAGS = $(AM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/src
libxslt_la_LIBADD = $(LIBXML2_LIBS)

EXTRA_DIST = Copyright

cleanup:
	-@(find . -name .\#\* -exec rm {} \;)

endif
