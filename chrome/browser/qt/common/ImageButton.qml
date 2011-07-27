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

Rectangle {
    id: button

    property alias text: title.text
    property alias btnColor: button.color
    property alias textColor: title.color
    property alias fontSize: title.font.pixelSize
    property alias imageSource: buttonImage.source
    property int imageLeftBlankSpace: 0
    property int widthNeed: imageSource == ""? buttonImage.paintedWidth + title.paintedWidth - imageLeftBlankSpace + 20 : buttonImage.paintedWidth + title.paintedWidth - imageLeftBlankSpace + 10
    signal clicked

    width: 150; height: 45
    smooth: true
    radius: 5

    SystemPalette { id: palette }
/*
    gradient: Gradient {
        GradientStop { id: gradientStop; position: 0.0; color:palette.midlight  }
        GradientStop { position: 1.0; color: "grey" }
    }
*/
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: { button.clicked() }
    }

    Image {
        id: buttonImage
        anchors.left: parent.left
        anchors.top: parent.top
    }

    Text {
        id: title
        anchors.left: buttonImage.right
        anchors.top: parent.top
        font.pixelSize: 22
        elide: Text.ElideRight
    }

    Component.onCompleted: {
        var totalWidth;
        totalWidth = buttonImage.paintedWidth + title.paintedWidth - imageLeftBlankSpace;
        if(button.width > totalWidth){
            buttonImage.anchors.leftMargin = (button.width - totalWidth)/2 - imageLeftBlankSpace;
        }else{
            buttonImage.anchors.leftMargin = imageLeftBlankSpace;
            title.anchors.right = button.right;
            title.anchors.rightMargin = 10;
            if(buttonImage.source == ""){
                buttonImage.anchors.leftMargin = 10;
            }
        }
        buttonImage.anchors.topMargin = (button.height - buttonImage.paintedHeight)/2;
        title.anchors.topMargin = (button.height - title.paintedHeight)/2;
    }

    states: State {
        name: "pressed"
        when: mouseArea.pressed
        PropertyChanges { target: button; color: palette.dark }
    }
}
