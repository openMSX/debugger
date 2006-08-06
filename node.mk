# $Id$

include build/node-start.mk

SUBDIRS:= \
	src build resources

DIST:= \
	ChangeLog AUTHORS GPL README GNUmakefile

include build/node-end.mk

