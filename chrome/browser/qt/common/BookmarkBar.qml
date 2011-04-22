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
  property alias model: view.model
  property int itemWidth: 200
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
          height: 45
          opacity: 1
      }
    }
  ]
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

  Rectangle {
    id: bookmarkcontainer
    height: parent.height
    width: parent.width
    clip: true
    opacity:1
    Image {
      id: bookmarkbackground
      anchors.fill: parent
      fillMode: Image.Tile
      source: "image://themedimage/browser/bg_bookmarkbar"
    }
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
      opacity: 0
    }
    Component {
      id: bookmarkDelegate
      Rectangle {
        id: bookmarkitem
        property bool pressed: false
        property bool active: true
        opacity: active ? 1.0 : 0.5
        width:length*20 > container.itemWidth? container.itemWidth:length*20 
        height: parent.height - 6
        radius: 20 
        anchors.verticalCenter: parent.verticalCenter
        clip: true

        gradient: Gradient {
          GradientStop { position: bookmarkitem.pressed ? 1.0 : 0.0; color: "lightgray" }
          GradientStop { position: bookmarkitem.pressed ? 0.0 : 1.0; color: "gray" }
        }

        Image {
          id: borderleft
          source: "image://themedimage/browser/btn_bookmarkitem_left"
          anchors.left: parent.left
          height: parent.height
          width: 15
        }
        Image {
           id: bordermiddle
           anchors.left: borderleft.right
           anchors.right: borderright.left
           source: "image://themedimage/browser/btn_bookmarkitem_middle"
           height: parent.height
        }
        Image {
          id: borderright
          anchors.right: parent.right
          source: "image://themedimage/browser/btn_bookmarkitem_right"
          height: parent.height
          width:15
        }

        Text {
          id: texttitle
          anchors.fill: parent
          anchors.leftMargin: 10
          anchors.rightMargin: 10
          color: "black"
          elide: Text.ElideRight
          verticalAlignment: Text.AlignVCenter
          horizontalAlignment: Text.AlignHCenter
          font.pixelSize: container.height * 0.5
          text: title
          wrapMode: Text.NoWrap
          clip: true
        }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            container.model.openBookmarkItem(index);
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
      anchors.leftMargin: 5
      anchors.rightMargin: 5
      spacing: 10
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
    onShowInstruction: instruction.opacity = 1
    onHideInstruction: instruction.opacity = 0
  }
}

