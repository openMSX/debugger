include build/node-start.mk

SRC_HDR:= \
	QAbstractSocketStreamWrapper SspiNegotiateClient SspiUtils

HDR_ONLY:= \
	openmsx \
	MSXException

include build/node-end.mk
