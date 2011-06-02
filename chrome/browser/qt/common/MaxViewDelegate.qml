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
//import QtQuick 1.0

Component{
    Item {
        id: gridItem;
        width: grid.cellWidth
	height: grid.cellHeight
        Rectangle {
            id: background
            width: maxViewRoot.itemWidth
	    height: maxViewRoot.itemHeight
            anchors { leftMargin: maxViewRoot.itemHMargin/2; topMargin:maxViewRoot.itemVMargin/2 } 	 
            anchors { rightMargin: maxViewRoot.itemHMargin/2; bottomMargin:maxViewRoot.itemVMargin/2 } 	 
	    smooth: true
	    color: "white"
	    border.color: "lightGray"
	    border.width: 2

            Image {
                id: snapshot
                source: thumbnail
                anchors.centerIn: background
	        anchors.fill: parent
                anchors { margins: 2 } 	 
                smooth: true 
		fillMode: Image.Stretch
            }

            Rectangle {
              id: titleContainer
	      color: "black"
              opacity: 0.8
              width: background.width
	      height: titleText.height + 10
              anchors { left: background.left; bottom: background.bottom }
/*
	      gradient: Gradient {
                  GradientStop { position: 0.0; color: "grey" }
         	  GradientStop { position: 1.0; color: "black" }
      	      }
*/
              Text { 
                id: titleText
		width: titleContainer.width - closeIcon.width - separator.width - 6
                anchors { left: titleContainer.left; leftMargin:8; rightMargin:8; verticalCenter:parent.verticalCenter }
		text: title
		elide: Text.ElideRight
                color: "white" 
                font.pixelSize: theme_fontPixelSizeNormal
              }

              Image {
                id: separator
                anchors.right: closeIcon.left
                width: 3
                height: parent.height
		fillMode: Image.Stretch
                source: "image://themedimage/widgets/common/toolbar/toolbar-item-separator"
              }

              Image {
                id: closeIcon
                height: parent.height
                width: height
                anchors { right: titleContainer.right; rightMargin:3; verticalCenter:parent.verticalCenter }
                source: "image://themedimage/images/notes/icn_close_up"
                property bool pressed: false
		visible: gridModel.getCloseButtonState()
                states: [
                  State {
                    name: "pressed"
                    when: closeIcon.pressed
                    PropertyChanges {
                    target: closeIcon
                    source: "image://themedimage/images/notes/icn_close_dn"
                    }
                  }
                ]

                MouseArea {
                  anchors.fill: parent
                  onClicked: {
                    mouse.accepted = true;
                    gridModel.removeWebPage(index)
		    console.log("remove index:" + index);
                  }
                  onPressed: {
                    mouse.accepted = true;
                    closeIcon.pressed = true
		    console.log("press index:" + index);
                  }
                  onReleased: {
                    mouse.accepted = true;
                    closeIcon.pressed = false
		    console.log("release index:" + index);
                  }
                }
              }

            }

            Behavior on x { enabled: gridMouseArea.longPressId!=-1 && background.state != "active"; NumberAnimation { duration: 400; easing.type: Easing.OutBack } }
            Behavior on y { enabled: gridMouseArea.longPressId!=-1 && background.state != "active"; NumberAnimation { duration: 400; easing.type: Easing.OutBack } }

            // highlighted the selected current thumbnail with a border
            states:[ 
	      State {
                name: 'highlight'
		when: gridMouseArea.pressId == grid_id && gridMouseArea.longPressId != grid_id
                PropertyChanges {
                    target: background;
                    border.color: "white";
                }
              },
	      State {
            	name: "active"; when: gridMouseArea.longPressId == grid_id
            	PropertyChanges {
	            target: background; 
	            scale: 0.8
                    border.color: "white";
		    x: gridMouseArea.mouseX - gridItem.x - background.width/2;
		    y: gridMouseArea.mouseY - gridItem.y - background.height/2; 
		}
	      },
	      State {
		name: "over"; when: gridMouseArea.dragOverId == grid_id
            	PropertyChanges {
	            target: background; 
	            scale: 1.05
		}
	      }
	    ]
            transitions: Transition { NumberAnimation { property: "scale"; duration: 200} }

        }

/*
        SequentialAnimation on rotation {
            NumberAnimation { to:  2; duration: 60 }
            NumberAnimation { to: -2; duration: 120 }
            NumberAnimation { to:  0; duration: 60 }
            running: gridMouseArea.longPressId != -1 && background.state != "active"
            loops: Animation.Infinite; alwaysRunToEnd: true
        }
*/
        states: [
            State {
              name: "landscape"
	      when: (scene.orientation == 1 || scene.orientation == 3) && index == 9
              PropertyChanges {
                target:gridItem
                visible: true
	      }
            },
            State {
              name: "portrait"
	      when: (scene.orientation == 2 || scene.orientation == 0) && index == 9
              PropertyChanges {
                target: gridItem
                visible: false
              }
            }
        ]
    }
}

