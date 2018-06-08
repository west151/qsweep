import QtQuick 2.4

ClientForm {

    buttonDisconnect.onClicked: {
        userInterface.onDisconnectFromHost()
    }

    buttonSendData.onClicked: {
        userInterface.onSendMessageToHost()
    }

    buttonConnect.onClicked: {
        userInterface.host = textEditAddress.text
        userInterface.port = textEditPort.text
        userInterface.onConnectToHost()
    }
}