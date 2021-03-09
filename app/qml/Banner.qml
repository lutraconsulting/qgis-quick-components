

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
  property color fontColor: "black"
  property color linkColor: fontColor
  property color bgColor: InputStyle.warningBannerColor
  property string text: ""
  property string link: ""
  property string source: InputStyle.exclamationIcon
  property real padding: InputStyle.innerFieldMargin
  property bool showWarning: false

  id: banner
  color: banner.bgColor
  radius: InputStyle.cornerRadius
  x: padding
  y: padding
  height: childrenRect.height
  anchors {
    margins: padding
  }
  state:"fade"

  states: [
    State { name: "show"; when: banner.showWarning;
      PropertyChanges { target: banner; opacity: 1.0 }
    },
    State { name: "fade"; when: !banner.showWarning;
      PropertyChanges { target: banner; opacity: 0.0 }
    }
  ]

  transitions: Transition {
    NumberAnimation { property: "opacity"; duration: 500 }
  }

  layer.enabled: true
  layer.effect: Shadow { verticalOffset: 0 }

  ColumnLayout {
    width: banner.width
    spacing: 0
    anchors.centerIn: banner

    TextWithIcon {
      id: content
      Layout.fillWidth: true
      fontColor: banner.fontColor
      iconColor: banner.fontColor
      bgColor: banner.bgColor
      source: banner.source
      textItem.font.bold: true
      textItem.text: "<style>a:link { color: " + banner.linkColor
            + "; text-decoration: underline; }</style>" +
            qsTr("%1 <br><a href='%2'>Learn more</a>").arg(banner.text).arg(banner.link)

      onLinkActivated: Qt.openUrlExternally(link)
    }
  }

}
