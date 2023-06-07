EXTENSION    = $(shell grep -m 1 '"name":' META.json | \
               sed -e 's/[[:space:]]*"name":[[:space:]]*"\([^"]*\)",/\1/')
EXTVERSION   = $(shell grep -m 1 '[[:space:]]\{8\}"version":' META.json | \
               sed -e 's/[[:space:]]*"version":[[:space:]]*"\([^"]*\)",\{0,1\}/\1/')
DISTVERSION  = $(shell grep -m 1 '[[:space:]]\{3\}"version":' META.json | \
               sed -e 's/[[:space:]]*"version":[[:space:]]*"\([^"]*\)",\{0,1\}/\1/')

DATA         = $(wildcard sql/*.sql)
DOCS         = $(wildcard doc/*.md)
TESTS        = $(wildcard test/sql/*.sql)
REGRESS      = $(patsubst test/sql/%.sql,%,$(TESTS))
OBJS         = $(patsubst %.c,%.o,$(wildcard src/*.c))
REGRESS_OPTS = --inputdir=test
MODULE_big   = pgms

PG_CONFIG   ?= pg_config
EXTRA_CLEAN  = sql/$(EXTENSION)--$(EXTVERSION).sql

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)


all: sql/$(EXTENSION)--$(EXTVERSION).sql

sql/$(EXTENSION)--$(EXTVERSION).sql: sql/$(EXTENSION).sql
	cp $< $@

.PHONY: results
results:
	rsync -avP --delete results/ test/expected

dist:
	git archive --format zip --prefix=$(EXTENSION)-$(DISTVERSION)/ -o $(EXTENSION)-$(DISTVERSION).zip HEAD

latest-changes.md: Changes
	perl -e 'while (<>) {last if /^(v?\Q${DISTVERSION}\E)/; } print "Changes for v${DISTVERSION}:\n"; while (<>) { last if /^\s*$$/; s/^\s+//; print }' Changes > $@
