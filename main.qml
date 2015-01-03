import QtQuick 2.4
import QtQuick.Window 2.2
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.3
import Arduino 1.0

ApplicationWindow {
	id: window
	title: qsTr("Arduino Qt")
	width: 640
	height: 480
	visible: true

	property real last_x_pos: 0
	property real last_y_pos: 0

	Arduino {
		id: arduino
	}

	property real distance_between_servos_mm: 250
	property real rod_length_mm: 350

	property real rod_0_0_deg: 16800
	property real rod_0_90_deg: 31960

	property real rod_1_0_deg: 32900
	property real rod_1_90_deg: 18250

	Rectangle {
		id: viewport
		anchors.fill: parent
		color: "red"
		MultiPointTouchArea {
			anchors.fill: parent
			touchPoints: [
				TouchPoint {
					id: point1
					onXChanged: updatePos();
					onYChanged: updatePos();
				}
			]
		}

		Rectangle {
			width: 30; height: 30
			radius: 30
			color: "black"
			x: point1.x - width/2.0
			y: point1.y - height/2.0
		}
	}

	RowLayout {
		Text {
			text: "Config"
			color: "yellow"
			font.pointSize: 40
			MouseArea {
				anchors.fill: parent
				onClicked: {
					settings.visible = true;
				}
			}
		}
		Text {
			id: pos
			text: "x = " + Math.round(last_x_pos) + "mm  y = " + Math.round(last_y_pos) + "mm"
		}
	}

	Rectangle {
		id: settings
		visible: false
		anchors.fill: parent
		color: "gray"
		opacity: 0.98

		GridLayout {
			//flow: GridLayout.TopToBottom
			anchors {
				top: parent.top
				left: parent.left
				right: parent.right
			}
			anchors.margins: 4
			columns: 2

			Label {
				text: "Configuration"
				font.pixelSize: 22
				Layout.columnSpan: 2
				Layout.alignment: Qt.AlignHCenter
			}

			Label {
				text: "Connection mode"
			}
			ComboBox {
				id: connectionMode
				Layout.fillWidth: true
				model: arduino.connectionModes
				editText: arduino.connectionMode
				onCurrentTextChanged: arduino.connectionMode = currentText
			}

			Label {
				text: "Enable websocket server"
			}
			Switch {
				id: websocket_server
				checked: arduino.webSocketServer
				onCheckedChanged: arduino.webSocketServer = checked
			}

			Label {
				text: "Websocket server port"
			}
			SpinBox {
				id: spin_box_websocket_server_port
				Layout.fillWidth: true
				value: arduino.webSocketServerPort
				onValueChanged: arduino.webSocketServerPort = value
				maximumValue: 65535
			}

			Label {
				text: "Serial interface"
			}
			ComboBox {
				id: serialInterface
				Layout.fillWidth: true
			}

			Label {
				text: "Remote websocket url"
			}
			TextField {
				id: textfield_remote_websocket_url
				Layout.fillWidth: true
				text: arduino.remoteWebSocketUrl
				onTextChanged: arduino.remoteWebSocketUrl = text
			}
		}

		Button {
			text: "close"
			anchors.right: settings.right
			anchors.bottom: settings.bottom
			onClicked: settings.visible = false;
		}

		onVisibleChanged: {
			serialInterface.model = arduino.serialInterfaces();
		}
	}

	function updatePos() {
		var x_pos = (1.0 - point1.x / viewport.width) * 250;
		var y_pos = (1.0 - point1.y / viewport.height) * 200 + 30;

		if(last_x_pos == x_pos && last_y_pos == y_pos)
			return;

		last_x_pos = x_pos;
		last_y_pos = y_pos;

		setPos(x_pos, y_pos);
	}

	function setPos(x, y) {
		var alpha1 = Math.atan(y/x);
		var alpha2 = Math.atan(y/(distance_between_servos_mm - x));
		var alpha1_factor = (alpha1/(Math.PI/2));
		var alpha2_factor = (alpha2/(Math.PI/2));
		var servo1 = Math.round((rod_0_90_deg-rod_0_0_deg)*alpha1_factor + rod_0_0_deg);
		var servo2 = Math.round((rod_1_90_deg-rod_1_0_deg)*alpha2_factor + rod_1_0_deg);
		var command = "#0 p" + servo1 + " #2 p" + servo2;
		arduino.send(command);
	}
}
