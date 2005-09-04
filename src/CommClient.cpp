// $Id$

#include "CommClient.h"


static inline char hex2val(char c)
{
	return (c>'9')?(10+c-'A'):(c-'0');
}


CommDebuggableRequest::CommDebuggableRequest(int source, 
                                             const char *debuggableName, 
                                             int readAt, 
                                             int length, 
                                             unsigned char *targetPtr, 
                                             int writeAt)
{
	requestType = REQUEST_DEBUGGABLE;
	sourceID = source;
	debuggable = debuggableName;
	target = targetPtr;
	readOffset = readAt;
	readSize = length;
	writeOffset = writeAt;
}



CommCommandRequest::CommCommandRequest(int source, const char *cmd)
{
	requestType = REQUEST_COMMAND;
	sourceID = source;
	command = cmd;
}



CommClient::CommClient()
{
	connectionEstablished = FALSE;
	socket = new QTcpSocket(this);

	connect( socket, SIGNAL(connected()), SLOT(socketConnected()) );
	connect( socket, SIGNAL(disconnected()), SLOT(socketDisonnected()) );
    connect( socket, SIGNAL(readyRead()), SLOT(socketReadyRead()) );
    connect( socket, SIGNAL(error(QAbstractSocket::SocketError)),
	                 SLOT(socketError(QAbstractSocket::SocketError)) );
	
}

CommClient::~CommClient()
{
}

void CommClient::connectToOpenMSX(const QString & host, quint16 port )
{
	socket->connectToHost(host, port);
}

void CommClient::closeConnection()
{
	if( socket->state() != QAbstractSocket::UnconnectedState )
		socket->disconnectFromHost();
	socketDisconnected();
}

void CommClient::getDebuggableData(CommDebuggableRequest *r) 
{
	if(!connectionEstablished) {
		emit dataTransferCancelled(r);
		return;
	}
	
	commandQueue.push_back(r);
	
	QString cmd = QString("debug_bin2hex [ debug read_block %1 %2 %3 ]")
					.arg(r->debuggable.data())
					.arg(r->readOffset)
					.arg(r->readSize);
	sendCommand(cmd.toAscii());
}	

void CommClient::getCommandResult(CommCommandRequest *r)
{
	if(!connectionEstablished) {
		emit dataTransferCancelled(r);
		return;
	}

	commandQueue.push_back(r);
	sendCommand(r->command);
}

void CommClient::sendCommand(const QByteArray& cmd)
{
	// write to the server
	socket->write("<command>", 9);
	socket->write(cmd.data(), cmd.length() );
	socket->write("</command>\n", 11);
}

void CommClient::socketConnected()
{
	// a timer should be started to timeout the wait for <openmsx-output>
	waitingForOpenMSX = TRUE;
}

void CommClient::socketReadyRead()
{
	// read from the server
	while ( socket->canReadLine() ) {
		QByteArray reply = socket->readLine();
		
		if(reply=="<openmsx-output>\n") {
         	// open openMSX control
			socket->write("<openmsx-control>\n", 18);
			connectionEstablished = TRUE;
			waitingForOpenMSX = FALSE;

			emit connectionReady();

		} else if(reply=="</openmsx-output>\n") {
			// terminate openMSX connection
			connectionEstablished = FALSE;
			socket->disconnectFromHost();
			
			emit connectionTerminated();

		} else if(reply.startsWith("<update")) {
			// handle breakpoints and status changes			
			UpdateMessage *msg = new UpdateMessage;
			
			// read the type attribute
			int start = reply.indexOf(" type=\"")+7;
			int len = 0;
			if(start>=0) {
				len = reply.indexOf('\"', start)-start;
				msg->type = reply.mid(start, len);
			}
			
			// read the name attribute
			start = reply.indexOf(" name=\"", start+len)+7;
			if(start>=0) {
				len = reply.indexOf('\"', start)-start;
				msg->name = reply.mid(start, len);
			}
			
			// read the data
			start = reply.indexOf('>') + 1;
			len = reply.lastIndexOf('<') - start;
			msg->result = reply.mid(start, len);
			
			emit updateReceived( msg );
			
		} else {
			CommRequest *c = commandQueue.front();
		
			if(c->requestType == CommRequest::REQUEST_COMMAND) {
				// command results can be multiple lines 
				CommCommandRequest *r = (CommCommandRequest *)c;
				int start = 0, len = reply.length();
				bool last = FALSE;
				
				if(reply.startsWith("<reply")) {
					start = reply.indexOf('>') + 1;
					r->result = "";
				}
				
				if(reply.endsWith("</reply>\n")) {
					len -= 9;
					last = TRUE;
				}					
				
				r->result.append(reply.mid(start, len-start));
				
				if(last) {
					emit dataTransferReady( c );
					commandQueue.pop_front();
				}

			} else {
				// binary replies are simple hex streams of a single line
				CommDebuggableRequest *r = (CommDebuggableRequest *)c;
				int start = reply.indexOf('>') + 1;
				int end = reply.lastIndexOf('<');
				unsigned char *cptr = r->target + r->readOffset;
				const char *data = reply.data();
			
				for(int i=start; i<end; i+=2, cptr++)
					*cptr = (unsigned char)( (hex2val(data[i]) << 4) + hex2val(data[i+1]));
				
				// send a signal
				emit dataTransferReady( c );
				commandQueue.pop_front();
			}
		}
	}
	
}

void CommClient::socketDisconnected()
{
	connectionEstablished = FALSE;
	emit connectionTerminated();
	rejectRequests();
}

void CommClient::socketError( QAbstractSocket::SocketError e )
{
	ConnectionError error;

	switch (e) {
		case QAbstractSocket::ConnectionRefusedError:
			error = CONNECTION_REFUSED;
			break;
		case QAbstractSocket::RemoteHostClosedError:
			error = CONNECTION_CLOSED;
			break;
		case QAbstractSocket::HostNotFoundError:
			error = HOST_NOT_FOUND;
			break;
		case QAbstractSocket::NetworkError:
			error = NETWORK_ERROR;
			break;
		case QAbstractSocket::UnknownSocketError:
			error = UNKNOWN_ERROR;
			break;
		default:
			error = SOCKET_ERROR;
	}
	
	emit errorOccured(error);
}

void CommClient::rejectRequests()
{
	while(!commandQueue.empty()) {
		emit dataTransferCancelled( commandQueue.front() );
		commandQueue.pop_front();
	}
}
