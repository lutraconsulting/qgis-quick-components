

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import "."
import "./components"

Rectangle {
  property color fontColor: InputStyle.fontColor
  property color bgColor: InputStyle.softOrange
  property string text: ""
  property string source: InputStyle.exclamationIcon
  property real padding: InputStyle.innerFieldMargin
  property real rowHeight: InputStyle.rowHeight
  property bool showWarning: true
  property string link: "https://help.inputapp.io/" // TODO direct link to the GPDS accuracy section

  id: banner
  color: banner.bgColor
  radius: InputStyle.cornerRadius
  x: padding
  height: childrenRect.height
  anchors {
    margins: padding
  }

  states: [
      State { name: "show"; when: banner.showWarning;
          PropertyChanges {   target: banner; opacity: 1.0    }
      },
      State { name: "fade"; when: !banner.showWarning;
          PropertyChanges {   target: banner; opacity: 0.0    }
      }
  ]
  transitions: Transition {
      NumberAnimation { property: "opacity"; duration: 500}
  }

  ColumnLayout {
    width: banner.width
    spacing: 0

    SimpleTextWithIcon {
      height: banner.rowHeight
      Layout.fillWidth: true
      color: "transparent"
      fontColor: banner.fontColor
      source: banner.source
      text: banner.text
    }

    Text {
      Layout.fillWidth: true
      color: banner.fontColor
      bottomPadding: InputStyle.innerFieldMargin
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      font.pixelSize: InputStyle.fontPixelSizeSmall
      wrapMode: Text.WordWrap
      textFormat: Text.RichText
      text: "<style>a:link { color: " + InputStyle.highlightColor
            + "; text-decoration: underline; }</style>" +
            qsTr("More information <a href='%1'>here</a>.").arg(banner.link)

      onLinkActivated: Qt.openUrlExternally(link)
    }
  }

}
