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
  objectName: "tabButton"
  height: parent.height
  width: childrenRect.width
  property Item tabSidebar
  property int popupDirection: 0 // 0 up, 1 down, 2 left, 3 right

  Image {
    id: tabIcon
    height: parent.height
    anchors.centerIn: parent
    source: "image://themedimage/images/browser/icn_toolbar_tabs_button_up"
    property bool pressed: false
    states: [
      State {
        name: "pressed"
        when: tabIcon.pressed
        PropertyChanges {
          target: tabIcon
          source: "image://themedimage/images/browser/icn_toolbar_tabs_button_dn"
        }
      },
      State {
        name: "sidebarShown"
        when: tabSidebar.showItem
        PropertyChanges {
            target: tabIcon
            source: "image://themedimage/images/browser/icn_toolbar_tabs_button_dn"
        }
      }
    ]
  }
  MouseArea {
    anchors.fill: parent
    onClicked: {
      var px = width /2;
      var py = 0;

      if (popupDirection == 0) {
        py = 0
      } else if (popupDirection == 1) {
        py = py + parent.height
      }

      var map = mapToItem(scene, px, py);
      scene.lastMousePos.mouseX = map.x;
      scene.lastMousePos.mouseY = map.y;
      tabButton.focus = true;
      browserToolbarModel.tabButtonClicked()
    }
    onPressed: tabIcon.pressed = true
    onReleased: tabIcon.pressed = false
  }
}
