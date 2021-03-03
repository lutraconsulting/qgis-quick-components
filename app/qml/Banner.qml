

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
import QtQuick 2.7
import QtGraphicalEffects 1.0
import "."

Rectangle {
  property color fontColor: "white"
  property color bgColor: InputStyle.fontColorBright
  property string text: ""
  property string source: ""

  id: banner
  color: banner.bgColor

  signal clicked

  MouseArea {
    anchors.fill: parent
    onClicked: {
      banner.clicked()
    }
  }

  Row {
    anchors.centerIn: parent
    height: banner.height
    spacing: 0

    Item {
      id: iconContainer
      height: banner.height
      width: banner.height

      Image {
        id: icon
        anchors.fill: parent
        anchors.margins: banner.height / 4
        sourceSize.width: width
        sourceSize.height: height
        source: banner.source
        fillMode: Image.PreserveAspectFit
      }

      ColorOverlay {
        anchors.fill: icon
        source: icon
        color: banner.fontColor
      }
    }

    Text {
      id: label
      height: banner.height
      text: banner.text
      color: banner.fontColor
      font.bold: true
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      rightPadding: banner.height / 4
    }
  }
}
