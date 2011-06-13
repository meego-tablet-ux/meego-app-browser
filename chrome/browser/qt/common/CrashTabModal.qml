 /****************************************************************************
 **
 ** Copyright (c) 2011 Intel Corporation.
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

Item {
  id: container
  property variant model: browserCrashDialogModel
  opacity: 0
  function show(){
    container.opacity = 1
    fadeIn.running = true
  }
  function hide(){
    container.opacity = 0
    fadeOut.running = true
  }
  PropertyAnimation{
    id: fadeOut
    running: false
    target: container
    property: "opacity"
    from: 1
    to: 0
    duration: theme_dialogAnimationDuration
  }
  PropertyAnimation{
    id: fadeIn
    running: false
    target: container
    property: "opacity"
    from: 0
    to: 1
    duration: theme_dialogAnimationDuration
  }
  Rectangle {
    id: fogRect
    anchors.fill: parent
    color: "gray"
    opacity: 0.5
  }
  MouseArea {
    id: blocker
    anchors.fill: parent
  }
  BorderImage {
    id: borderBack
    width: 342
    height: 230
    anchors.centerIn: parent
    border.left: 6
    border.right: 6
    border.top: 55
    border.bottom: 65
    source: "image://themedimage/widgets/common/modal-dialog/modal-dialog-background"
    clip: true
    horizontalTileMode: BorderImage.Stretch
    verticalTileMode: BorderImage.Stretch
    z: 1

    Text { 
      id: headTitle
      anchors.left: parent.left
      anchors.top: parent.top
      anchors.leftMargin: 14
      anchors.topMargin: 20
      font.bold: true
      font.pixelSize: 18
      font.family: "Droid Sans"
      color: "#383838"
      text: model.GetHeadContent()
    }
    TextEdit {
      id: bodyTitle
      readOnly: true
      wrapMode: TextEdit.WordWrap
      anchors.left: parent.left
      anchors.right: parent.right
      anchors.top: headTitle.bottom
      anchors.leftMargin: 14
      anchors.rightMargin: 14
      anchors.topMargin: 35
      font.family: "Droid Sans"
      font.pixelSize: 18
      color: "#383838"
      text: model.GetBodyContent()
    }
    BorderImage {
      id: borderButton
      width: parent.width / 2.5
      height: 50
      source: "image://themedimage/widgets/common/button/button"
      anchors.bottom: parent.bottom
      anchors.bottomMargin: 5
      anchors.horizontalCenter: parent.horizontalCenter
      border.left: 11
      border.right: 11
      border.top: 9
      border.bottom: 12
      
      Text {
        id: buttonTitle
        anchors.centerIn: parent
        font.family: "Droid Sans"
        font.pixelSize: 18
        color: "white"
        text: model.GetCloseButtonContent()
      }
      MouseArea{
        id: buttonArea
        anchors.fill: parent
        onClicked: { 
          hide();
          browserCrashTabObject.onCloseButtonClicked();
        }
        onPressed: {
          borderButton.source = "image://themedimage/widgets/common/button/button-pressed";
        }
        onReleased: {
          borderButton.source = "image://themedimage/widgets/common/button/button";
        }
      }
    }  
  }
}
