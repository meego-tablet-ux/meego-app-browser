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
  // Represent backward, forward and backforward button
  // It can switch between these 3 icons according to the 'kind'
  // property
  objectName: "backForwardButton"
  id: container
  height: 83
  width: height
  // button type: 
  // 0 - backward button
  // 1 - forward button
  // 2 - backforward button
  property int kind: 0
  // whether the button is active, only affect when button is of backward button
  property bool active: false
  property Item popup
  Image {
    id: backForwardIcon
    anchors.centerIn: parent
    height: parent.height
    source: "image://theme/browser/icn_toolbar_back_forward_up"
    property bool pressed: false
    opacity: 1
    states: [
      State {
        name: "popupShown"
          when: popup.showed
          PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_back_forward_dn"
          }
      },
      // backward inactive icon
      State {
        name: "backInactiveUnpressed"
        when: kind == 0 && !active && !backForwardIcon.pressed
        PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_back_button_up"
            opacity: 0.5
        }
      },
      State {
        name: "backActiveUnpressed"
        when: kind == 0 && active && !backForwardIcon.pressed
        PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_back_button_up"
        }
      },
      // backward active icon
      State {
        name: "backPressed"
        when: kind == 0 && backForwardIcon.pressed
        PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_back_button_dn"
        }
      },
      State {
        name: "forwardUnpressed"
        when: kind == 1 && !backForwardIcon.pressed
        PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_forward_button_up"
        }
      },
      // forward icon
      State {
        name: "forwardPressed"
        when: kind == 1 && backForwardIcon.pressed
        PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_forward_button_dn"
        }
      },
      State {
        name: "backForwardUnpressed"
          when: kind == 2 && !backForwardIcon.pressed
          PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_back_forward_up"
          }
      },
      // backforward icon
      State {
        name: "backForwardPressed"
          when: kind == 2 && backForwardIcon.pressed
          PropertyChanges {
            target: backForwardIcon
            source: "image://theme/browser/icn_toolbar_back_forward_dn"
          }
      }
    ]
  }
  MouseArea {
    anchors.fill: parent
    onPressed: backForwardIcon.pressed = true
    onReleased: backForwardIcon.pressed = false
    onClicked: {
      console.log("back-forward button is tapped")
      // open the history stack in a flyout
      var map = mapToItem(scene, width / 2, height / 3 * 2);
      scene.lastMousePos.mouseX = map.x;
      scene.lastMousePos.mouseY = map.y;
      browserToolbarModel.bfButtonTapped()
    }
    onPressAndHold: {
      // the same behavior with 'tap' gesture
      var map = mapToItem(scene, width / 2, height / 3 * 2);
      scene.lastMousePos.mouseX = map.x;
      scene.lastMousePos.mouseY = map.y;
      browserToolbarModel.bfButtonTappedAndHeld()
    }
  }
}
