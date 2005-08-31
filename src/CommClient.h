// $Id$

#ifndef _COMMCLIENT_H
#define _COMMCLIENT_H

#include <QTcpSocket>
#include <QByteArray>
#include <deque>

enum { DISCARD_RESULT_ID,
       DISASM_MEMORY_REQ_ID,
       CPUREG_REQ_ID,
       HEXDATA_MEMORY_REQ_ID,
       STACK_MEMORY_REQ_ID,
	
       BREAKPOINTS_REQ_ID,
       SLOTMAP_REQ_ID,

       INIT_PAUSE,
       INIT_BREAK
};

class CommRequest
{
public:
	enum RequestType {REQUEST_DEBUGGABLE, REQUEST_COMMAND};
	RequestType requestType;
	int sourceID;
};

struct UpdateMessage {
	QByteArray type;
	QByteArray name;
	QByteArray result;
};

class CommDebuggableRequest : public CommRequest
{
public:
	CommDebuggableRequest(int source, 
	                      const char *debuggableName, 
	                      int readAt = 0, 
	                      int length = 0, 
	                      unsigned char *targetPtr = NULL, 
	                      int writeAt = 0);

	QByteArray debuggable;
	unsigned char *target;
	int readOffset;
	int readSize;
	int writeOffset;
};


class CommCommandRequest : public CommRequest
{
public:
	CommCommandRequest(int source, const char *cmd);

	QByteArray command;
	QByteArray result;
};


class CommClient : public QObject
{
	Q_OBJECT
public:
	CommClient();
	~CommClient();

	void connectToOpenMSX(const QString& host, quint16 port = 9938);
	void closeConnection();

public slots:
	void getDebuggableData(CommDebuggableRequest *r);
	void getCommandResult(CommCommandRequest *r);

signals:
	void connectionReady();
	void connectionTerminated();
	void dataTransferReady(CommRequest *r);
	void dataTransferCancelled(CommRequest *r);
	void updateReceived(UpdateMessage *m);
	void errorOccured( QString error );

private:
	QTcpSocket *socket;
	bool connectionEstablished;
	bool waitingForOpenMSX;
	std::deque<CommRequest *> commandQueue;
	
	void sendCommand(const QByteArray& cmd);
	void rejectRequests();

private slots:
	void socketConnected();
    void socketReadyRead();
    void socketDisconnected();
    void socketError( int e );

};

#endif    // _COMMCLIENT_H
