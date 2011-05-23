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

//Rectangle {  // for test in qmlviwer
//  width: 800; height: 600 // for test in qmlviewer
Item {
  id: treeContainer
  anchors.fill: parent
  property alias model: tree.model

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

  ListView {
    id: tree
    anchors.fill: parent
    delegate: BookmarkListTreeItem {}
    //model: bookmarkTreeModel
    interactive: treeMouseArea.currentId == -1
    //Rectangle { color: "green"
    Item {
      width: bmGlobal.listHeight //\TODO dragElement.width
      height: bmGlobal.listHeight*tree.count
      anchors { top: parent.top; right: parent.right }//; rightMargin: bmGlobal.leftMargin }
      MouseArea {
        id: treeMouseArea
        //anchors.fill: parent
        anchors.fill:parent

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property int currentId: -1  // Original ID in model
        property int newIndex       // Current Position in model
        property int index          // Item underneath cursor
        property int oldIndex       // Start position on moving
        onPressed: {
          index = tree.indexAt(mouseX, mouseY)
          if (index < 0) return
          if (0 == model.level(index)) return; // forbid "bar"/"others" moving
          bmGlobal.dragging = true
          oldIndex = index
          currentId = model.id(newIndex = index)
        }
        onReleased: {
          bmGlobal.dragging = false
          if (currentId == -1) return
          //console.log("hdq... moveDone", currentId, model.id(newIndex))
          //model.moveDone(currentId, model.id(newIndex))
          currentId = -1
          //model.moveDone(oldIndex, newIndex)
        }
        onMousePositionChanged: {
          index = tree.indexAt(mouseX, mouseY)
          if (treeMouseArea.currentId != -1 && index != -1 && index != newIndex) {
            //model.moving(newIndex, newIndex = index) // This wouldn't affect model until moveDone() called
            console.log("hdq...... move", newIndex, "-->", index, "id of ", model.id(newIndex), model.id(index));
            model.moveDone(newIndex, index, model.id(newIndex), model.id(newIndex = index))
          }
        }
      }
    }
  }

  Connections {
    target: bmGlobal.portrait ? bookmarkBarListModel : tree.model   // connects with menu->filter_
//    onMoveToAnother: { tree.model.moveToAnotherFolder(bmGlobal.idxHasMenu); }
    onOpenItemInNewTab: {
      console.log("hdq opening item of ", bmGlobal.idHasMenu)
      tree.model.openBookmarkItem(bmGlobal.idHasMenu); }
    onRemoveItem: {
      bmItemDeleteDialog.show()
      bmGlobal.currentModel = tree.model
    }
    onEditItem: {
      console.log("hdq editing item of ", bmGlobal.idHasMenu)
      bmItemEditDialog.show()
      bmGlobal.currentModel = tree.model
    }
  }

}
