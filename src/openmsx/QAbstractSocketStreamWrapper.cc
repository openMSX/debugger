// $Id$

#ifdef _WIN32

#include "QAbstractSocketStreamWrapper.hh"

namespace openmsx {

QAbstractSocketStreamWrapper::QAbstractSocketStreamWrapper(QAbstractSocket* userSock)
	: sock(userSock)
{
}

unsigned int QAbstractSocketStreamWrapper::Read(void* buffer, unsigned int cb)
{
	sock->waitForReadyRead(30);
	qint64 recvd = sock->read(static_cast<char*>(buffer), cb);
	if (recvd == -1) {
		return STREAM_ERROR;
	}
	return static_cast<unsigned int>(recvd);
}

unsigned int QAbstractSocketStreamWrapper::Write(void* buffer, unsigned int cb)
{
	qint64 sent = sock->write(static_cast<char*>(buffer), cb);
	if (sent == -1 || !sock->flush()) {
		return STREAM_ERROR;
	}
	return static_cast<unsigned int>(sent);
}

} // namespace openmsx

#endif
