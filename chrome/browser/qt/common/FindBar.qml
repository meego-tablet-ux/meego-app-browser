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
    source: "image://themedimage/widgets/common/toolbar/toolbar-background"
    anchors.fill: parent

    TextEntry {
      id: findboxcontainer
      height: parent.height*4/5
      anchors.verticalCenter: parent.verticalCenter
      anchors.right: divider1.left
      anchors.left: parent.left
      anchors.margins: 5
      onTextChanged: findBarModel.textChanged(text);
      horizontalMargins: 20
      textInput.width: textInput.parent.width - horizontalMargins * 2 - matchesLabel.width
      textInput.anchors.right: matchesLabel.left
      textInput.anchors.rightMargin: 10
      color: "#383838"
      font.family: "Droid Sans"
      font.pixelSize: 18
      
      Item {
        id: matchesLabel
        objectName: "matchesLabel"
        parent: findboxcontainer.textInput.parent
        height: findboxcontainer.height
        width: matchesText.width
        anchors.right: parent.right
        anchors.rightMargin: 15
        Text {
          id: matchesText
          objectName: "matchesText"
          anchors.centerIn: parent
          color: "#999999"
          font.family: "Droid Sans"
          font.pixelSize: 15
        }
        Connections {
          target: findBarModel
          ignoreUnknownSignals:true;
          onSetMatchesLabel: matchesText.text = text
        }
      }
    }
    Image {
        id: divider1
        anchors.right: prevButton.left
        height: parent.height
        source: "image://themedimage/widgets/common/toolbar/toolbar-item-separator"
    }
    Item {
      id: prevButton
      height: parent.height
      width: height
      anchors.right: divider2.left
      Image {
        id: prevIconBg
        anchors.centerIn: parent
        height: parent.height
        width:  parent.width
        source:  ""
        states: [
          State {
            name:  "pressed"
            when:  prevIcon.pressed
            PropertyChanges {
              target: prevIconBg
              source: "image://themedimage/widgets/common/toolbar-item/toolbar-item-background-active"
            }
          }
        ]
      }
      Image {
        id: prevIcon
        anchors.centerIn: parent
        source: "image://themedimage/icons/toolbar/go-up"
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: prevIcon.pressed
            PropertyChanges {
              target: prevIcon
              source: "image://themedimage/icons/toolbar/go-up-active"
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
    Image {
        id: divider2
        anchors.right: nextButton.left
        height: parent.height
        source: "image://themedimage/widgets/common/toolbar/toolbar-item-separator"
    }
    Item {
      id: nextButton
      height: parent.height
      width: height
      anchors.right: divider3.left
      Image {
        id: nextIconBg
        anchors.centerIn: parent
        height: parent.height
        width:  parent.width
        source:  ""
        states: [
          State {
            name:  "pressed"
            when:  nextIcon.pressed
            PropertyChanges {
              target: nextIconBg
              source: "image://themedimage/widgets/common/toolbar-item/toolbar-item-background-active"
            }
          }
        ]
      }
      Image {
        id: nextIcon
        anchors.centerIn: parent
        source: "image://themedimage/icons/toolbar/go-down"
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: nextIcon.pressed
            PropertyChanges {
              target: nextIcon
              source: "image://themedimage/icons/toolbar/go-down-active"
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
    Image {
        id: divider3
        anchors.right: closeButton.left
        height: parent.height
        source: "image://themedimage/widgets/common/toolbar/toolbar-item-separator"
    }
    Item {
      id: closeButton
      objectName: "closeButton"
      height: parent.height
      width: height
      anchors.right: parent.right
      Image {
        id: closeIconBg
        anchors.centerIn: parent
        height: parent.height
        width:  parent.width
        source:  ""
        states: [
          State {
            name:  "pressed"
            when:  closeIcon.pressed
            PropertyChanges {
              target: closeIconBg
              source: "image://themedimage/widgets/common/toolbar-item/toolbar-item-background-active"
            }
          }
        ]
      }
      Image {
        id: closeIcon
        anchors.centerIn: parent
        source: "image://themedimage/icons/toolbar/view-refresh-stop"
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: closeIcon.pressed
            PropertyChanges {
              target: closeIcon
              source: "image://themedimage/icons/toolbar/view-refresh-stop-active"
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
    ignoreUnknownSignals:true;
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
