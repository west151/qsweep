import QtQuick 2.4

SweepMessageLogForm {

    listViewMessageLog.model: messageLogModel
    listViewMessageLog.delegate: Text {
            id: name
            text: message_text
        }

//    Component {
//        id: viewInfoDelegate
//        Item {
//            id: mainItem
//            x: 5
//            width: listView.width
//            height: idTextInfo.height

//            Row {
//                id: idRow
//                anchors.verticalCenter: parent.verticalCenter

//                Text {
//                    id: idTextInfo
//                    width: mainItem.width
//                    text: "Serial Numbers: " + info_serial_numbers + "\n"
//                          + "Board ID: " + info_board_id + "\n"
//                          + "Firmware Version: " + info_firmware_version + "\n"
//                          + "Part ID Number: " + info_part_id_number + "\n"
//                          + "Libhackrf Version: " + info_hackrf_version
//                    wrapMode: Text.WordWrap
//                    anchors.verticalCenter: parent.verticalCenter
//                    font.pointSize: 13

//                }

//                spacing: 10
//            }
//        }
//    }
}
