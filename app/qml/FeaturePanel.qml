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
import QtQuick.Dialogs 1.2
import QgsQuick 0.1 as QgsQuick
import "."  // import InputStyle singleton

Drawer {

    property var mapSettings
    property var project
    property real panelHeight
    property real previewHeight
    property bool isReadOnly

    signal editGeometryClicked()
    signal panelClosed()

    property alias formState: featureForm.state
    property alias feature: attributeModel.featureLayerPair
    property alias currentAttributeModel: attributeModel

    function saveFeatureGeom() {
        featureForm.save()
    }

    function reload() {
      // order matters!
      attributeFormModel.forceClean();
      attributeModel.forceClean();
    }

    function onAboutToClose() {
      if (featureForm.hasAnyChanges())  {
        saveChangesDialog.open()
      } else {
        featurePanel.visible = false
      }
    }

    id: featurePanel
    visible: false
    modal: false
    interactive: previewPanel.visible
    dragMargin: 0 // prevents opening the drawer by dragging.
    edge: Qt.BottomEdge
    closePolicy: Popup.CloseOnEscape // prevents the drawer closing while moving canvas

    Behavior on height {
        PropertyAnimation { properties: "height"; easing.type: Easing.InOutQuad }
    }

    Item {
      id: backHandler
      focus: true
      Keys.onReleased: {
        if (event.key === Qt.Key_Back || event.key === Qt.Key_Escape) {
          featurePanel.onAboutToClose()
        }
      }
    }

    background: Rectangle {
        id: stateManager
        color: InputStyle.clrPanelMain

        state: "preview"
        states: [
            State {
                name: "preview"
                PropertyChanges { target: featurePanel; height: featurePanel.previewHeight }
                PropertyChanges { target: formContainer; visible: false }
                PropertyChanges { target: previewPanel; visible: true }
            }
            ,State {
                name: "form"
                PropertyChanges { target: featurePanel; height: featurePanel.panelHeight }
                PropertyChanges { target: formContainer; visible: true }
                PropertyChanges { target: previewPanel; visible: false }
            }
        ]

    }

    onClosed: {
        stateManager.state = "preview"
    }

    function show_panel(feature, formState, panelState) {
        featurePanel.feature = feature
        featurePanel.formState = formState
        featurePanel.visible = true
        featurePanel.isReadOnly = feature.layer.readOnly
        backHandler.focus = true

        if (panelState === "preview") {
            previewPanel.title = __loader.featureTitle(feature)
            previewPanel.mapTipType = __loader.mapTipType(feature)
            previewPanel.mapTipHtml = __loader.mapTipHtml(feature)
            previewPanel.mapTipImage = __loader.mapTipImage(feature)
            previewPanel.previewFields = __loader.mapTipFields(feature)
        }
        stateManager.state = panelState
    }

    PreviewPanel {
      id: previewPanel
      model: featureForm.model
      height: featurePanel.previewHeight
      width: parent.width
      visible: false
      isReadOnly: featurePanel.isReadOnly
      x: 0
      y: 0
      onContentClicked: {
          stateManager.state = "form"
      }

      onEditClicked: {
          stateManager.state = "form"
          featurePanel.formState = "Edit"
      }
    }


    Item {
        id: formContainer
        width: featurePanel.width
        height: featurePanel.height
        visible: false

        PanelHeader {
            id: header
            height: InputStyle.rowHeightHeader
            width: parent.width
            color: InputStyle.clrPanelMain
            fontBtnColor: InputStyle.highlightColor
            rowHeight: InputStyle.rowHeightHeader
            titleText: featurePanel.formState === "Edit" ? qsTr("Edit Feature") : qsTr("Feature")
            backIconVisible: false
            backTextVisible: true

            onBack: {
              featurePanel.close()
            }

            Text {
                id: saveButtonText
                text: qsTr("Save")
                visible: featureForm.state === "Edit" || featureForm.state === "Add"
                enabled: featureForm.model.constraintsHardValid
                color: enabled ? InputStyle.highlightColor : "red"
                font.pixelSize: InputStyle.fontPixelSizeNormal
                height: header.rowHeight
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.top: parent.top
                anchors.rightMargin: InputStyle.panelMargin // same as back button

                MouseArea {
                  anchors.fill: parent
                  onClicked: featureForm.save()
                }
            }

        }

        // TODO currently disabled since supporting photos is not yet implemented
        Rectangle {
            id: photoContainer
            height: 0
            visible: false
            width: parent.width
            anchors.top: header.bottom
            color: InputStyle.panelBackground2

            Text {
                id: backButtonText
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No photos added.")
                color: InputStyle.clrPanelMain
                font.pixelSize: InputStyle.fontPixelSizeNormal
            }
        }

        QgsQuick.FeatureForm {
            id: featureForm
            visible: true

            width: parent.width
            height: parent.height - header.height - photoContainer.height - toolbar.height
            anchors.top: photoContainer.bottom
            anchors.bottom: toolbar.top
            externalResourceHandler: externalResourceBundle.handler
            importDataHandler: codeReaderHandler.handler
            toolbarVisible: false
            allowRememberAttribute: __appSettings.reuseLastEnteredValues
            style: QgsQuick.FeatureFormStyling {
                property color backgroundColor: "white"
                property real backgroundOpacity: 1
                property real titleLabelPointSize: 16

                property QtObject group: QtObject {
                  property color backgroundColor: InputStyle.panelBackgroundLight
                  property color marginColor: InputStyle.panelBackgroundDark
                  property real leftMargin: 0 * QgsQuick.Utils.dp
                  property real rightMargin: 0 * QgsQuick.Utils.dp
                  property real topMargin: 1 * QgsQuick.Utils.dp
                  property real bottomMargin: 1 * QgsQuick.Utils.dp
                  property real height: 64 * QgsQuick.Utils.dp
                  property color fontColor: InputStyle.fontColor
                  property int spacing: 10 * QgsQuick.Utils.dp
                  property int fontPixelSize: 24 * QgsQuick.Utils.dp
                }

                property QtObject tabs: QtObject {
                  property color normalColor: InputStyle.fontColor
                  property color activeColor: InputStyle.fontColor
                  property color disabledColor: InputStyle.fontColor
                  property color backgroundColor: InputStyle.panelBackgroundDark
                  property color normalBackgroundColor: InputStyle.panelBackgroundLight
                  property color activeBackgroundColor: "#FFFFFF"
                  property color disabledBackgroundColor: InputStyle.panelBackgroundDark
                  property real height: 48 * QgsQuick.Utils.dp
                  property real buttonHeight: height * 0.8
                  property real spacing: 5 * QgsQuick.Utils.dp
                  property int tabLabelPointSize: 12
                }

                property QtObject constraint: QtObject {
                  property color validColor: InputStyle.labelColor
                  property color invalidColor: "#c0392b"
                  property color descriptionColor: "#e67e22"
                }

                property QtObject toolbutton: QtObject {
                  property color backgroundColor: "transparent"
                  property color backgroundColorInvalid: "#bdc3c7"
                  property color activeButtonColor: InputStyle.activeButtonColor
                  property real size: 80 * QgsQuick.Utils.dp
                }

                  property QtObject fields: QtObject {
                    property color backgroundColor: InputStyle.panelBackgroundLight
                    property color backgroundColorInactive: "grey"
                    property color fontColor: InputStyle.fontColor
                    property color activeColor: InputStyle.fontColor
                    property color attentionColor: "#aa0000"
                    property color normalColor: InputStyle.panelBackgroundLight
                    property real cornerRadius: 8 * QgsQuick.Utils.dp
                    property real height: 54 * QgsQuick.Utils.dp
                    property int fontPixelSize: 22 * QgsQuick.Utils.dp
                    property real sideMargin: 10 * QgsQuick.Utils.dp
                    property real outerMargin: 20 * QgsQuick.Utils.dp
                    property int fontPointSize: 15
                    property int labelPointSize: 12
                  }

                property QtObject icons: QtObject {
                  property var camera: InputStyle.cameraIcon
                  property var remove: InputStyle.removeIcon
                  property var gallery:InputStyle.galleryIcon
                  property var brokenImage: QgsQuick.Utils.getThemeIcon("ic_broken_image_black")
                  property var notAvailable: QgsQuick.Utils.getThemeIcon("ic_photo_notavailable_white")
                  property var today: QgsQuick.Utils.getThemeIcon("ic_today")
                  property var back: InputStyle.backIcon
                  property var combobox: InputStyle.comboboxIcon
                  property var valueRelationMore: InputStyle.valueRelationIcon
                  property var importData: InputStyle.qrCodeIcon
                }

              property QtObject checkboxComponent: QtObject {
                property color baseColor: InputStyle.panelBackgroundDarker
                }
              }

            model: QgsQuick.AttributeFormModel {
                id: attributeFormModel
                attributeModel: QgsQuick.AttributeModel {
                    id: attributeModel
                }
            }

            project: featurePanel.project
            onSaved: {
                featurePanel.panelClosed()
                featurePanel.visible = false
            }
            onCanceled: {
              featurePanel.panelClosed()
              featurePanel.visible = false
            }

            onStateChanged: {
                toolbar.state = featureForm.state
            }

            onNotify: __inputUtils.showNotificationRequested(message)

            customWidgetCallback: valueRelationWidget.handler
        }

        FeatureToolbar {
            id: toolbar
            width: parent.width
            height: InputStyle.rowHeightHeader
            y: parent.height - height
            state: featurePanel.formState
            visible: !featurePanel.isReadOnly
            isFeaturePoint: featurePanel.feature.layer && digitizing.hasPointGeometry(featurePanel.feature.layer)

            onEditClicked: featureForm.state = "Edit"
            onDeleteClicked: deleteDialog.visible = true
            onEditGeometryClicked: {
                featurePanel.editGeometryClicked()
            }
        }


        MessageDialog {
          id: deleteDialog
          visible: false
          title: qsTr( "Delete feature" )
          text: qsTr( "Are you sure you want to delete this feature?" )
          icon: StandardIcon.Warning
          standardButtons: StandardButton.Ok | StandardButton.Cancel

          //! Using onButtonClicked instead of onAccepted,onRejected which have been called twice
          onButtonClicked: {
              if (clickedButton === StandardButton.Ok) {
                featureForm.model.attributeModel.deleteFeature()
                visible = false
                featureForm.canceled()
              }
              else if (clickedButton === StandardButton.Cancel) {
                visible = false
              }
          }
        }

        MessageDialog {
          id: saveChangesDialog
          visible: false
          title: qsTr( "Unsaved changes" )
          text: qsTr( "Do you want to save changes?" )
          icon: StandardIcon.Warning
          standardButtons: StandardButton.Yes | StandardButton.No | StandardButton.Cancel

          //! Using onButtonClicked instead of onAccepted,onRejected which have been called twice
          onButtonClicked: {
              if (clickedButton === StandardButton.Yes) {
                featureForm.save()
              }
              else if (clickedButton === StandardButton.No) {
                featureForm.canceled()
              }
              else if (clickedButton === StandardButton.Cancel) {
                // Do nothing
              }
              visible = false
          }
        }
    }

    ExternalResourceBundle {
      id: externalResourceBundle
    }

    ValueRelationWidget {
      id: valueRelationWidget

      onWidgetClosed: backHandler.forceActiveFocus()
    }

    CodeReaderHandler {
      id: codeReaderHandler
    }

}
