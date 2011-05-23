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
  width: parent.width
  height: 50
  anchors.top: parent.top
  anchors.topMargin: 0
  onCrumbTriggered: scene.crumbTriggered(payload)
  onSearch: scene.search(needle)
  onAppSwitcherTriggered: mainWindow.showTaskSwitcher()

  states: [
    State {
      name: "show"
      when: scene.showtoolbar && !scene.fullscreen && !scene.appmode
      PropertyChanges {
        target: toolbar
        height: 50
        opacity: 1
      }
    },
    State {
      name: "hide"
      when: !scene.showtoolbar || scene.fullscreen || scene.appmode
      PropertyChanges {
        target: toolbar
        height: 0
        opacity: 0
      }
    }
  ]

  transitions: [
    Transition {
      from: "*"
      to: "*"
      PropertyAnimation {
          properties: "anchors.topMargin,height,opacity"
          duration: 500
      }
    }
  ]


  property alias locationBar: omniboxcontainer

  property bool showsearch: true
  // to comply with window
  property int crumbTrail: 0
  property int crumbMaxWidth: 160
  property int crumbHeight: height

  signal crumbTriggered(variant payload)
  signal search(string needle)
  signal appSwitcherTriggered()

  BorderImage {
    id: background
    source: "image://themedimage/images/titlebar_l"
    anchors.fill: parent

    BackForwardButton {
      id: backForwardButton
      // change its icons to listen to signals from browserToolbarModel
      height: parent.height
      Connections {
        target: browserToolbarModel
        onUpdateBfButton: {
            backForwardButton.kind = kind
            backForwardButton.active = active
        }
      }
    }

    Omnibox {
      id: omniboxcontainer
      height: parent.height
      popupHeight: innerContent.height/2
      z: 10
      anchors.right: closeButton.left
      anchors.left: backForwardButton.right
      state: "normal"
      onActiveFocusChanged: {
        if (activeFocus == true){
          if (!showqmlpanel)
            state = "expand";
        } else {
            state = "normal";
        }
      }
      states: [
        State{
          name: "normal"
            PropertyChanges{ target:omniboxcontainer; anchors.left:backForwardButton.right }
            PropertyChanges{ target:backForwardButton; opacity:1 }
          },
        State{
          name: "expand"
            PropertyChanges{ target:omniboxcontainer; anchors.left:parent.left }
            PropertyChanges{ target:backForwardButton; opacity:0 }
        }
     ]

    }

    CloseButton {
      id: closeButton
      height: parent.height
    }

  }
}
