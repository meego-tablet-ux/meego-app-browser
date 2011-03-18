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
import MeeGo.Labs.Components 0.1
import Qt.labs.gestures 2.0

Item {
  id: downloadsContainer//; width: 640; height: 480
  property variant model: downloadsObject
  anchors.fill: parent
  property bool showed: false
  z: 10
  opacity: 0

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
  }

  Image {
    id: downloadBackground
    anchors.fill: parent
    fillMode: Image.Stretch
    source: "image://theme/bg_application_p"
  }
  Rectangle {
    id: controlContainer
    width: parent.width
    height: 50
    anchors.top: parent.top
    anchors.topMargin: 10
    Image {
      id: controlBackground
      anchors.fill: parent
      fillMode: Image.Stretch
      source: "image://theme/bg_application_p"
    }

    Image {
      id: back
      height: parent.height
      width: height
      anchors.left: parent.left
      property bool pressed: false
      source: "image://theme/icn_toolbar_back_button_up"
      states: [
        State {
          name: "backPressed"
          when: back.pressed
          PropertyChanges {
            target: back
            source: "image://theme/icn_toolbar_back_button_dn"
          }
        }
      ]
      MouseArea {
        anchors.fill: parent
        onClicked: { 
          downloadsContainer.showed = false;
          back.pressed = false; 
        }
        onPressed: {
          back.pressed = true;
        }
      }
    }

    Text {
      id: head
      height: parent.height
      text: downloadTitle
      anchors.left: back.right
      anchors.leftMargin: 30
      anchors.verticalCenter: parent.verticalCenter
      verticalAlignment:Text.AlignVCenter
      font { bold: true; pixelSize: 28 }
    }
 
    TextEntry {
      id: searchEdit
      width: downloadsContainer.width*1/2; height: head.height*4/5
      anchors { left: head.right; leftMargin: 10; verticalCenter: back.verticalCenter }
      defaultText: downloadSearch
      onTextChanged: {
        downloadsContainer.model.textChanged(text);
      }
    }
    Text {
      id: clearAll
      height: parent.height
      anchors.right: parent.right
      anchors.rightMargin: 10
      anchors.verticalCenter: parent.verticalCenter
      verticalAlignment:Text.AlignVCenter
      text: downloadClearAll
      color: "blue"
      font { bold: true; pixelSize: 20;underline:true; }
      MouseArea {
        anchors.fill: parent
        onClicked: { downloadsContainer.model.clearAllItem()}
      }
    }
  }
  Image {
    id: divide
    height: 5
    width: parent.width
    anchors.top: controlContainer.bottom
    fillMode: Image.Stretch
    source: "image://theme/btn_grey_up"
  }
  DownloadShelf {
    id: listContainer
    anchors.margins: 10
    height: parent.height - controlContainer.height - controlContainer.anchors.topMargin- divide.height;
    model: downloadsObject
    anchors { top: divide.bottom }
  }
  states: State {
    name: "showDownload"
    when: downloadsContainer.showed
    PropertyChanges {
      target: downloadsContainer
      opacity: 1
    }
  }
}
