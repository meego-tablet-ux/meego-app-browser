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
//import QtQuick 1.0

Component{
    Item {
        id: gridItem;
        width: grid.cellWidth
	height: grid.cellHeight
        Rectangle {
            id: background
            width: parent.width - sector.itemHMargin*2
	    height: parent.height - sector.itemVMargin*2
            anchors { leftMargin:sector.itemHMargin; topMargin:sector.itemVMargin } 	 
	    //anchors.fill: parent
            //radius: 12
	    smooth: true
	    color: "white"
	    border.color: "grey"
	    border.width: 1

            Image {
                id: snapshot
                source: thumbnail
                anchors.centerIn: background
                anchors.margins: 2
                width: background.width-4; height: background-4
                smooth: true 
		fillMode: Image.PreserveAspectFit
            }

            Rectangle {
              id: titleContainer
	      color: "black"
              opacity: 0.8
              width: background.width
	      height: titleText.height*2
              anchors { left: background.left; bottom: background.bottom}
	      gradient: Gradient {
                  GradientStop { position: 0.0; color: "grey" }
         	  GradientStop { position: 1.0; color: "black" }
      	      }

              Text { 
                    id: titleText
		    width: titleContainer.width - 8
                    anchors { left: titleContainer.left; top: titleContainer.top; margins:8 }
		    text: title
		    elide: Text.ElideRight
                    color: "white" 
		    font.pixelSize: 15
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

    }
}

