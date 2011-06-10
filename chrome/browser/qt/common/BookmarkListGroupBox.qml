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

Item
{
    id:group
    property alias mouseArea: mouseArea
    property alias loader: contentLoader
    property alias infoText: groupInfoText.text
    property alias info: groupInfoText
    property int innerHeight
    property int itemHeight
    width: parent.width
    height: groupBorderImage.height + innerHeight
    signal clicked()

    property bool expanded: state=="expanded"

    state: "elapsed"

    states: [
        State {
            name: "elapsed"
            PropertyChanges  { target:contentLoader; height: 0; sourceComponent: undefined }
            PropertyChanges  { target:groupBorderImage; opacity: 0.7; height: header.height }
            PropertyChanges  { target:arrow; source: "image://themedimage/icons/toolbar/go-up" }
        },
        State {
            name: "expanded"
            PropertyChanges  { target:contentLoader; height: innerHeight;}
            PropertyChanges  { target:groupBorderImage; opacity: 1; height: header.height + contentLoader.childrenRect.height }
            PropertyChanges  { target:arrow; source: "image://themedimage/icons/toolbar/go-down" }
        }
    ]

    BorderImage
    {
        id:groupBorderImage
        source:"image://themedimage/widgets/common/menu/default-dropdown"
        border { left: 12; top: 12; bottom: 12; right: 12 }
        width: parent.width
        height: header.height

        BorderImage {
            id: header
            source: "image://themedimage/widgets/apps/browser/header-dropdown-bar"
            border { left: 12; top: 0; bottom: 0; right: 12 }
            width: parent.width
            height: 50

            // Info text
            Text {
                id: groupInfoText
                anchors.top: parent.top
                anchors.topMargin:12
                anchors.left: parent.left
                anchors.leftMargin:15
                anchors.right: arrow.left
                anchors.rightMargin: 10
                font.pixelSize: theme_fontPixelSizeNormal
                clip: true
                elide: Text.ElideRight
                color:"#383838"
            }

            Image {
                id:arrow
                anchors.verticalCenter: groupInfoText.verticalCenter
                anchors.right: parent.right
            }
        }

    }

    Loader
    {
        id:contentLoader
        width:groupBorderImage.width - 25
        anchors { top: parent.top; topMargin: header.height; left: parent.left; leftMargin: 15 }
        //onLoaded: groupBorderImage.height = header.height + contentLoader.childrenRect.height
    }

    MouseArea {
        id: mouseArea
        anchors.top:parent.top
        anchors.left:parent.left
        width:parent.width
        height: header.height
        hoverEnabled: true
        onClicked: {
            if(group.state == "expanded") {
                group.state = "elapsed";
            }
            else { 
                group.state = "expanded"; 
            }
        }
    }

}
