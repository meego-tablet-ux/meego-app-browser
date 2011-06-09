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
import Qt.labs.gestures 2.0

Item {
  id: container
  property bool showfindbar: false
  property alias textentry: findboxcontainer
  width: parent.width
  GestureArea {
    anchors.fill: parent
    Tap {}
    TapAndHold {}
    Pan {}
    Pinch {}
    Swipe {}
  }

  BorderImage {
    id: background
    source: "image://themedimage/images/titlebar_l"
    anchors.fill: parent

    TextEntry {
      id: findboxcontainer
      width: container.width - matchesLabel.width - prevButton.width - nextButton.width - closeButton.width - 10
      height: parent.height*4/5
      anchors.verticalCenter: parent.verticalCenter
      anchors.right: matchesLabel.left
      anchors.margins: 5
      onTextChanged: findBarModel.textChanged(text);
    }

    Item {
      id: matchesLabel
      objectName: "matchesLabel"
      height: parent.height
      width: matchesText.width
      anchors.right: prevButton.left
      Text {
        id: matchesText
        objectName: "matchesText"
        anchors.centerIn: parent
//        color: "black"
        color: theme_fontColorHighlight
        font.pixelSize: 24
      }
      Connections {
        target: findBarModel
        onSetMatchesLabel: matchesText.text = text
      }
    }
    Item {
      id: prevButton
      height: parent.height
      width: height
      anchors.right: nextButton.left
      Image {
        id: prevIcon
        anchors.centerIn: parent
        source: "image://themedimage/images/browser/btn_findbar_prev"
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: prevIcon.pressed
            PropertyChanges {
              target: prevIcon
              source: "image://themedimage/images/browser/btn_findbar_prev_dn"
            }
          }
        ]
      }
      MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: findBarModel.prevButtonClicked()
        onPressed: prevIcon.pressed = true
        onReleased: prevIcon.pressed = false
      }
    }
    Item {
      id: nextButton
      height: parent.height
      width: height
      anchors.right: closeButton.left
      Image {
        id: nextIcon
        anchors.centerIn: parent
        source: "image://themedimage/images/browser/btn_findbar_next"
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: nextIcon.pressed
            PropertyChanges {
              target: nextIcon
              source: "image://themedimage/images/browser/btn_findbar_next_dn"
            }
          }
        ]
      }
      MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: findBarModel.nextButtonClicked()
        onPressed: nextIcon.pressed = true
        onReleased: nextIcon.pressed = false
      }
    }

    Item {
      id: closeButton
      objectName: "closeButton"
      height: parent.height
      width: height
      anchors.right: parent.right
      Image {
        id: closeIcon
        anchors.centerIn: parent
        source: "image://themedimage/images/browser/btn_findbar_close"
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: closeIcon.pressed
            PropertyChanges {
              target: closeIcon
              source: "image://themedimage/images/browser/btn_findbar_close_dn"
            }
          }
        ]
      }
      MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
          container.showfindbar = false;
          findBarModel.closeButtonClicked();
          //scene.hasfindbar = false;
        }
        onPressed: closeIcon.pressed = true
        onReleased: closeIcon.pressed = false
      }
    }
  }
  Connections {
    target: findBarModel
    //  findBarModel.positionUpdated(container.x, container.y, container.width, container.height);
    onHide: container.showfindbar = false
    onSetX: {
      container.x = (x + width > toolbar.x + toolbar.width) ? container.x : x
    }
  }
  states: [
    State {
      name: "show"
      when: container.showfindbar
      PropertyChanges {
        target: container
        opacity: 1
        z: 20
      }
    },
    State {
      name: "hide"
      when: !container.showfindbar
      PropertyChanges {
        target: container
        opacity: 0
        z: 0
      }
    }
  ]
  transitions: [
    Transition {
      from: "hide"
      to: "show"
      PropertyAnimation {
        properties: "opacity"
        duration: 200
      }
    },
    Transition {
      from: "show"
      to: "hide"
      PropertyAnimation {
        properties: "opacity"
        duration: 250
      }
    }
  ]
}
