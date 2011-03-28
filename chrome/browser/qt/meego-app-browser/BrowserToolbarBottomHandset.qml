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
  id: toolbarBottom
  x: 0
  y: parent.height-height
  width: parent.width
  height: 50
  anchors.topMargin: 0

  property int buttonWidth: 65
  property int buttonHeight: toolbarBottom.height

  property alias wrenchmenushown: wrenchButton.shown

  states: [
    State {
      name: "show"
      when: parent.containerVisible && !scene.fullscreen
      PropertyChanges {
        target: toolbarBottom
        height: 50
        opacity: 1
      }
    },
    State {
      name: "hide"
      when: !parent.containerVisible || scene.fullscreen
      PropertyChanges {
        target: toolbarBottom
        height: 0
        opacity: 0
      }
    }
  ]

  transitions: [
    Transition {
      from: "*"
      to: "*"
      PropertyAnimation {
          properties: "anchors.topMargin,height,opacity"
          duration: 500
      }
    }
  ]

  BorderImage {
    id: background
    source: "image://theme/titlebar_l"
    anchors.fill: parent
    width: parent.width

    //do we really need this one?
    GestureArea {
      anchors.fill: parent
      Tap {}
      TapAndHold {}
      Pan {}
      Pinch {}
      Swipe {}
    }

    ReloadButton {
      id: reloadButton
      anchors.left: parent.left
      anchors.leftMargin: 10
      height: buttonHeight
      width: buttonWidth
    }

    WrenchButton {
      id: wrenchButton
      topWidget: scene
      popupDirection: 0
      height: buttonHeight
      width: buttonWidth
      anchors.horizontalCenter: parent.horizontalCenter
    }

    TabButton {
      id: tabButton
      anchors.right: parent.right
      anchors.rightMargin: 10
      height: buttonHeight
      width: buttonWidth
      popupDirection: 0
    }
  }
}
