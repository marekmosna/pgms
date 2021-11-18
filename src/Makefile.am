pglibdir = $(POSTGRESQL_LIBDIR)
pglib_LTLIBRARIES = libpgms.la


libpgms_la_SOURCES = \
		pgms.c


libpgms_la_LDFLAGS = -lm
libpgms_la_CPPFLAGS = -std=gnu99 -O3 -fallow-store-data-races -fno-math-errno $(POSTGRESQL_CPPFLAGS)
