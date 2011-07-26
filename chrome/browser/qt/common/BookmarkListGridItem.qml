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

Component{
    Item {
        id: main
        width: grid.cellWidth; height: grid.cellHeight
        Rectangle {
            id: item; parent: gridMouseArea
            width: main.width - itemMargin*2; height: main.height - itemMargin*2
            x: main.x - grid.contentX; y: main.y - grid.contentY
           // anchors { left: main.left; top: main.top; margins: itemMargin }
            color: "white";
            border.color: "gray"; border.width: 1

            Item {
                id: snapshotContainer
                /*color: green; opacity: 0.5*/
                /*x: item.x; y: item.y*/
                width: parent.width - itemMargin; height: parent.height - itemMargin
                anchors { left: parent.left; top: parent.top; margins: itemMargin/2 }
                Image {
                    id: snapshot
                    source: image
                    width: parent.width; height: parent.height
                    smooth: true; fillMode: Image.PreserveAspectFit
                }
                states: [
                    State {
                        name: "exiting"; when: bmGlobal.exiting
                        PropertyChanges {
                            target: snapshotContainer
                            parent: bmlistContainer
                            x: bmlistContainer.x; y: bmlistContainer.y
                            width: bmlistContainer.width; height: bmlistContainer.height
                        }
                    }
                ]
                transitions: [
                    Transition {
                        from: '*'; to: 'exiting'
                        SequentialAnimation {
                            NumberAnimation { properties: "x,y,width,height"; duration: 400; easing.type: Easing.OutQuad }
                        }
                    }
                ]
                //Rectangle { color: "lightgreen"
                Item {  // container of click to page mousearea
                    id: clickerContainer
                    anchors.top: snapshot.top
                    width: snapshot.width; height: snapshot.height - titleContainer.height - urlContainer.height
                    visible: true
                    MouseArea {
                    //    enabled: !bmGlobal.dialoging
                        anchors.fill: parent
                        onClicked: {
                    //        bmGlobal.exiting = true
                    //        snapshot.fillMode = Image.PreserveAspectCrop
                    //        bmlistContainer.visible = false
                          grid.model.openBookmarkItem(gridId);
                        }
                        onPressAndHold: {
                          var map = mapToItem(window, mouseX, mouseY);
                          //map.y-=115;
                          //bmGlobal.idxHasMenu = grid.indexAt(map.x-parallax.width*(1-bmGlobal.parallaxWidthFactor), map.y-headHeight)
                          bmGlobal.idHasMenu = gridId
                          bmGlobal.currentTitle = title
                          bmGlobal.currentUrl = url
                          grid.model.popupMenu(map.x, map.y)
                          console.log("hdq press and hold ", map.x, map.y, gridId)
                        }
                    }
                }
            }

            Item {
                id: titleContainer
                width: parent.width - itemMargin; height: titleText.height + itemMargin/2
                anchors { left: item.left; bottom: urlContainer.top; leftMargin: itemMargin/2 }
            }
            Item {
                id: urlContainer
                width: titleContainer.width; height: urlText.height + itemMargin/2; clip: true
                anchors { left: titleContainer.left; bottom: item.bottom; bottomMargin: 5 }
            }
            Rectangle {
              color: "black"
              opacity: 0.55
              y: snapshotContainer.height - height + itemMargin
              width: titleContainer.width + itemMargin;
              height: titleContainer.height + urlContainer.height + itemMargin/2
            }
            Text {
                id: titleText; text: title; width: titleContainer.width; elide: Text.ElideRight
                color: "white"; font.pixelSize: theme_fontPixelSizeNormal;
                anchors { left: titleContainer.left; top: titleContainer.top; leftMargin: itemMargin/2; topMargin: itemMargin/4 }
            }
            Text {
                id: urlText; text: url; width: urlContainer.width - itemMargin; elide: Text.ElideRight
                color: "white"; font.pixelSize: theme_fontPixelSizeMedium;
                anchors { left: urlContainer.left; top: urlContainer.top; leftMargin: itemMargin/2; topMargin: itemMargin/8 }
            }

            // not working when parent is not gridMouseArea
            Behavior on x {
                enabled: gridMouseArea.currentId != -1 && item.state != "active";
                NumberAnimation { duration: 400; easing.type: Easing.OutBack }
            }
            Behavior on y {
                enabled: gridMouseArea.currentId != -1 && item.state != "active";
                NumberAnimation { duration: 400; easing.type: Easing.OutBack }
            }
            states: State {
                name: "active"; when: gridMouseArea.currentId == gridId
                PropertyChanges {
                    target: item; scale: 0.6;
                    //parent: gridMouseArea
                    x: gridMouseArea.mouseX - width/2 - itemMargin;
                    y: gridMouseArea.mouseY - height/2 - itemMargin;
                    border.color: "cornflowerblue"; border.width: 2
                }
            }//,
            //    State {
            //        name: "resetmodel"; when: bmGlobal.resetModel
            //        PropertyChanges {
            //            target: item; parent: main
            //        }
            //    }
            //transitions: Transition { NumberAnimation { property: "scale"; duration: 200 } }
//            Rectangle {
//                anchors.fill: parent;
//                border.color: "#326487"; border.width: 6
//                color: "transparent"; radius: 5
//                visible: item.state == "active"
//            }
        }
    }
}

