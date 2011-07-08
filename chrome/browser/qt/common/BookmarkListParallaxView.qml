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

/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
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
** $QT_END_LICENSE$
**
****************************************************************************/

import Qt 4.7
import Qt.labs.gestures 2.0

Item {
  id: parallaxView

  property int currentIndex: 0
  default property alias content: visualModel.children

  MouseArea {
    anchors.fill: parent
    acceptedButtons: Qt.LeftButton | Qt.RightButton
  }

  GestureArea {
    anchors.fill: parent
    Tap {}
    TapAndHold {}
    Pan {}
    Pinch {}
    Swipe {}
  }

  Rectangle { color: "white"; anchors { top: parent.top; bottom: parent.bottom; left: selectorContainer.right; right: parent.right; } }
  Item {
    id: listContainer
    width: parent.width/4 * 3
    height: parent.height
    //anchors { top: parent.top; topMargin: bmGlobal.leftMargin; left: selectorContainer.right; leftMargin: bmGlobal.leftMargin }
    anchors { top: parent.top; left: selectorContainer.right; }

    ListView {
      id: list
      anchors.fill: parent
      interactive: bmGlobal.gridShow && (barContainer.interactive && othersContainer.interactive) //!stateGlobal.enableDrag

      currentIndex: parallaxView.currentIndex
      onCurrentIndexChanged: parallaxView.currentIndex = currentIndex

      orientation: Qt.Horizontal
      boundsBehavior: bmGlobal.gridShow ? Flickable.DragOverBounds : Flickable.DragAndOvershootBounds
      model: VisualItemModel { id: visualModel }

      highlightRangeMode: ListView.StrictlyEnforceRange
      highlightMoveDuration: bmGlobal.gridShow ? 200 : 1
      snapMode: ListView.SnapOneItem
    }
  }

  Item {
    id: selectorContainer
    width: parent.width/4; height: parent.height
    anchors { left: parent.left; top: parent.top }
    //Rectangle { color: "#e5e5e5"
    Image {
      anchors.fill: selectorContainer
      source: "image://themedimage/images/bg_application_p"
//      source: "image://themedimage/widgets/apps/browser/bookmark-manager-background"//; smooth: true
    }
    ListView {
      id: selector
      anchors.fill: parent
      interactive: false

      Image {
        id: shadowtab; 
        source: "image://themedimage/widgets/apps/browser/bookmark-manager-shadow-tab"//; smooth: true
        anchors { right: selector.right }
        y: 8
        Behavior on y { NumberAnimation { duration: 150; } }
      }

      currentIndex: parallaxView.currentIndex
      onCurrentIndexChanged: {
        shadowtab.y = currentIndex * bmGlobal.listHeight + 8
        parallaxView.currentIndex = currentIndex
        parallax.barContainer.enabled = !currentIndex
      }

      model: visualModel.children
      delegate: Item {
        id: delegateparallaxView
        height: bmGlobal.listHeight; width: selector.width

        Item {
          id: folderItemContainer
          anchors.fill: parent
          Rectangle {
            id: folderItem;
            color: "#e5e5e5"
            width: parent.width
            height: bmGlobal.listHeight-2
            Text {
              id: folderItemText
              text: modelData.text; elide: Text.ElideRight
              font { pixelSize: theme_fontPixelSizeNormal; family: theme_fontFamily }
              anchors { left: parent.left; leftMargin: bmGlobal.leftMargin; verticalCenter: parent.verticalCenter }
            }
            MouseArea {
              anchors.fill: parent
              onClicked: { parallaxView.currentIndex = index }
            }
          }
          Rectangle { id: wedge; color: "#bac4c8"; height: 1; anchors { top: folderItem.bottom; left: parent.left; right: parent.right } }
          Rectangle {            color: "#f6f6f6"; height: 1; anchors { top: wedge.bottom;      left: parent.left; right: parent.right } }
        }

        states: State {
          name: "Selected"
          when: delegateparallaxView.ListView.isCurrentItem == true
          PropertyChanges {
            target: folderItem
            color: "#eaf6fb"
          }
        }
        transitions: Transition {
          NumberAnimation { properties: "scale,y, opacity" }
        }
      }

      //        Rectangle {
      //            color: "gray"
      //            /*x: -selector.iSpace/2; */ y: -3; z: -1
      //            width: parent.width + selector.iSpace; height: parent.height
      //            radius: 10
      //            opacity: 0.8
      //        }
    }
  }
}
