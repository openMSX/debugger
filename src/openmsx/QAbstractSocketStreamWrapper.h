#ifndef QTCP_SOCKET_STREAM_WRAPPER_H
#define QTCP_SOCKET_STREAM_WRAPPER_H

#ifdef _WIN32

#include <QAbstractSocket>
#include "SspiUtils.h"

namespace openmsx {

using namespace sspiutils;

class QAbstractSocketStreamWrapper : public StreamWrapper
{
private:
	QAbstractSocket* sock;
public:
	QAbstractSocketStreamWrapper(QAbstractSocket* userSock);

	unsigned Read (void* buffer, unsigned cb);
	unsigned Write(void* buffer, unsigned cb);
};

} // namespace openmsx

#endif // _WIN32

#endif // QTCP_SOCKET_STREAM_WRAPPER_H
