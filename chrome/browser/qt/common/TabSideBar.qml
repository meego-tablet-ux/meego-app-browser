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
import Qt.labs.gestures 2.0

Item {
  id: container
  anchors.fill: parent
  property int start_y: up?tabSideBarLoader.start_y - 15:tabSideBarLoader.start_y
  property int start_x: tabSideBarLoader.start_x
  property bool up: tabSideBarLoader.up
  property int sidebarMargin: 5
  property int headSpacing: 5
  property int maxSideBarHeight: tabSideBarLoader.maxSideBarHeight
  property bool isLandscape: scene.isLandscapeView()

  Rectangle {
    id: fog
    z: -2
    anchors.fill: parent
    color: "#000000"
    opacity: 0.5
  }

  Image {
    id: arrow
    x: start_x - width / 2
    y: up?start_y-height/4:start_y - height
    z: 1
    source: up? "image://themedimage/images/popupbox_arrow_top":"image://themedimage/images/popupbox_arrow_bottom"
  }

  Column {
    width: isLandscape ? tabgridview.width : 202
    property int tabviewHeight: isLandscape ? tabgridview.height : tablistview.item.height
    height: tabviewHeight ? borderImage1.height + newtab.height + tabviewHeight + 10 + borderImage2.height : borderImage1.height + newtab.height + tabviewHeight + borderImage2.height

    x: calcX()
    y: up?arrow.y + arrow.height-2:start_y - arrow.height - height +2

    function calcX () {
      var minimumX = 4
      if ((start_x-width/2) < minimumX) {
          return minimumX;
      } else if ((start_x + width/2) > parent.width) {
          return up? (start_x + toolbar.buttonWidth/2 - width):(start_x + toolbarBottom.buttonWidth/2 - width);
      } else {
          return Math.round(start_x-width/2);
      }
    }

    BorderImage {
      id: borderImage1
      width: parent.width
      border.left: 10
      border.right: 10
      source: "image://themedimage/images/popupbox_1"
    }

    BorderImage {
      source: "image://themedimage/images/popupbox_2"
      verticalTileMode: BorderImage.Repeat
      clip: true
      width: parent.width
      height: parent.height - borderImage1.height - borderImage2.height
      border.left: 10
      border.right: 10

      NewTab {
        id: newtab
        width: isLandscape ? 192 : 182
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 10 - borderImage1.height
        height: 50
        z: 1000
      }
      Loader {
        id:tabgridview
        source: isLandscape ? "TabGridView.qml" : ""
        anchors.left: parent.left
        anchors.top: newtab.bottom
        anchors.topMargin: 10
      }
      Loader {
        id:tablistview
        source: isLandscape ? "" : "TabListView.qml"
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: newtab.bottom
        anchors.topMargin: 10
        width: parent.width
        property int maxHeight: maxSideBarHeight - 20 - newtab.height - arrow.height - borderImage1.height - borderImage2.height
      }

      Connections {
        target: tabSideBarModel

        onSetNewTabEnabled: {
          if (enabled) {
            newtab.state = "normal"
          } else {
            newtab.state = "overLimit"
          }
        }
      }
    }

    BorderImage {
      id: borderImage2
      width: parent.width
      border.left: 10
      border.right: 10
      source: "image://themedimage/images/popupbox_3"
    }

  }

  GestureArea {
    anchors.fill: parent
    z: -2
    Tap {}
    TapAndHold {}
    Pan {}
    Pinch {}
    Swipe {}
  }

  Item {
    anchors.fill: parent
    z: -1 
    MouseArea {
      anchors.fill: parent
      onClicked: {
        //mouse.accepted = false
        browserToolbarModel.tabButtonClicked()
      }
    }
  }
}
