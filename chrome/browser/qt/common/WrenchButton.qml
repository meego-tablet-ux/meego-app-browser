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
  property Item topWidget
  objectName: "wrenchButton"
  height: parent.height
  width: childrenRect.width
  property bool shown: false
  property int popupDirection: 0 // 0 up, 1 down, 2 left, 3 right
  property int offsetHeight: 8

  Image {
      id: wrenchIconBg
      anchors.centerIn: parent
      height: parent.height
      source:  ""
      states: [
          State {
            name:  "pressed"
            when:  wrenchIcon.pressed
            PropertyChanges {
                target: wrenchIconBg
                source: "image://themedimage/widgets/common/toolbar-item/toolbar-item-background-active"
            }
          },
          State {
            name:  "selected"
            when:  shown
            PropertyChanges {
                target: wrenchIconBg
                source: "image://themedimage/widgets/common/toolbar-item/toolbar-item-background-selected"
            }
          }
      ]
  }

  Image {
    id: wrenchIcon
    anchors.verticalCenter: parent.verticalCenter
    source: "image://themedimage/icons/toolbar/view-actions"
    property bool pressed: false
    states: [
      State {
        name: "pressed"
        when: wrenchIcon.pressed
        PropertyChanges {
          target: wrenchIcon
          source: "image://themedimage/icons/toolbar/view-actions-active"
          height: parent.height
        }
      },
      State {
        name: "shown"
        when: shown
        PropertyChanges {
          target: wrenchIcon
          source: "image://themedimage/icons/toolbar/view-actions-selected"
          height: parent.height
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
        py = py + parent.height - offsetHeight
      }

      var map = mapToItem(topWidget, px, py);
      scene.lastMousePos.mouseX = map.x;
      scene.lastMousePos.mouseY = map.y;
      
      browserToolbarModel.wrenchButtonClicked();
    }
    onPressed: wrenchIcon.pressed = true
    onReleased: wrenchIcon.pressed = false
  }
}
