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
import Qt.labs.gestures 2.0

Item {
  id: container
  visible: false
  anchors.fill: parent
  property int radius
  property string color: "grey"
  property int  bubbleWidth: bubbleLoader.bubbleWidth
  property int  bubbleHeight: 0
  property int  bubbleX: 0
  property int  bubbleY: 0
  property variant model: bookmarkBubbleObject
  property double bubbleOpacity: 1

  property int fingerX: 0
  property int fingerY: 0

  property variant payload
  property int itemHeight: 45
  property int fingerMode: 0

  property string defaultFont: theme_fontFamily
  signal close()
  property Item bubbleItem: null

  GestureArea {
    anchors.fill: parent
    Tap {}
    TapAndHold {}
    Pan {}
    Pinch {}
    Swipe {}
  }

  //doesn't need this one?
  MouseArea {
    anchors.fill: parent
    acceptedButtons: Qt.LeftButton | Qt.RightButton
  }

  onVisibleChanged: {
    if(!visible) {
      close();  
    }
    else {
      if(container.bubbleItem) container.bubbleItem.destroy();
      container.bubbleItem = bubbleVisualLogic.createObject(container)
      bubbleHeight = container.bubbleItem.bubbleHeight;
    }
  }
   
  onClose: {
    bubbleLoader.close()
  }
   
  Component {
    id: bubbleVisualLogic
    Item {
      id: everything
      anchors.fill: parent
      property int bubbleHeight: bubble.height
      ModalFogBrowser {}

      MouseArea {
        anchors.fill: parent
        onClicked: {
          //container.model.cancel();
          container.close();
        }
      }

      Item {
        id: bubble
        x: container.bubbleX
        y: container.bubbleY
        width: container.bubbleWidth
        height: 292
        opacity: container.bubbleOpacity
        
        Column {
          id: bubbleColumn
          width: parent.width
          height: parent.height
          BorderImage {
            id: borderImage1
            source: "image://themedimage/images/popupbox_1"
            border.left: 20
            border.right: 20
            border.top: 5
            width: parent.width
          }
          BorderImage {
            id: borderImageMiddle
            source: "image://themedimage/images/popupbox_2"
            verticalTileMode: BorderImage.Repeat
            width: parent.width
            clip: true
            height: parent.height - borderImage1.height - borderImage2.height
            border.left: 20
            border.right: 20
            Text {
              id: title
              width: parent.width
              height: 55 - borderImage1.height - divide.height
              anchors.left: parent.left
              anchors.leftMargin: 20
              anchors.top: parent.top
              color: "#383838"
              text: bubbleTitle
              verticalAlignment:Text.AlignVCenter
              font.bold: true
              font.pixelSize: 18
              font.family: defaultFont
            }
            Image {
              id: divide
              height: 2
              width: parent.width
              anchors.top: title.bottom
              fillMode: Image.Stretch
              source: "image://themedimage/widgets/common/menu/menu-item-separator"
            }
            Text {
              id: nameLabel
              width: parent.width
              anchors.left: parent.left
              anchors.leftMargin: 10
              anchors.top: divide.bottom
              text: bubbleName
              color: "#383838"
              height: 35
              font.pixelSize: 13
              font.bold: true
              font.family: defaultFont
              verticalAlignment:Text.AlignVCenter
            }
            BubbleInputBox {
              id: nameEdit
              text:  bubbleNameInput
              textColor: "#383838"
              height: itemHeight
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.leftMargin: 10
              anchors.rightMargin: 10
              anchors.top: nameLabel.bottom
              onTextChanged: {
                container.model.setTitle(text);
              }
            }
            Text {
              id: folderLabel
              text: bubbleFolder
              color: "#383838"
              height: 35
              width:parent.width
              anchors.left: parent.left
              anchors.leftMargin: 10
              anchors.top: nameEdit.bottom
              font.pixelSize: 13
              font.bold: true
              font.family: defaultFont
              verticalAlignment:Text.AlignVCenter
            }

            GroupBox {
              id: folderGroup
              anchors.left: parent.left
              anchors.right: parent.right
              anchors.leftMargin: 10
              anchors.rightMargin: 10
              anchors.top: folderLabel.bottom
              infoText: bubbleFolderInput
              innerHeight: 135
              initHeight: 45
              loader.sourceComponent: folderCombo
              Component {
                id: folderCombo
                ListView {
                  id: view
                  delegate: Text {
                    id: folderItem
                    text: modelData
                    width: parent.width
                    opacity: 0.5
                    font.pixelSize: 18
                    height: folderGroup.initHeight
                    verticalAlignment: Text.AlignVCenter
                    MouseArea {
                      anchors.fill: parent
                      onClicked: {
                        folderGroup.infoText = modelData;
                        container.model.folderSelectedIndex(index);
                        folderGroup.state = "elapsed";
                        button.opacity = 1;
                      }
                    }
                  }
                  model:  bubbleFolderModel
                  focus: true
                  orientation: ListView.Vertical
                  opacity: 1
                }
              }
            }
            Image {
              id: divide2
              height: 2
              width: parent.width
              anchors.top: folderGroup.bottom
              anchors.topMargin: 10
              fillMode: Image.Stretch
              visible: folderGroup.state == "elapsed"
              source: "image://themedimage/widgets/common/menu/menu-item-separator"
            }
            Row {
              id: button
              anchors.horizontalCenter: parent.horizontalCenter
              anchors.bottom: parent.bottom
              anchors.top: divide2.bottom
              anchors.topMargin: 10
              anchors.leftMargin: 10
              anchors.rightMargin: 10
              anchors.bottomMargin: 10
              spacing: 10
              height: itemHeight
              TextButton {
                id: doneButton
                width:121
                height:45
                btnColor: "#2CACE3"
                textColor: "white"
                fontSize: 18
                text: bubbleDone
                onClicked: {
                  container.model.setTitle(nameEdit.text);
                  container.model.doneButtonClicked();
                  container.close();
                }
              }
              TextButton {
                id: removeAndCancelButton
                width: 121
                height: 45
                btnColor: "red"
                textColor: "white"
                fontSize: 18
                text: bubbleRemove
                onClicked: {
                  container.model.setTitle(nameEdit.text);
                  container.model.removeButtonClicked();
                  container.close();
                }
              }
            }

          }
          BorderImage {
            id: borderImage2
            source: "image://themedimage/images/popupbox_3"
            width: parent.width
            border.left: 20
            border.right: 20
            border.bottom: 10
          }
        }
    
        Behavior on opacity {
          PropertyAnimation { duration: 500 }
        }
      }
      Image {
        id: finger
        opacity:bubble.opacity
        Behavior on opacity {
          PropertyAnimation { duration: 100 }
        }
        states: [
          State {
            name: "left"
            when: container.fingerMode == 0
            PropertyChanges {
              target: finger
              x: container.fingerX
              y: container.fingerY - finger.height / 2
              source: "image://themedimage/images/popupbox_arrow_left"
            }
          },
          State {
            name: "right"
            when: container.fingerMode == 1
            PropertyChanges {
              target: finger
              x: container.bubbleWidth - borderImage1.border.right + 5
              y: container.fingerY - finger.height / 2
              source: "image://themedimage/images/popupbox_arrow_right"
            }
          },
          State {
            name: "top"
            when: container.fingerMode == 2
            PropertyChanges {
              target: finger
              //x: container.fingerX - finger.width / 2
              //y: -(finger.height) + 2
              x: container.fingerX - finger.width/2
              y: container.fingerY - finger.height/2;
              source: "image://themedimage/images/popupbox_arrow_top"
            }
          },
          State {
            name: "bottom"
            when: container.fingerMode == 3
            PropertyChanges {
              target: finger
              x: container.fingerX - finger.width / 2
              y: container.bubbleHeight - 13 - finger.height
              source: "image://themedimage/images/popupbox_arrow_bottom"
            }
          }
        ]
      }
    }
  }
}
