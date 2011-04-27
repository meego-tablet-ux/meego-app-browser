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
    property double fogOpacity: 0.4

    property int fingerX: 0
    property int fingerY: 0

    property variant payload
    property int itemHeight: 45
    property int fingerMode: 0

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
	  Rectangle {
	    id: fog
	    anchors.fill: parent
	    color: "gray"
	    opacity: container.fogOpacity
	    Behavior on opacity {
		PropertyAnimation { duration: 500 }
	    }
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
	    height: 300
	    opacity: container.bubbleOpacity

	    BorderImage {
	      id: borderImage1
	      source: "image://themedimage/images/popupbox_1"
	      border.left: 20
	      border.right: 20
	      border.top: 5
	      width: parent.width
	      anchors.top: parent.top
            }
	    BorderImage {
                id: borderImageMiddle
		anchors.top: borderImage1.bottom
		anchors.bottom: borderImage2.top
		source: "image://themedimage/images/popupbox_2"
                verticalTileMode: BorderImage.Repeat
		width: parent.width
		clip: true
		height: column.height
		border.left: 20
		border.right: 20
	    }
	    BorderImage {
		id: borderImage2
		anchors.bottom: parent.bottom
		source: "image://themedimage/images/popupbox_3"
		width: parent.width
		border.left: 20
		border.right: 20
		border.bottom: 10
	    }

            Column {
              id: column
              anchors.fill: parent
              anchors.leftMargin:20
              anchors.rightMargin:20
              Text {
                id: title
                width:column.width
                height: itemHeight
                color: "gray"
                text: bubbleTitle
                verticalAlignment:Text.AlignVCenter
                font.bold: true
                font.pixelSize: height*0.5
              }
              Image {
                id: divide
                height: 2
                width: parent.width
                fillMode: Image.Stretch
                source: "image://themedimage/images/btn_grey_up"
              }
              Text {
                id: nameLabel
                text: bubbleName
                color: "gray"
                height: itemHeight
                font.pixelSize: height*0.5
                verticalAlignment:Text.AlignVCenter
              }
              BubbleInputBox {
                id: nameEdit
                text:  bubbleNameInput
                textColor: "gray"
                width: parent.width
                height: itemHeight
                onTextChanged: {
                  container.model.setTitle(text);
                }
              }
              Text {
                id: folderLabel
                text: bubbleFolder
                color: "gray"
                height: itemHeight
                font.pixelSize: height*0.5
                verticalAlignment:Text.AlignVCenter
              }
              GroupBox {
                id: folderGroup
                infoText: bubbleFolderInput
                innerHeight: 150
                initHeight: 50
                loader.sourceComponent: folderCombo
                Component {
                    id: folderCombo
                    ListView {
                        id: view
                        anchors.topMargin: 5
                        anchors.bottomMargin: 5
                        spacing: 5
                        delegate: Text {
                          id: folderItem
                          text: modelData
                          width: parent.width
                          opacity: 0.5
                          font.pixelSize: itemHeight*0.5
                          height: folderGroup.initHeight
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
            }
            Row {
              id: button
              anchors.horizontalCenter: parent.horizontalCenter
              anchors.bottom: parent.bottom
              anchors.bottomMargin: 10
              spacing: 20
              height: itemHeight
              TextButton {
                id: doneButton
                text: bubbleDone
                onClicked: {
                  container.model.setTitle(nameEdit.text);
                  container.model.doneButtonClicked();
                  container.close();
                }
              }
              TextButton {
                id: removeAndCancelButton
                text: bubbleRemove
                onClicked: {
                  container.model.setTitle(nameEdit.text);
                  container.model.removeButtonClicked();
                  container.close();
                }
              }
            }
      	    Behavior on opacity {
		PropertyAnimation { duration: 500 }
	    }

	    Image {
		id: finger
		opacity:bubble.opacity
		Behavior on opacity {
         		PropertyAnimation { duration: 100 }
		}
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
				x: container.fingerX - finger.width / 2
				y: -(finger.height) + 2
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
