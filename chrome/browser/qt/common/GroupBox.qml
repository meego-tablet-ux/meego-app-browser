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

Item
{
    id:group
    property alias mouseArea: mouseArea
    property alias loader: contentLoader
    property alias infoText: groupInfoText.text
    property alias info: groupInfoText
    property int innerHeight
    property int initHeight
    width:parent.width
    height: groupBorderImage.height
    signal clicked()

    state: "elapsed"

    states: [
        State {
            name: "elapsed"
            PropertyChanges  { target:contentLoader; height: 0; sourceComponent:undefined }
            PropertyChanges  { target:groupBorderImage; height: 60; opacity: 0.7}
            PropertyChanges  { target:arrowDown; rotation:180; }
        },
        State {
            name: "expanded"
            PropertyChanges  { target:contentLoader; height: innerHeight;}
            PropertyChanges  { target:arrowDown; rotation:0; }
        }
    ]

//    transitions: Transition {
//        NumberAnimation { properties: "height"; easing.type: Easing.InOutQuart; duration: 300 }
//    }

    BorderImage
    {
        id:groupBorderImage
        source:"image://theme/browser/bg_list_white"
        opacity:1
        width: parent.width
        border { left: 20; right: 20; top:10; bottom: 10}
        height:  60//+ border.top
        horizontalTileMode: BorderImage.Stretch
        verticalTileMode: BorderImage.Stretch

        BorderImage {
            id: bg
            source: "image://theme/browser/bg_bookmarkbar"
            border { left: 20; right: 20; top: 50; bottom:20 }
            width: parent.width
            height: parent.height
        }


        // Info text
        Text {
            id: groupInfoText
           // text: 
            anchors.top: parent.top
            anchors.topMargin:20
            anchors.left: parent.left
            anchors.leftMargin:10
            font.pixelSize: 20
            color:"black"
            style: Text.Outline
            styleColor: "gray"
        }

        Image {
            id:arrowDown
            source: "image://theme/notes/icn_dropdown_off"
            anchors.top: parent.top
            anchors.topMargin: 23
            anchors.right: parent.right
            anchors.rightMargin: 25
            state: "inactive"
            states: [
                State {
                    name: "inactive"
                    PropertyChanges {
                        target: arrowDown
                        source: "image://theme/notes/icn_dropdown_off"
                        anchors.topMargin: 23
                        anchors.rightMargin: 20
                    }
                },
                State {
                    name: "active"
                    PropertyChanges {
                        target: arrowDown
                        source: "image://theme/notes/icn_dropdown_active"
                        anchors.topMargin: (64 - arrowDown.height)/2
                        anchors.rightMargin: 10
                    }
                }
            ]
        }
    }

    Loader
    {
        id:contentLoader
        width:groupBorderImage.width - 20
        anchors { top: parent.top; topMargin: 70; left: parent.left; leftMargin: 8 }
        onLoaded: groupBorderImage.height = item.height + 70 + groupBorderImage.border.top + groupBorderImage.border.bottom
    }

    MouseArea {
  	id: mouseArea
  	anchors.top:parent.top
        anchors.left:parent.left
        width:parent.width
        height:50
        hoverEnabled: true

        onEntered: { arrowDown.state = "active"; groupBorderImage.opacity = 1; }

        onExited: {
            arrowDown.state = "inactive";
            if(group.state == "elapsed")groupBorderImage.opacity = 0.7;
        }

        onClicked: {
            if(group.state == "expanded") {
                group.state = "elapsed";
                button.opacity = 1;
            }
            else { 
                group.state = "expanded"; 
                button.opacity = 0;
            }
  	}
    }

}
