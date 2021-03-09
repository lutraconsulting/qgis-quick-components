/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0
import "."  // import InputStyle singleton

Row {
  spacing: 0
  id: root
  property real fieldHeight: InputStyle.rowHeight
  property string source: ""
  property string text: ""
  property color bgColor: "white"
  property color fontColor: InputStyle.fontColor
  property color iconColor: InputStyle.fontColor

  property alias textItem: textItem

  signal linkActivated(string link)

  Rectangle {
    id: iconContainer
    height: fieldHeight
    width: fieldHeight
    color: root.bgColor

    Image {
      anchors.margins: (fieldHeight / 4)
      id: icon
      height: fieldHeight
      width: fieldHeight
      anchors.fill: parent
      source: root.source
      sourceSize.width: width
      sourceSize.height: height
      fillMode: Image.PreserveAspectFit
    }

    ColorOverlay {
      anchors.fill: icon
      source: icon
      color: root.iconColor
    }
  }

  Text {
    id: textItem
    width: parent.width - iconContainer.width
    height: fieldHeight
    verticalAlignment: Text.AlignVCenter
    font.pixelSize: InputStyle.fontPixelSizeNormal
    onLinkActivated: root.linkActivated(link)
    color: root.fontColor
    text: "<style>a:link { color: " + InputStyle.highlightColor
          + "; text-decoration: underline; }</style>" + root.text
    textFormat: Text.RichText
    wrapMode: Text.WordWrap
  }
}
