#pragma once

#include <QObject>
#ifdef HAS_QSERIALPORT
#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#endif
#include <QWebSocket>
#include <QWebSocketServer>
#include <memory>

class Arduino : public QObject
{
	Q_OBJECT

public:
	enum ConnectionMode {
		None,
		Serial,
		WebSocket
	};

private:
	Q_PROPERTY(bool webSocketServer READ hasWebSocketServer WRITE setWebSocketServer NOTIFY webSocketServerChanged)
	Q_PROPERTY(quint16 webSocketServerPort READ webSocketServerPort WRITE setWebSocketServerPort NOTIFY webSocketServerPortChanged)

	Q_PROPERTY(QStringList connectionModes READ connectionModes CONSTANT)
	Q_PROPERTY(QString connectionMode READ connectionMode WRITE setConnectionMode NOTIFY connectionModeChanged)
	Q_PROPERTY(bool connected READ isConnected)

	Q_PROPERTY(QString remoteWebSocketUrl READ remoteWebSocketUrl WRITE setRemoteWebSocketUrl NOTIFY remoteWebSocketUrlChanged)

	Q_PROPERTY(QString serialInterface READ serialInterface WRITE setSerialInterface NOTIFY serialInterfaceChanged)

public:
	explicit Arduino(QObject *parent = 0);
	~Arduino() override;

	Q_INVOKABLE void send(QString cmd);
	Q_INVOKABLE QStringList serialInterfaces() const {
		QStringList result;

#ifdef HAS_QSERIALPORT
		for(const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
			if(info.manufacturer().startsWith("Arduino"))
				result.push_back(info.portName());
		}
#endif

		return result;
	}

	QString connectionMode() const {
		switch(m_connectionMode) {
			case None: return "none";
			case Serial: return "serial";
			case WebSocket: return "websocket";
			default: return "unknown";
		}
	}

	void disconnect() {
		if(m_connected) {
			if(internal_disconnect()) {
				m_connected = false;
				emit disconnected();
			}
		}
	}

	void connect() {
		internal_disconnect();

		switch(m_connectionMode) {
		case None: break;
		case Serial: {
#ifdef HAS_QSERIALPORT

			QStringList interfaces = serialInterfaces();
			if(!interfaces.isEmpty()) {

				// use the first interface if non was selected
				if(m_serialInterface.isEmpty()) {
					m_serialInterface = interfaces[0];
					emit serialInterfaceChanged(m_serialInterface);
				}

				serialPort.setPortName(m_serialInterface);
				serialPort.setBaudRate(QSerialPort::Baud115200);
				serialPort.setDataBits(QSerialPort::Data8);
				serialPort.setParity(QSerialPort::NoParity);
				serialPort.setStopBits(QSerialPort::OneStop);
				serialPort.setFlowControl(QSerialPort::NoFlowControl);
				if (serialPort.open(QIODevice::ReadWrite)) {
					m_connected = true;
					emit connected();
				}
			}
#endif
			break;}
		case WebSocket:
			m_qwebsocket.open(QUrl(m_remoteWebSocketUrl));
			break;
		default:
			break;
		}
	}

	bool hasWebSocketServer() const
	{
		return m_webSocketServer;
	}

	qint16 webSocketServerPort() const
	{
		return m_webSocketServerPort;
	}

	QString remoteWebSocketUrl() const
	{
		return m_remoteWebSocketUrl;
	}

	bool isConnected() const
	{
		return m_connected;
	}

	QString serialInterface() const
	{
		return m_serialInterface;
	}

	QStringList connectionModes() const
	{
		return m_connectionModes;
	}

signals:

	void connected();
	void disconnected();

	void serialInterfaceChanged(QString arg);

	void connectionModeChanged(QString mode);

	void remoteWebSocketUrlChanged(QString url);

	void webSocketServerChanged(bool arg);

	void webSocketServerPortChanged(quint16 arg);

	void messageReceived(QString msg);

	void clientConnected(QUrl url);

public slots:

	void setWebSocketServer(bool arg)
	{
		if(m_webSocketServer == arg)
			return;

		m_webSocketServer = arg;

		internal_handleWebSocketServerState();

		emit webSocketServerChanged(arg);
	}

	void setConnectionMode(QString new_mode) {
		if(new_mode.toLower() == "none") {
			m_connectionMode = None;
		} else if(new_mode.toLower() == "serial") {
			m_connectionMode = Serial;
		} else if(new_mode.toLower() == "websocket") {
			m_connectionMode = WebSocket;
		} else {
			m_connectionMode = None;
		}
		connect();
		emit connectionModeChanged(new_mode);
	}


	void setWebSocketServerPort(quint16 arg)
	{
		if(m_webSocketServerPort == arg)
			return;

		m_webSocketServerPort = arg;

		internal_handleWebSocketServerState();

		emit webSocketServerPortChanged(arg);
	}

	void setSerialInterface(QString arg)
	{
		if (m_serialInterface == arg)
			return;

		m_serialInterface = arg;

		if(m_connectionMode == Serial)
			connect();

		emit serialInterfaceChanged(arg);
	}

	void setRemoteWebSocketUrl(QString arg)
	{
		if (m_remoteWebSocketUrl == arg)
			return;

		m_remoteWebSocketUrl = arg;

		if(m_connectionMode == WebSocket)
			connect();

		emit remoteWebSocketUrlChanged(arg);
	}

	void messageFromClientReceived(QString cmd) {
		send(cmd);
	}

	void onNewWebSocketClientConnection() {
		QWebSocket *pSocket = m_qwebsocketServer->nextPendingConnection();

		QObject::connect(pSocket, &QWebSocket::textMessageReceived, this, &Arduino::messageFromClientReceived);
		QObject::connect(pSocket, &QWebSocket::disconnected, this, &Arduino::onWebSocketClientDisconnected);

		m_clients << pSocket;

		emit clientConnected(pSocket->requestUrl());
	}

	void onWebSocketClientDisconnected() {
		QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
		if (pClient) {
			m_clients.removeAll(pClient);
			pClient->deleteLater();
		}
	}

private:

	void internal_handleWebSocketServerState() {
		stopWebSocketServer();
		if(m_webSocketServer) {
			if (m_qwebsocketServer->listen(QHostAddress::Any, m_webSocketServerPort))
					qDebug() << "Arduino relay listening on port" << m_webSocketServerPort;
		}
	}

	bool internal_disconnect() {
		bool did_disconnect = false;
#ifdef HAS_QSERIALPORT
		if(serialPort.isOpen()) {
			serialPort.close();
			did_disconnect = true;
		}
#endif
		m_qwebsocket.close();
		return did_disconnect;
	}

	void stopWebSocketServer() {
		if(m_qwebsocketServer) {
			m_qwebsocketServer->close();
		}
		qDeleteAll(m_clients.begin(), m_clients.end());
	}

	bool m_connected = false;
	ConnectionMode m_connectionMode = None;
#ifdef HAS_QSERIALPORT
	QSerialPort serialPort;
#endif
	bool m_webSocketServer = false;
	quint16 m_webSocketServerPort = 55777;
	QString m_remoteWebSocketUrl = "ws://192.168.1.61:55777";
	QString m_serialInterface;
	QStringList m_connectionModes;

	std::unique_ptr<QWebSocketServer> m_qwebsocketServer;
	QList<QWebSocket*> m_clients;

	QWebSocket m_qwebsocket;
};
