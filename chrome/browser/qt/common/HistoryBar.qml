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
    // represents a overlay with a shown history bar 
    id: historyOverlay
    anchors.fill: parent
    property bool showed: false
	property int itemCount: 0
	property int commonMargin: 4
	property int showWidth: (itemCount * 240 + commonMargin*3) < parent.width ? itemCount * 240 + commonMargin*3 : parent.width
    //property alias historyBarY: historyBar.y
    z: 10
    opacity: 0
    property int fingerX: 0
    property int fingerY: 0

    Rectangle {
      id: fog
      anchors.fill: parent
      color: "gray"
      opacity: 0.4 
    }

    Image {
      id: finger
      x: fingerX - finger.width / 2 
      y: fingerY
      source: "image://themedimage/images/popupbox_arrow_top"
    }

    Item {
      id: historyContainer
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: finger.bottom
      height: historyBar.height
      z: parent.z

    BorderImage {
        id: borderImage1
        source: "image://themedimage/images/popupbox_1"
        border.left: 20
        border.right: 20
        border.top: 5
        width: showWidth
        anchors.top: parent.top
    }
    
    BorderImage {
        anchors.top: borderImage1.bottom
        anchors.bottom: borderImage2.top
        source: "image://themedimage/images/popupbox_2"
        verticalTileMode: BorderImage.Repeat
        width: showWidth
        clip: true
        height: parent.height - borderImage1.height - borderImage2.height
        border.left: 20
        border.right: 20
    }
    
    BorderImage {
        id: borderImage2
        anchors.bottom: parent.bottom
        source: "image://themedimage/images/popupbox_3"
        width: showWidth
        border.left: 20
        border.right: 20
        border.bottom: 34
    }
    HistoryView {
        id: historyBar
        width: parent.width
        // y: toolbar.y + toolbar.height
        z: parent.z + 1
    }

    }

    GestureArea {
      anchors.fill: parent
      Tap {}
      TapAndHold {}
      Pan {}
      Pinch {}
      Swipe {}
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            historyOverlay.showed = false
        }
    }

    transitions: Transition {
      PropertyAnimation { properties: "opacity"; easing.type: Easing.OutSine; duration: 500}
    }
    states: State {
        name: "showOverlay"
        when: historyOverlay.showed
        PropertyChanges {
            target: historyOverlay
            opacity: 1
        }
    }
    // listen to signals from historyBar to decide whether showing history overlay
    Connections {
        target: historyBar
        onHideOverlay: {
            historyOverlay.showed = false
        }
        onShowOverlay: {
            historyOverlay.showed = true
        }
    }
}
