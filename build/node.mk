# $Id$

include build/node-start.mk

DIST:= \
	main.mk version.mk \
	node-end.mk node-start.mk \
	detectsys.py \
	install-recursive.sh \
	package-darwin

include build/node-end.mk
