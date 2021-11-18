pglibdir = $(POSTGRESQL_LIBDIR)
pglib_LTLIBRARIES = libpgms.la


libpgms_la_SOURCES = \
		cosine_greedy.c \
		cosine_hungarian.c \
		cosine.h \
		lsap.c \
		lsap.h \
		pgms.c \
		spectrum.c \
		utils.h


libpgms_la_LDFLAGS = -lm
libpgms_la_CPPFLAGS = -std=gnu99 -O3 -fallow-store-data-races -fno-math-errno $(POSTGRESQL_CPPFLAGS)
