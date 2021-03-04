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
import QtQuick.Dialogs 1.1
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0
import QgsQuick 0.1 as QgsQuick
import "."  // import InputStyle singleton
import "./components"  // import InputStyle singleton
import lc 1.0

Item {
    signal addClicked
    signal cancelClicked
    signal gpsSwitchClicked
    signal manualRecordingClicked
    signal stopRecordingClicked
    signal removePointClicked
    signal close
    signal layerLabelClicked

    property int rowHeight: InputStyle.rowHeightHeader
    property int extraPanelHeight: InputStyle.rowHeightHeader * 0.6
    property int itemSize: rowHeight * 0.8
    property color gpsIndicatorColor: InputStyle.softRed
    property bool pointLayerSelected: true
    property bool manualRecordig: false
    property bool extraPanelVisible: true
    property bool showWarning: false
    property string gpsAccuracyInfo: ""

    property QgsQuick.VectorLayer activeVectorLayer: __activeLayer.vectorLayer
    property string activeLayerName: activeVectorLayer ? activeVectorLayer.name : ""
    property string activeLayerIcon: __loader.loadIconFromLayer( activeVectorLayer )

    id: root
    onClose: visible = false
    focus: true

    //! Provides banner animation also when the toolbar is shown
    onVisibleChanged: {
      notificationBanner.state = visible ? "show" : "fade"
    }

    Keys.onReleased: {
      if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
        event.accepted = true;
        cancelButton.activated()
      }
    }

    Rectangle {
        anchors.fill: parent
        color: InputStyle.clrPanelBackground
        opacity: InputStyle.panelOpacity

        MouseArea {
          anchors.fill: parent
          onClicked: {} // dont do anything, just do not let click event propagate
        }
    }

    Banner {
      id: notificationBanner
      width: parent.width - notificationBanner.anchors.margins * 2
      showWarning: root.showWarning
      text: qsTr("Low GPS position accuracy (%1m)").arg(root.gpsAccuracyInfo)
      anchors.bottom: extraPanel.top
      rowHeight: InputStyle.rowHeight
    }

    SimpleTextWithIcon {
      id: extraPanel
      height: extraPanelHeight
      width: parent.width
      color: InputStyle.fontColorBright
      fontColor: "white"
      fontPixelSize: InputStyle.fontPixelSizeSmall
      visible: extraPanelVisible
      source: root.activeLayerIcon
      text: root.activeLayerName
      onClicked: layerLabelClicked()
    }

    RowLayout {
        height: root.rowHeight
        width: parent.width
        anchors.bottom: parent.bottom

        Item {
            height: parent.height
            Layout.fillWidth: true

            MainPanelButton {
                id: gpsSwitchBtn
                width: root.itemSize
                text: qsTr("GPS")
                imageSource: "ic_gps_fixed_48px.svg"
                onActivated: root.gpsSwitchClicked()
                onActivatedOnHold: root.manualRecordingClicked()

                RoundIndicator {
                    width: parent.height/4
                    height: width
                    anchors.right: parent.right
                    anchors.top: parent.top
                    color: gpsIndicatorColor
                }
            }
        }

        Item {
            Layout.fillWidth: true
            height: parent.height
            visible: root.pointLayerSelected ? false : true

            MainPanelButton {
                id: removePointButton
                width: root.itemSize
                text: qsTr("Undo")
                imageSource: "undo.svg"
                enabled: manualRecordig

                onActivated: root.removePointClicked()
            }
        }

        Item {
            height: parent.height
            Layout.fillWidth: true

            MainPanelButton {
                id: addButton
                width: root.itemSize
                text: qsTr("Add Point")
                imageSource: InputStyle.plusIcon
                enabled: manualRecordig

                onActivated: root.addClicked()
            }
        }

        Item {
            Layout.fillWidth: true
            height: parent.height
            visible: root.pointLayerSelected ? false : true

            MainPanelButton {
                id: finishButton
                width: root.itemSize
                text: qsTr("Done")
                imageSource: "check.svg"

                onActivated: root.stopRecordingClicked()
            }
        }

        Item {
            height: parent.height
            Layout.fillWidth: true

            MainPanelButton {
                id: cancelButton
                width: root.itemSize
                text: qsTr("Cancel")
                imageSource: "no.svg"

                onActivated: root.cancelClicked()
            }
        }
    }

}
