# $Id$

include build/node-start.mk

SUBDIRS:= \
	src build resources

DIST:= \
	ChangeLog AUTHORS GPL README TODO

include build/node-end.mk

