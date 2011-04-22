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

Item {
  id: container
  Rectangle {
    id: newTabContainer
    anchors.fill: parent
    anchors.margins: 4
    opacity: 1
    property bool newTabEnabled: true
    Rectangle {
      id: newtab
      z: 1000
      anchors.fill: parent
      color: "#383838"
      border.width: 4
      border.color: "black"
      Image {
        id: addbutton
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 10
        height: parent.height
        width: height
        anchors.left: parent.left
        source: "image://themedimage/browser/icn_add_up"
	property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: addbutton.pressed
            PropertyChanges{
              target: addbutton
              source: "image://themedimage/browser/icn_add_dn"
            }
          }
        ]

      }
      Text { 
        anchors.left: addbutton.right
        anchors.margins: 10
        anchors.verticalCenter: parent.verticalCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: theme_fontPixelSizeNormal
	//font.bold: true
        color: "white"
        text: newtabtitle 
      }
      MouseArea {
        anchors.fill: parent
	onPressed: { addbutton.pressed = true}
        onReleased: { mouse.accepted = true; if (newTabContainer.newTabEnabled) tabSideBarModel.newTab()}
      }
    }
    Rectangle {
      id: fog
      z: 10
      anchors.fill: parent
      color: "#CCCCCC"
      opacity: 0.5
    }
/*    Connections {
      target: tabSideBarModel
      onSetNewTabEnabled: {if (enabled) {newTabContainer.newTabEnabled = true; fog.z = 10; newtab.z = 1000;} 
                           else {newTabContainer.newTabEnabled = false; fog.z = 1000; newtab.z = 10;}}
    } */
  }
}
