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
import "."  // import InputStyle singleton
import "./components"

Page {

    property real rowHeight: InputStyle.rowHeight
    property string defaultLayer: __appSettings.defaultLayer
    property color gpsIndicatorColor: InputStyle.softRed

    id: settingsPanel
    visible: false
    padding: 0
    focus: true

    background: Rectangle {
        anchors.fill: parent
        color: InputStyle.clrPanelMain
    }

    Keys.onReleased: {
      if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
        event.accepted = true;

        if (aboutPanel.visible) { // hide about panel
          aboutPanel.visible = false;
        }
        else if (logPanel.visible) {
          logPanel.visible = false
        }
        else if (settingsPanel.visible) {
          settingsPanel.visible = false;
        }
      }
    }

    PanelHeader {
        id: header
        height: settingsPanel.rowHeight
        width: parent.width
        color: InputStyle.clrPanelMain
        rowHeight: InputStyle.rowHeightHeader
        titleText: qsTr("Settings")

        onBack: settingsPanel.visible = false;
    }

    Rectangle {
        id: settingsList
        anchors.top: header.bottom
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 0
        color: InputStyle.panelBackgroundDark
        width: parent.width
        height: parent.height

        Column {
            id: settingListContent
            anchors.fill: parent
            spacing: 1

             // Header "GPS"
            PanelItem {
                color: InputStyle.panelBackgroundLight
                text: qsTr("GPS")
                bold: true
            }

            PanelItem {
                height: settingsPanel.rowHeight
                width: parent.width
                color: InputStyle.clrPanelMain
                text: qsTr("Follow GPS with map")

                SettingsSwitch {
                  id: autoCenterMapSwitch

                  checked: __appSettings.autoCenterMapChecked
                  onCheckedChanged: __appSettings.autoCenterMapChecked = checked
                }

                MouseArea {
                  anchors.fill: parent
                  onClicked: autoCenterMapSwitch.toggle()
                }
            }

            PanelItem {
                height: settingsPanel.rowHeight
                width: parent.width
                color: InputStyle.clrPanelMain
                text: qsTr("GPS accuracy")

                Row {
                    id: widget
                    property real indicatorSize: {
                        var size = height/3
                        size % 2 === 0 ? size : size + 1
                    }
                    width: indicatorSize * 4
                    anchors.top: parent.top
                    anchors.topMargin: 0
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 0
                    anchors.right: parent.right
                    anchors.rightMargin: InputStyle.panelMargin
                    spacing: InputStyle.panelSpacing

                    RoundIndicator {
                        width: widget.indicatorSize
                        height: width
                        anchors.margins: height/3
                        color: InputStyle.panelBackgroundLight
                        anchors.verticalCenter: parent.verticalCenter
                        visible: false // disabled due no manual GPS on/off support
                    }

                    RoundIndicator {
                        width: widget.indicatorSize
                        height: width
                        color: InputStyle.softRed
                        isActive: color === gpsIndicatorColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    RoundIndicator {
                        width: widget.indicatorSize
                        height: width
                        color: InputStyle.softOrange
                        isActive: color === gpsIndicatorColor
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    RoundIndicator {
                        width: widget.indicatorSize
                        height: width
                        color: InputStyle.softGreen
                        isActive: color === gpsIndicatorColor
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            PanelItem {
                height: settingsPanel.rowHeight
                width: parent.width
                text: qsTr("Accuracy threshold")

                NumberSpin {
                    value: __appSettings.gpsAccuracyTolerance
                    suffix: " m"
                    onValueChanged: __appSettings.gpsAccuracyTolerance = value
                    height: InputStyle.fontPixelSizeNormal
                    rowHeight: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                    width: height * 6
                    anchors.right: parent.right
                    anchors.rightMargin: InputStyle.panelMargin
                }
            }

            PanelItem {
                height: settingsPanel.rowHeight
                width: parent.width
                text: qsTr("Accuracy warning")

                SettingsSwitch {
                  id: accuracyWarningSwitch

                  checked: __appSettings.gpsAccuracyWarning
                  onCheckedChanged: __appSettings.gpsAccuracyWarning = checked
                }

                MouseArea {
                  anchors.fill: parent
                  onClicked: accuracyWarningSwitch.toggle()
                }
            }

            // Header "Recording"
            PanelItem {
                color: InputStyle.panelBackgroundLight
                text: qsTr("Recording")
                bold: true
            }

            PanelItem {
                height: settingsPanel.rowHeight
                width: parent.width
                text: qsTr("Line rec. interval")

                NumberSpin {
                    id: spinRecordingInterval
                    value: __appSettings.lineRecordingInterval
                    minValue: 1
                    maxValue: 30
                    suffix: " s"
                    onValueChanged: __appSettings.lineRecordingInterval = spinRecordingInterval.value
                    height: InputStyle.fontPixelSizeNormal
                    rowHeight: parent.height
                    anchors.verticalCenter: parent.verticalCenter
                    width: height * 6
                    anchors.right: parent.right
                    anchors.rightMargin: InputStyle.panelMargin
                }
            }


            PanelItem {
                height: settingsPanel.rowHeight
                width: parent.width
                color: InputStyle.clrPanelMain
                text: qsTr("Reuse last value option")

                SettingsSwitch {
                  id: rememberValuesSwitch

                  checked: __appSettings.reuseLastEnteredValues
                  onCheckedChanged: __appSettings.reuseLastEnteredValues = checked
                }

                MouseArea {
                  anchors.fill: parent
                  onClicked: rememberValuesSwitch.toggle()
                }
            }

            // Delimeter
            PanelItem {
                color: InputStyle.panelBackgroundLight
                text: ""
                height: settingsPanel.rowHeight/3
            }

            // App info
           PanelItem {
               text: qsTr("About")
               MouseArea {
                   anchors.fill: parent
                   onClicked: aboutPanel.visible = true
               }
           }

           // Help
          PanelItem {
              text: qsTr("Help")
              MouseArea {
                  anchors.fill: parent
                  onClicked: Qt.openUrlExternally("https://help.inputapp.io/");
              }
          }

          // Help
          PanelItem {
             text: qsTr("Privacy Policy")
             MouseArea {
                 anchors.fill: parent
                 onClicked: Qt.openUrlExternally(__inputHelp.privacyPolicyLink);
             }
          }

           // Debug/Logging
          PanelItem {
              text: qsTr("Diagnostic Log")
              MouseArea {
                  anchors.fill: parent
                  onClicked: {
                    logPanel.text = __inputHelp.fullLog(true, 200000) //0.2MB
                    logPanel.visible = true
                  }
              }
          }
        }
    }

    AboutPanel {
        id: aboutPanel
        anchors.fill: parent
        visible: false
    }

    LogPanel {
        id: logPanel
        anchors.fill: parent
        visible: false
    }
}
