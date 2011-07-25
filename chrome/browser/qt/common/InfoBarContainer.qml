 /****************************************************************************
 **
 ** Copyright (c) 2010 Intel Corporation.
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of Intel Corporation nor the names of its 
 **     contributors may be used to endorse or promote
 **     products derived from this software without specific prior written
 **     permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 **
 ****************************************************************************/

import Qt 4.7
import MeeGo.Components 0.1

Item {
  id: container

  height: infobarContainer.height
  property alias model: view.model
  property int maxItems: 3
  property int itemHeight: 65
  property int buttonMargin: 5

  Image {
    id: infobarContainer
    width: parent.width
    height: {(view.count  > container.maxItems ? maxItems * container.itemHeight : view.count * container.itemHeight)}
    source: "image://themedimage/widgets/app/browser/infobar-navbar-background"

    clip: true

    Component {
      id: infobarDelegate

      Item {
        width: parent.width
        height: container.itemHeight
        opacity: 1

        Text {
          id: infolabel
          anchors.verticalCenter: parent.verticalCenter
          anchors.left: parent.left
          anchors.leftMargin: buttonMargin
          anchors.rightMargin: buttonMargin * 2
          anchors.right: acceptButton.left
          color: "white"
          font.pixelSize: theme_fontPixelSizeNormal
          font.family: theme_fontFamily
          elide: Text.ElideRight
          verticalAlignment: Text.AlignVCenter
          horizontalAlignment: Text.AlignRight
          text: info
        }

        Button {
          id: acceptButton
          anchors.right: cancelButton.left
          objectName: "acceptButton"
          anchors.verticalCenter: parent.verticalCenter
          bgSourceUp: "image://themedimage/widgets/common/button/button-default"
          bgSourceDn: "image://themedimage/widgets/common/button/button-default-pressed"
          width: 120
          height: parent.height - buttonMargin * 2
          text: acceptLabel
          textColor: "white"
          font.pixelSize: theme_fontPixelSizeLarge
          font.family: theme_fontFamily

          Component.onCompleted: { 
              if (acceptLabel == "") {
                  acceptButton.width = 0;
                  acceptButton.opacity = 0;
              }
          }
 
          onClicked: {
            infobarContainerModel.infobarInvoked(index, "ButtonAccept")
          }
        }

        Button {
          id: cancelButton
          anchors.right: vSeperatorImage.left
          objectName: "cancelButton"
          bgSourceUp: "image://themedimage/widgets/common/button/button"
          bgSourceDn: "image://themedimage/widgets/common/button/button-pressed"
          anchors.rightMargin: buttonMargin
          anchors.verticalCenter: parent.verticalCenter
          width: 120
          height: parent.height - buttonMargin * 2
          text: cancelLabel
          textColor: "white"
          font.pixelSize: theme_fontPixelSizeLarge
          font.family: theme_fontFamily

          Component.onCompleted: { 
              if (cancelLabel == "") {
                  cancelButton.width = 0;
                  cancelButton.opacity = 0;
              }
          }

          onClicked: {
            infobarContainerModel.infobarInvoked(index, "ButtonCancel")
          }
        }
        
        Image {
          id: vSeperatorImage
          anchors.right: closeButton.left
          height: parent.height
          source: "image://themedimage/widgets/common/action-bar/action-bar-separator"
        }

        Item {
          id: closeButton
          objectName: "closeButton"
          anchors.right: parent.right
          height: parent.height
          width: height 
          z: 10
          Image {
            id: closeIcon
            anchors.centerIn: parent
            source: "image://themedimage/images/browser/btn_findbar_close"
            property bool pressed: false
            states: [
              State {
                name: "pressed"
                when: closeIcon.pressed
                PropertyChanges {
                  target: closeIcon
                  source: "image://themedimage/images/browser/btn_findbar_close_dn"
                }
              }
            ]
          }
          MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: {
              infobarContainerModel.infobarInvoked(index, "ButtonClose")
            }
            onPressed: closeIcon.pressed = true
            onReleased: closeIcon.pressed = false
          }
        }

        Image {
            id: seperatorImage
            anchors.bottom: parent.bottom
            width: parent.width
            visible: index < view.count - 1     // Seperator won't be visible for the last item
            source: "image://themedimage/images/menu_item_separator"
        }

      }
    }

    ListView {
      id: view
      anchors.fill: parent
      delegate: infobarDelegate
      model: infobarContainerModel
      focus: false
      interactive: count  > container.maxItems ? true : false
      opacity: 1
    }
  }

}
