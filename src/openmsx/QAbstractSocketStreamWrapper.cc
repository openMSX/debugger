// $Id$

#ifdef _WIN32

#include "QAbstractSocketStreamWrapper.hh"

namespace openmsx {

QAbstractSocketStreamWrapper::QAbstractSocketStreamWrapper(QAbstractSocket* userSock)
	: sock(userSock)
{
}

unsigned QAbstractSocketStreamWrapper::Read(void* buffer, unsigned cb)
{
	sock->waitForReadyRead(30);
	qint64 recvd = sock->read(static_cast<char*>(buffer), cb);
	if (recvd == -1) {
		return STREAM_ERROR;
	}
	return unsigned(recvd);
}

unsigned QAbstractSocketStreamWrapper::Write(void* buffer, unsigned cb)
{
	qint64 sent = sock->write(static_cast<char*>(buffer), cb);
	if (sent == -1 || !sock->flush()) {
		return STREAM_ERROR;
	}
	return unsigned(sent);
}

} // namespace openmsx

#endif
