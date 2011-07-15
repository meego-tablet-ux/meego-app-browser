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
  objectName: "reloadButton"
  height: parent.height
  width: childrenRect.width

  property bool loading : false
  
  Image {
      id: reloadBg
      anchors.centerIn: parent
      height: parent.height
      source:  ""
      states: [
          State {
            name:  "pressed"
            when:  reloadIcon.pressed
            PropertyChanges {
                target: reloadBg
                source: "image://themedimage/widgets/common/toolbar-item/toolbar-item-background-active"
            }
          }
      ]
  }

  Image {
    id: reloadIcon
    height: parent.height
    anchors.centerIn: parent
    source: "image://themedimage/icons/toolbar/view-refresh"
    property bool pressed: false
    states: [
      State {
        name: "reloadPressed"
        when: reloadIcon.pressed && !loading && !showqmlpanel
        PropertyChanges {
          target: reloadIcon
          source: "image://themedimage/icons/toolbar/view-refresh-active"
        }
      },
     State {
         name: "reloadUnPressed"
         when: !reloadIcon.pressed && !loading || showqmlpanel
         PropertyChanges {
             target: reloadIcon
             source: "image://themedimage/icons/toolbar/view-refresh"
         }
     }, 
    State {
        name: "stopPressed"
        when: reloadIcon.pressed && loading && !showqmlpanel
        PropertyChanges {
            target: reloadIcon
            source: "image://themedimage/icons/toolbar/view-refresh-stop-active"
        }
    }, 
    State {
        name: "stopUnPressed"
        when: !reloadIcon.pressed && loading && !showqmlpanel
        PropertyChanges {
            target: reloadIcon
            source: "image://themedimage/icons/toolbar/view-refresh-stop"
        }
    }
    ]

  }
  MouseArea {
    anchors.fill: parent
    onClicked: {
      if (!showqmlpanel)
        browserToolbarModel.reloadButtonClicked()
    }
    onPressed: if (!showqmlpanel) reloadIcon.pressed = true
    onReleased: reloadIcon.pressed = false
  }
}
