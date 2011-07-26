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
import MeeGo.Components 0.1

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
      ModalFogBrowser {
        opacity: 0
      }

      MouseArea {
        anchors.fill: parent
        onClicked: {
          container.model.cancel();
          container.close();
        }
      }

      Item {
        id: bubble
        x: container.bubbleX
        y: container.bubbleY
        width: container.bubbleWidth
        height: 320
        opacity: container.bubbleOpacity
        
        Item {

            id: bubbleColumn
            width: parent.width
            height: parent.height
          
            Item {
                id: titleWrapper
                height: 55
                width: parent.width

                BorderImage {
                    id: titleBorderImageShadow1
                    height: parent.height - titleBorderImageShadow2.height
                    anchors.left : parent.left
                    anchors.leftMargin: -16
                    anchors.right: parent.right
                    anchors.rightMargin: -16    
                    anchors.top : parent.top
                    border {
                        left: 16;
                        right: 16;
                    }
                    source: "image://themedimage/images/popupbox_1_shadow"
                }

                BorderImage {
                    id: titleBorderImageShadow2
                    height: 45
                    anchors.left : parent.left
                    anchors.leftMargin: -16
                    anchors.right: parent.right
                    anchors.rightMargin: -16    
                    anchors.bottom: parent.bottom
                    border {
                        left: 16;
                        right: 16;
                    }
                    source: "image://themedimage/images/popupbox_2_shadow"
                }

                BorderImage {
                    id: titleBorderImage
                    anchors.left: parent.left
                    anchors.leftMargin: -5
                    anchors.right: parent.right
                    anchors.rightMargin: -5
                    anchors.top: parent.top
                    anchors.topMargin: -4
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: -6
                    border{
                        left: 10;
                        right: 10;
                        top: 9;
                        bottom: 6
                    }
                    source: "image://themedimage/widgets/apps/browser/header-background"
                }
                Text {
                    id: title
                    width: parent.width
                    height: parent.height
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
            }

            Image {
                id: divide
                height: 2
                width: parent.width
                anchors.top: titleWrapper.bottom
                fillMode: Image.Stretch
                source: "image://themedimage/widgets/common/menu/menu-item-separator"
            }

            BorderImage {
                id: borderImageMiddle
                width: parent.width
                
                anchors.top: titleWrapper.bottom
                anchors.bottom: borderImageBottom.top
                anchors.left: titleWrapper.left
                border{
                    left: 5;
                    right: 5;
                }
                source: "image://themedimage/images/popupbox_2"
            }
    
            BorderImage {
                id: borderImageMiddleShadow
                anchors.top: borderImageMiddle.top
                anchors.bottom: borderImageMiddle.bottom
                anchors.right: borderImageMiddle.right
                anchors.rightMargin: -16
                anchors.left: borderImageMiddle.left
                anchors.leftMargin: -16
                border {
                    left: 16;
                    right: 16;
                }
                source: "image://themedimage/images/popupbox_2_shadow"
            }
            
            BorderImage {
                id: borderImageBottom
                width: parent.width
                height: 7
                anchors.bottom: button.bottom
                anchors.left: parent.left
                border{
                    left: 5;
                    right: 5;
                }
                source: "image://themedimage/images/popupbox_3"
            }

            BorderImage {
                id: borderImageBottomShadow
                anchors.top: borderImageBottom.top
                anchors.bottom: borderImageBottom.bottom
                anchors.bottomMargin: -29 
                anchors.right: borderImageBottom.right
                anchors.rightMargin: -15
                anchors.left: borderImageBottom.left
                anchors.leftMargin: -15
                border {
                    left: 19;
                    right: 19;
                    bottom: 32;
                }
                source: "image://themedimage/images/popupbox_3_shadow"
            }

            Item {
                id: nameLabelAndEditWrapper
                anchors.top: divide.bottom
                width: parent.width
                height: nameEdit.height + 45
                
                Item{
                    id: nameLabelWrapper
                    height: 35
                    width: parent.width
                    anchors.top: parent.top
                    anchors.left: parent.left

                    Text {
                        id: nameLabel
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 10
                        text: bubbleName
                        color: "#383838"
                        font.pixelSize: 13
                        font.bold: true
                        font.family: defaultFont
                    }
                }

                TextEntry {
                    id: nameEdit
                    text: bubbleNameInput
                    color: "#383838"
                    height: itemHeight
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.top: nameLabelWrapper.bottom

                    onTextChanged: {
                        container.model.setTitle(text);
                    }
                }
            }

            Image {   
              id: nameEditAndFolderLabelDivide
              height: 2
              width: parent.width
              anchors.top: nameLabelAndEditWrapper.bottom
              fillMode: Image.Stretch
              source: "image://themedimage/widgets/common/menu/menu-item-separator"
            }

            Item {

                id: folderLabelAndGroupWrapper
                width: parent.width
                anchors.top: nameEditAndFolderLabelDivide.bottom 
                height: 85 // 80 if uncomment folderGroup.anchors.topMargin
                
                Item {
                    id: folderLabelWrapper
                    height:35
                    width:parent.width
                    anchors.top: parent.top
                    anchors.left: parent.left
                    Text {
                        id: folderLabel
                        text: bubbleFolder
                        color: "#383838"
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.right: parent.right
                        anchors.rightMargin: 10
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: 10
                        font.pixelSize: 13
                        font.bold: true
                        font.family: defaultFont
                    }
                }

                DropDown {

                    id: folderGroup
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.bottom: folderLabelAndGroupWrapper.bottom
                    anchors.top: folderLabelWrapper.bottom
                    //anchors.topMargin: -5

                    title: "DropDown"
                    titleColor: "black"
                    maxWidth: 300

                    model: bubbleFolderModel

                    iconRow: [
                        Item {
                            id: dummyItem
                            height: parent.height
                            width: 7
                        }
                    ]

                    onTriggered: {
                        container.model.folderSelectedIndex(selectedIndex);
                    }

                    Component.onCompleted: {
                        selectedIndex = 0;
                        container.model.folderSelectedIndex(selectedIndex);
                    }
                }
            }
            Image {
                id: divide2
                height: 2
                width: parent.width
                anchors.top: folderLabelAndGroupWrapper.bottom
                anchors.topMargin: 5
                fillMode: Image.Stretch
                //visible: folderGroup.state == "elapsed"
                source: "image://themedimage/widgets/common/menu/menu-item-separator"
            }

            Item {
                id: button
                anchors.top: divide2.bottom
                anchors.left: parent.left
                height: doneButton.height + 20
                width: parent.width

                ImageButton {
                    id: doneButton
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    width: (parent.width - 30) / 2
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

                ImageButton {
                    id: removeAndCancelButton
                    anchors.right:parent.right
                    anchors.rightMargin: 10
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    width: (parent.width - 30) / 2
                    height: 45
                    btnColor: "red"
                    textColor: "white"
                    fontSize: 18
                    text: bubbleRemove
                    imageSource: "image://themedimage/icons/menus/delete-active"
                    imageLeftBlankSpace: 5
                    onClicked: {
                        container.model.setTitle(nameEdit.text);
                        container.model.removeButtonClicked();
                        container.close();
                    }
                }
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
              x: container.bubbleWidth + 5
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
