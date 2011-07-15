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
  id: downloadsContainer
  property variant model: downloadsObject
  anchors.fill: parent
  property int headTextPixel: 21
  property int headTopMargin: 20
  property int headLeftMargin: 20
  property int headRightMargin: 30
  property int bodyMargin: 40
  property alias textFocus: searchEdit.textFocus
  property bool showed: false
  property bool isLandscape: scene.isLandscapeView()
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
    source: "image://themedimage/images/bg_application_p"
  }
  Rectangle {
    id: controlContainer
    width: parent.width
    height: 150
    anchors.top: parent.top
    anchors.topMargin: headTopMargin
    Image {
      id: controlBackground
      anchors.fill: parent
      fillMode: Image.Stretch
      source: "image://themedimage/images/bg_application_p"
    }
    Text {
      id: head
      text: downloadTitle
      anchors {top: parent.top; left: parent.left; leftMargin: headLeftMargin;} 
      font { pixelSize: headTextPixel }
      font.family: theme_fontFamily
    }

    Text {
      id: clearAll
      height: 50
      anchors {bottom: parent.bottom; right: parent.right; rightMargin: 30;}
      text: downloadClearAll
      color: "red"
      font { family: theme_fontFamily; pixelSize: 20;underline:true; }
      MouseArea {
        anchors.fill: parent
        onClicked: { downloadsContainer.model.clearAllItem()}
      }
    }
 
    TextEntry {
      id: searchEdit
      width: isLandscape? parent.width/2 : (parent.width*2)/3
      height: 40
      anchors { left: parent.left; leftMargin:headLeftMargin; top: head.bottom; topMargin: 20}
      defaultText: downloadSearch
      onTextChanged: {
        downloadsContainer.model.textChanged(text);
      }
    }
    
    //this item is deleted, because of string freeze.    
    /***
    TextButton {
      id: sendRequest
      width: isLandscape? parent.width/5 : parent.width/4
      height: 40
      anchors.left: searchEdit.right
      anchors.leftMargin: 20
      anchors.top: head.bottom
      anchors.topMargin: 20
      textColor: "white"
      fontSize: 20
      btnColor: "#2CACE3"
      text: "Send"
      onClicked: {
        downloadsContainer.model.textChanged(searchEdit.text);
      }
    }
    */
  }

  DefaultDivider {
    id: divide
    anchors { left: parent.left; leftMargin: 30; right: parent.right; rightMargin: 30; top: controlContainer.bottom }
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
