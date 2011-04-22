 /****************************************************************************
 **
 ** Copyright (c) <2010> Intel Corporation.
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
  property int itemHeight: 40
  property int buttonMargin: 2

  Image {
    id: infobarContainer
    width: parent.width
    height: {(view.count  > container.maxItems ? maxItems * container.itemHeight : view.count * container.itemHeight)}
    source: "image://themedimage/images/navigationBar_l"

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
          anchors.leftMargin: 10
          anchors.right: acceptButton.left
          color: theme_buttonFontColor
          elide: Text.ElideRight
          verticalAlignment: Text.AlignVCenter
          font.pixelSize: container.itemHeight * 0.6
          text: info
        }

        Button {
          id: acceptButton
          anchors.right: cancelButton.left
          objectName: "acceptButton"
          bgSourceUp: "image://themedimage/images/btn_blue_up"
          bgSourceDn: "image://themedimage/images/btn_blue_dn"
          y: buttonMargin
          height: parent.height - buttonMargin * 2
          width: {
            if (acceptLabel == "")
              1
            else
              0
          }
          opacity: {
            if (acceptLabel == "")
              0
            else
              1
          }
          text: acceptLabel
          font.pixelSize: container.itemHeight * 0.6

          onClicked: {
            infobarContainerModel.infobarInvoked(index, "ButtonAccept")
          }
        }

        Button {
          id: cancelButton
          anchors.right: closeButton.left
          objectName: "cancelButton"
          bgSourceUp: "image://themedimage/images/btn_red_up"
          bgSourceDn: "image://themedimage/images/btn_red_dn"
          y: buttonMargin
          height: parent.height - buttonMargin * 2
          width: {
            if (cancelLabel == "")
              1
            else
              0
          }
          opacity: {
            if (cancelLabel == "")
              0
            else
              1
          }
          text: cancelLabel
          font.pixelSize: container.itemHeight * 0.6

          onClicked: {
            infobarContainerModel.infobarInvoked(index, "ButtonCancel")
          }
        }

        Item {
          id: closeButton
          objectName: "closeButton"
          anchors.right: parent.right
          anchors.rightMargin: 10
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
