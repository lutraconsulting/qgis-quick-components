/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

import QtQuick 2.6
import QtQuick.Controls 2.12
import QgsQuick 0.1 as QgsQuick

import "../" // import InputStyle singleton

QgsQuick.SwitchComponent {
  id: switchComp

  implicitHeight: parent.height * 0.45
  anchors.verticalCenter: parent.verticalCenter
  anchors.right: parent.right
  anchors.rightMargin: InputStyle.panelMargin

  bgndColorActive: InputStyle.softGreen
  bgndColorInactive: InputStyle.panelBackgroundDark
}
