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

Item {
  id: container
  state: "normal"
  states: [
    State {
      name: "normal"
      PropertyChanges{
        target: newtab
        source: "image://themedimage/widgets/common/button/button-default"
        border.left: 12
        border.right: 12
        border.top: 12
        border.bottom: 12
        newTabEnabled: true
      }
      PropertyChanges{
        target: textDisplay
        text: newtabtitle
        font.pixelSize: theme_fontPixelSizeNormal
        color: "white"
        font.bold: false
      }
    },
    State {
      name: "overLimit"
      PropertyChanges{
        target: newtab
        source: "image://themedimage/widgets/common/infobar/infobar-background"
        border.left: 4
        border.right: 4
        border.top: 4
        border.bottom: 4
        newTabEnabled: false
      }
      PropertyChanges{
        target: textDisplay
        text: overLimitContent
        font.pixelSize: 13
        color: "#383838"
        font.bold: true
      }
    }
  ]
  BorderImage{
    id: newtab
    source: "image://themedimage/widgets/common/button/button-default"
    state: "normal"
    border.left: 12
    border.right: 12
    border.top: 12
    border.bottom: 12
    anchors.fill : parent
    property bool newTabEnabled: true

    TextEdit {
      id: textDisplay
      width: parent.width
      height: parent.height
      anchors.verticalCenter: parent.verticalCenter
      anchors.horizontalCenter: parent.horizontalCenter
      verticalAlignment: Text.AlignVCenter
      horizontalAlignment: Text.AlignHCenter
      readOnly: true
      wrapMode: TextEdit.WordWrap
      font.pixelSize: theme_fontPixelSizeNormal
      font.family: theme_fontFamily
      font.bold: false
      color: "white"
      text: newtabtitle 
    }
    MouseArea {
        anchors.fill: parent
        onPressed: { 
          if( container.state == "normal"){
            newtab.source = "image://themedimage/widgets/common/button/button-default-pressed"
          }
        }
        onReleased: { 
          showbookmarkmanager = false;
          showdownloadmanager = false;
          tabChangeFromTabSideBar = true;
          mouse.accepted = true; 
          if (newtab.newTabEnabled) tabSideBarModel.newTab();
        }
    }
  }
}
