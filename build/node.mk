# $Id$

include build/node-start.mk

DIST:= \
	main.mk version.mk \
	node-end.mk node-start.mk \
	config.guess detectsys.sh \
	install-recursive.sh

include build/node-end.mk
