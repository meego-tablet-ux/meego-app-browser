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
    // represents a overlay with a shown history bar 
    id: historyOverlay
    anchors.fill: parent
    property bool showed: false
    property int itemCount: 0
    property int commonMargin: 10
    property int parentWidth: 0
    property int itemWidth: 180 + itemShadowWidth*2
    property int itemShadowWidth: 8
    property int spaceWidth: commonMargin - itemShadowWidth*2
    property int showWidth: (itemCount * itemWidth + (commonMargin - itemShadowWidth)*2 + (itemCount - 1) * spaceWidth) < parentWidth ? 
        itemCount * itemWidth + (commonMargin - itemShadowWidth)*2 + (itemCount - 1) * spaceWidth: 
        parentWidth;
    //property alias historyBarY: historyBar.y
    z: 10
    opacity: 0
    property int fingerX: 0
    property int fingerY: 0
    property int stackImageBorderWidth: 11
    property int stackImageBlankSpaceWidth: 6

    //ModalFogBrowser {}

    Item {
      id: historyContainer
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: parent.top 
      anchors.topMargin: 55 + stackImageBlankSpaceWidth/2
      height: historyBar.height
      z: parent.z

      BorderImage {
        id: historyStackBorderImage
        border.left : stackImageBorderWidth
        border.right: stackImageBorderWidth
        border.top: stackImageBorderWidth
        border.bottom: stackImageBorderWidth
        width: showWidth + 2*stackImageBlankSpaceWidth
        anchors.top: parent.top
        anchors.topMargin: 0 - stackImageBlankSpaceWidth
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0 - stackImageBlankSpaceWidth
        anchors.left: parent.left
        anchors.leftMargin: 0 - stackImageBlankSpaceWidth
        verticalTileMode: BorderImage.Stretch
        horizontalTileMode: BorderImage.Stretch
        source : "image://themedimage/widgets/apps/browser/stacks-background"
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
            historyStackModel.OnOverlayHide()
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
        ignoreUnknownSignals:true;
        onHideOverlay: {
            historyOverlay.showed = false
        }
        onShowOverlay: {
            historyOverlay.showed = true
        }
    }
}
