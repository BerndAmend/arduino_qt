#include "Arduino.hpp"

Arduino::Arduino(QObject *parent)
	: QObject(parent)
	, m_qwebsocketServer(new QWebSocketServer(QStringLiteral("arduino relay"),
						 QWebSocketServer::NonSecureMode, this))
{
#ifdef HAS_QSERIALPORT
	m_connectionModes.push_back("serial");
#endif
	m_connectionModes.push_back("websocket");
	m_connectionModes.push_back("none");

	QObject::connect(&m_qwebsocket, &QWebSocket::connected, this, &Arduino::connected);
	QObject::connect(&m_qwebsocket, &QWebSocket::disconnected, this, &Arduino::disconnected);
	QObject::connect(&m_qwebsocket, &QWebSocket::textMessageReceived, this, &Arduino::messageReceived);

	QObject::connect(m_qwebsocketServer.get(), &QWebSocketServer::newConnection,
					 this, &Arduino::onNewWebSocketClientConnection);
	//QObject::connect(m_qwebsocketServer, &QWebSocketServer::closed, this, &Arduino::closed);

}

Arduino::~Arduino()
{
	stopWebSocketServer();
}

void Arduino::send(QString cmd)
{
	switch(m_connectionMode) {
	case None:
		break;
	case Serial:
#ifdef HAS_QSERIALPORT
		cmd += "\r";
		serialPort.write(cmd.toUtf8());
#endif
		break;
	case WebSocket:
		m_qwebsocket.sendTextMessage(cmd);
		break;
	}
}
