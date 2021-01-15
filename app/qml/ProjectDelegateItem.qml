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
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import lc 1.0
import QgsQuick 0.1 as QgsQuick
import "."  // import InputStyle singleton
import "./components"

/*
  |----------------------------------------------------------------------|
  |   ProjectName                               |    1st     |    2nd    |
  |                                             |   action   |   action  |
  |   ProjectDescription                        |            |           |
  |----------------------------------------------------------------------|

  1st action = status action
  2nd action = menu action
 */

Rectangle {
    id: itemContainer
    color: itemContainer.highlight ? InputStyle.fontColorBright : itemContainer.primaryColor

    property color primaryColor: InputStyle.clrPanelMain
    property color secondaryColor: InputStyle.fontColor
    property int cellWidth: width
    property int cellHeight: height
    property real iconSize: height/2
    property real borderWidth: 1 * QgsQuick.Utils.dp
    property bool highlight: false
    property bool pending: false
    property string statusIconSource: "more_menu.svg"
    property string projectFullName   // <namespace>/<projectName>
    property bool disabled: false
    property real itemMargin: InputStyle.panelMargin
    property real progressValue: 0
    property bool isAdditional: false
    property string additionalIconSource: ""

    signal itemClicked()
    signal menuClicked()
    signal delegateButtonClicked()
    signal additionalIconClicked()
    signal updateFinished()


//    property Input.Project project: Input.InvalidProject

    // temp properties until we get Project instance
    property string projectName_future
    property string projectDescription_future
    property bool hasMenu_future

    signal projectClicked_future()
    signal statusClicked_future()
    signal menuClicked_future()

    states: [
      State {
        name: "idle"
        PropertyChanges {
          target: object

        }
      },
      State {
        name: "pending"
        PropertyChanges {
          target: object

        }
      },
      State {
        name: "invalid"
        PropertyChanges {
          target: object

        }
      },
      State {
        name: "fetching"
        PropertyChanges {
          target: object

        }
      }
    ]


    property var endBusyIndicator: function endBusyIndicator() {
      loadingProjectStatusIndicator.running = false
      iconTEMPcontainer.visible = true
    }

    property var runBusyIndicator: function runBusyIndicator() {
      loadingProjectStatusIndicator.running = true
      iconTEMPcontainer.visible = false
    }

    MouseArea {
        anchors.fill: parent
        onClicked: if (!disabled) itemClicked()
    }

    Rectangle {
        id: backgroundRect
        visible: disabled
        width: parent.width
        height: parent.height
        color: InputStyle.panelBackgroundDark
    }

    Item {
        width: parent.width
        height: parent.height
        visible: !itemContainer.isAdditional

        RowLayout {
            id: row
            anchors.fill: parent
            anchors.leftMargin: itemContainer.itemMargin
            spacing: InputStyle.panelMargin

            Item {
                id: textContainer
                height: itemContainer.cellHeight
                Layout.fillWidth: true

                Text {
                    id: mainText
                    text: __inputUtils.formatProjectName(itemContainer.projectFullName)
                    height: textContainer.height/2
                    width: textContainer.width
                    font.pixelSize: InputStyle.fontPixelSizeNormal
                    color: itemContainer.highlight? itemContainer.primaryColor : itemContainer.secondaryColor
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignBottom
                    elide: Text.ElideRight
                }

                Text {
                    id: secondaryText
                    visible: !pending
                    height: textContainer.height/2
                    text: projectInfo ? projectInfo : ""
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.top: mainText.bottom
                    font.pixelSize: InputStyle.fontPixelSizeSmall
                    color: itemContainer.highlight ? itemContainer.primaryColor : InputStyle.panelBackgroundDark
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignTop
                    elide: Text.ElideRight
                }

                ProgressBar {
                  property real itemHeight: InputStyle.fontPixelSizeSmall

                  id: progressBar
                  anchors.top: mainText.bottom
                  height: InputStyle.fontPixelSizeSmall
                  width: secondaryText.width
                  value: progressValue
                  visible: pending

                  background: Rectangle {
                    implicitWidth: parent.width
                    implicitHeight: progressBar.itemHeight
                    color: InputStyle.panelBackgroundLight
                  }

                  contentItem: Item {
                    implicitWidth: parent.width
                    implicitHeight: progressBar.itemHeight

                    Rectangle {
                      width: progressBar.visualPosition * parent.width
                      height: parent.height
                      color: InputStyle.fontColor
                    }
                  }
                }
            }

            Item {
              id: additionalIconContainer
              height: itemContainer.cellHeight
              width: height
              visible: model.isMerginProject ? model.isMerginProject : false

              MouseArea {
                anchors.fill: parent
                onClicked: additionalIconClicked()
              }

              Item {
                id: busyTEMPcontainer
                visible: true
                anchors.fill: parent

                BusyIndicator {
                  id: loadingProjectStatusIndicator
                  anchors.horizontalCenter: parent.horizontalCenter
                  anchors.verticalCenter: parent.verticalCenter
                  running: false
                  implicitHeight: 80 * QgsQuick.Utils.dp // 80 just to be visible on PC, 40 is fine for mobile
                  implicitWidth: implicitHeight
                }
              }

              Item {
                id: iconTEMPcontainer
                anchors.fill: parent
                visible: false

                Image {
                  id: additionalIcon
                  anchors.centerIn: parent
                  source: additionalIconSource
                  height: itemContainer.iconSize
                  width: height
                  sourceSize.width: width
                  sourceSize.height: height
                  fillMode: Image.PreserveAspectFit
                }

                ColorOverlay {
                  anchors.fill: additionalIcon
                  source: additionalIcon
                  color: itemContainer.highlight ? itemContainer.primaryColor : itemContainer.secondaryColor
                }
              }
            }

            Item {
                id: statusContainer
                height: itemContainer.cellHeight
                width: height
                y: 0

                MouseArea {
                    anchors.fill: parent
                    onClicked:menuClicked()
                }

                Image {
                    id: statusIcon
                    anchors.centerIn: parent
                    source: statusIconSource
                    height: itemContainer.iconSize
                    width: height
                    sourceSize.width: width
                    sourceSize.height: height
                    fillMode: Image.PreserveAspectFit
                }

                ColorOverlay {
                    anchors.fill: statusIcon
                    source: statusIcon
                    color: itemContainer.highlight ? itemContainer.primaryColor : itemContainer.secondaryColor
                }
            }
        }

        Rectangle {
            id: borderLine
            color: InputStyle.panelBackground2
            width: itemContainer.width
            height: itemContainer.borderWidth
            anchors.bottom: parent.bottom
        }
    }

    // Additional item
    DelegateButton {
      visible: itemContainer.isAdditional
      width: itemContainer.width
      height: itemContainer.height
      text: qsTr("Fetch more")

      onClicked: itemContainer.delegateButtonClicked()
    }

}
