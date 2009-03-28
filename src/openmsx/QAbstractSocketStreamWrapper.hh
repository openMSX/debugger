// $Id$

#ifndef QTCP_SOCKET_STREAM_WRAPPER_HH
#define QTCP_SOCKET_STREAM_WRAPPER_HH

#ifdef _WIN32

#include <QAbstractSocket>
#include "SspiUtils.hh"

namespace openmsx {

using namespace sspiutils;

class QAbstractSocketStreamWrapper : public StreamWrapper
{
private:
	QAbstractSocket* sock;
public:
	QAbstractSocketStreamWrapper(QAbstractSocket* userSock);

	unsigned int Read(void* buffer, unsigned int cb);
	unsigned int Write(void* buffer, unsigned int cb);
};

} // namespace openmsx

#endif // _WIN32

#endif // QTCP_SOCKET_STREAM_WRAPPER_HH
