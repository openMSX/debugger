include build/node-start.mk

SUBDIRS:= \
	src build resources

DIST:= \
	AUTHORS GPL README GNUmakefile

include build/node-end.mk

