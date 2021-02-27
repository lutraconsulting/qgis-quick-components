/***************************************************************************
 qgsquickicontextitem.qml
  --------------------------------------
  Date                 : 2019
  Copyright            : (C) 2019 by Viktor Sklencar
  Email                : viktor.sklencar@lutraconsulting.co.uk
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.5
import QtGraphicalEffects 1.0
import QtQuick.Layouts 1.3
import QgsQuick 0.1 as QgsQuick

Item {
  property real iconSize
  property color fontColor
  property real fontPointSize: root.iconSize * 0.75
  property string iconSource
  property string labelText

  id: root
  width: text.paintedWidth
  height: root.iconSize + text.paintedHeight

  ColumnLayout {
    anchors.fill: parent
    spacing: 20 * QgsQuick.Utils.dp

    Item {
      id: iconContainer
      implicitHeight: 15 * QgsQuick.Utils.dp
      Layout.alignment: Qt.AlignHCenter

      Image {
        id: icon
        x: -width/2
        source: root.iconSource
        width: root.iconSize
        height: root.iconSize
        sourceSize.width: width
        sourceSize.height: height
        fillMode: Image.PreserveAspectFit
      }

      ColorOverlay {
        anchors.fill: icon
        source: icon
        color: root.fontColor
      }
    }

    Item {
      id: textContainer
      implicitHeight: 15 * QgsQuick.Utils.dp


      Text {
        id: text
        height: root.iconSize
        text: root.labelText
        font.pointSize: root.fontPointSize
        color: root.fontColor
        Layout.alignment: Qt.AlignHCenter
      }
    }
  }
}
