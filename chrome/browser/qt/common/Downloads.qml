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
import Qt.labs.gestures 2.0

Item {
  id: downloadsContainer//; width: 640; height: 480
  property variant model: downloadsObject
  property int initx: 0
  property int inity: 0
  property alias textFocus: searchEdit.textFocus
  parent: outerContent
  property bool showed: false
  z: 0
  x: {!scene.fullscreen ? initx:0}
  y: {!scene.fullscreen ? inity:0}
  opacity: 0
  width: parent.width
  height: {!scene.fullscreen ? parent.height - y: parent.height}

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
    source: "image://themedimage/images/bg_application_p"
  }
  Rectangle {
    id: controlContainer
    width: parent.width
    height: 100
    anchors.top: parent.top
    anchors.topMargin: 10
    Image {
      id: controlBackground
      anchors.fill: parent
      fillMode: Image.Stretch
      source: "image://themedimage/images/bg_application_p"
    }
    Text {
      id: head
      height: 50
      text: downloadTitle
      anchors {top: parent.top; left: parent.left; leftMargin: 10;} 
      font { bold: true; pixelSize: 28 }
    }

    Text {
      id: clearAll
      height: 50
      anchors {top: parent.top; right: parent.right; rightMargin: 10;}
      text: downloadClearAll
      color: "blue"
      font { bold: true; pixelSize: 20;underline:true; }
      MouseArea {
        anchors.fill: parent
        onClicked: { downloadsContainer.model.clearAllItem()}
      }
    }
 
    TextEntry {
      id: searchEdit
      width: parent.width/2
      anchors { left: parent.left; leftMargin: 10; top: head.bottom;  bottom:parent.bottom; bottomMargin: 5;}
      defaultText: downloadSearch
      onTextChanged: {
        downloadsContainer.model.textChanged(text);
      }
    }
  }
  Image {
    id: divide
    height: 5
    width: parent.width
    anchors.top: controlContainer.bottom
    fillMode: Image.Stretch
    source: "image://themedimage/images/btn_grey_up"
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
