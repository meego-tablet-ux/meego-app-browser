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
  property alias model: view.model
  property int itemMaxWidth: 200
  property int itemMinWidth: 60
  property int containerHeight: 55
  property int titleFontSize: 13
  property bool showInstruction: true
  parent: outerContent
  width: parent.width
  states: [
    State {
      name: "hide"
      when: !scene.showbookmarkbar || scene.fullscreen
      PropertyChanges {
        target: bookmarkBarLoader
          height: 0
          opacity: 0
      }
    },
    State {
      name: "show"
      when: scene.showbookmarkbar && !scene.fullscreen
      PropertyChanges {
        target: bookmarkBarLoader
          height: containerHeight
          opacity: 1
      }
    }
  ]
  /*
  transitions: [
    Transition {
      from: "show"
      to: "hide"
      reversible: true
      PropertyAnimation {
        properties: "anchors.topMargin,height,opacity"
        duration: 500
      }
    }
  ]
  */

  BorderImage {
    id: bookmarkcontainer
    height: parent.height
    width: parent.width
    opacity:1
    source: "image://themedimage/widgets/apps/browser/bookmark-bar-background"
    border { left:8; right:8; bottom:0; top:0 }
    horizontalTileMode: BorderImage.Repeat
    verticalTileMode: BorderImage.Repeat
    Text {
      id: instruction
      color: "gray"
      anchors.top: parent.top
      anchors.bottom: parent.bottom
      anchors.left: parent.left
      anchors.leftMargin: 5
      verticalAlignment: Text.AlignVCenter
      font.pixelSize: container.height*0.5
      text: bookmarkInstruction
      opacity: scene.showbookmarkbar && !scene.fullscreen && container.showInstruction ? 1.0 : 0.0
    }
    Component {
      id: bookmarkDelegate
      BorderImage {
        id: bookmarkitem
        property bool pressed: false
        property bool active: true
        opacity: active ? 1.0 : 0.5
        width:{
          if(length*13 + 18*2 > container.itemMaxWidth) 
             container.itemMaxWidth;
          else if (length*13 + 18*2 < container.itemMinWidth) 
             container.itemMinWidth;
          else
             length*13 + 18*2;
        } 
        height: parent.height - 16
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        source: "image://themedimage/widgets/apps/browser/bookmark-bar-item-background"
        border { left:19; right:19; top:0; bottom:0 }
        horizontalTileMode: BorderImage.Repeat
        verticalTileMode: BorderImage.Stretch
        Text {
          id: texttitle
          anchors.fill: parent
          anchors.leftMargin: 18
          anchors.rightMargin: 18
          color: "#383838"
          elide: Text.ElideRight
          verticalAlignment: Text.AlignVCenter
          horizontalAlignment: Text.AlignHCenter
          font.pixelSize: titleFontSize
          font.bold: false
          font.family: "Droid Sans"
          text: title
          wrapMode: Text.NoWrap
          clip: true
        }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            container.model.openBookmarkItem(bookmarkId);
          }
          onPressed: parent.pressed = true
          onReleased: parent.pressed = false
          onPositionChanged: parent.pressed = false
          onPressAndHold: parent.pressed = false
        }
      }
    }

    ListView {
      id: view
      anchors.fill: parent
      boundsBehavior: Flickable.DragOverBounds
      anchors.leftMargin: 10
      anchors.rightMargin: 10
      spacing: 8
      delegate: bookmarkDelegate
      model: bookmarkBarModel
      focus: true
      clip: true
      orientation: ListView.Horizontal
      // only interactive when items are full of container
      interactive: childrenRect.width + 10 > parent.width? true:false 
      opacity: 1
      states: State {
         name: "ShowBars"
         when: view.movingHorizontally
         PropertyChanges { target: horizontalScrollBar; opacity: 1; height: 5}
      }
    }
    ScrollBar {
      id: horizontalScrollBar
      width: view.width; height: 0
      anchors.bottom: bookmarkcontainer.bottom
      opacity: 0
      orientation: Qt.Horizontal
      position: view.visibleArea.xPosition
      pageSize: view.visibleArea.widthRatio
    }
  }
  Connections {
    target: bookmarkBarModel
    onShowInstruction: container.showInstruction = true;
    onHideInstruction: container.showInstruction = false;
  }
}

