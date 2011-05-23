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
import MeeGo.Components 0.1
import Qt.labs.gestures 2.0

Component {
  Item {
    id: wrapper
    height: bmGlobal.listHeight; width: tree.width //960//barRoot.width //tree.width
    Rectangle {
      id: wrapperItem; parent: tree //treeMouseArea
      width: wrapper.width; height: wrapper.height
      x: wrapper.x - tree.contentX; y: wrapper.y - tree.contentY
      color: level==0 ? "#ecf5f9" : "transparent"
      Rectangle { height: 1; color: "#d0d0d0"; anchors { top: wrapperItem.bottom; left: parent.left; right: parent.right } }
      Item {
        id: levelMargin //(level>5?6:level)*50 + 5
        width: level==0 ? 5 : (level>5?5:level-1)*50 + 5
        anchors { left: parent.left; leftMargin: bmGlobal.leftMargin; verticalCenter: parent.verticalCenter }
      }
      Text {
        id: titleElement
        text: title
        anchors { left: levelMargin.right; verticalCenter: parent.verticalCenter }
      }
      //Rectangle { color: "lightgreen"
      Item {
        id: openElement
        anchors.fill: parent
        Item {
          anchors { right: parent.right; verticalCenter: parent.verticalCenter }
          height: parent.height; width: height
          state: "leafNode"
          Image { id: openImage }
        }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            console.log("hdq folder icon clicked ", isOpened, index);
            (isOpened) ? bookmarkAllTreesModel.closeItem(index) : bookmarkAllTreesModel.openItem(index)
          }
        }
      }
      states: [
        State { name: "active"; when: bmGlobal.dragging && treeMouseArea.currentId == bookmarkId;
          PropertyChanges { target: wrapperItem; y: treeMouseArea.mouseY - height/2; }},
        State { name: "leafNode"; when: !hasChildren;
          PropertyChanges { target: openElement; visible: false; } 
          PropertyChanges { target: openContainer; visible: true; } },
        State { name: "openedNode"; when: (hasChildren)&&(isOpened);
          PropertyChanges { target: openContainer; visible: false; } 
          PropertyChanges { target: openElement; visible: true; }
          PropertyChanges { target: openImage; source: "image://themedimage/images/browser/btn_findbar_next" } },
        State { name: "closedNode"; when: (hasChildren)&&(!isOpened);
          PropertyChanges { target: openContainer; visible: false; } 
          PropertyChanges { target: openElement; visible: true; }
          PropertyChanges { target: openImage; source: "image://themedimage/images/browser/btn_findbar_prev" } }
      ]
      Rectangle { 
        id: openContainer
        color: "transparent"
        anchors { left: parent.left; right: dragElement.left; }
        height: parent.height
        MouseArea {
          anchors.fill: parent
          onClicked: {
            console.log("hdq will open id", bookmarkId);
            tree.model.openBookmarkItem(bookmarkId);
          }
          onPressAndHold: {
            var map = mapToItem(parallax, mouseX, mouseY);
            bmGlobal.idxHasMenu = tree.indexAt(map.x-tree.width*(1-bmGlobal.parallaxWidthFactor), map.y-headHeight)
            bmGlobal.idHasMenu = bookmarkId
            bmGlobal.currentTitle = title
            bmGlobal.currentUrl = url
            tree.model.popupMenu(map.x, map.y)
            console.log("hdq press and hold on idx ", bmGlobal.idxHasMenu, map.x, map.y, "id is ",bookmarkId)
          }
        }
      }
      Item {
        id: dragElement
        visible: level!=0
        height: parent.height; width: height
        anchors { right: parent.right; rightMargin: bmGlobal.leftMargin; verticalCenter: parent.verticalCenter }
        Image { source: "image://themedimage/images/browser/icn_toolbar_back_forward_up" }
      }

      Behavior on x {
          enabled: treeMouseArea.currentId != -1 && item.state != "active";
          NumberAnimation { duration: 250; easing.type: Easing.OutQuart }
      }
      Behavior on y {
          enabled: treeMouseArea.currentId != -1 && item.state != "active";
          NumberAnimation { duration: 250; easing.type: Easing.OutQuart }
      }
      //states:
      //  State {
      //    name: "active"; when: bmGlobal.dragging && treeMouseArea.currentId == bookmarkId;
      //    PropertyChanges { target: wrapperItem; y: treeMouseArea.mouseY - height/2; }
      //}
    }
  }
}
