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
import MeeGo.Labs.Components 0.1
import Qt.labs.gestures 2.0

Item {
  id: container
  width: 320; height:  320
  property int itemWidth: 240
  property int itemMargin: 10
  property alias model: grid.model
  property alias interactive: grid.interactive

  GridView {
    id: grid
    interactive: gridMouseArea.currentId == -1
    anchors.centerIn: parent
    cellWidth: itemWidth; cellHeight: itemWidth/3*2
    width: parent.width; height: parent.height
    //model: BookmarkListModel { id: defaultModelforTest }
    delegate: BookmarkListGridItem { }

    GestureArea {
      anchors.fill: parent
      Tap {}
      TapAndHold {}
      Pan {}
      Pinch {}
      Swipe {}
    }


    MouseArea {
      anchors.fill: parent
      acceptedButtons: Qt.LeftButton | Qt.RightButton
      property int currentId: -1  // Original ID in model
      property int newIndex       // Current Position in model
      property int index          // Item underneath cursor
      property int oldIndex       // Start position on moving
      id: gridMouseArea
      onPressAndHold: {
        index = grid.indexAt(mouseX, mouseY)
        if (index < 0) return
        oldIndex = index
        currentId = model.id(newIndex = index)
      }
      onReleased: {
        if (currentId == -1) return
        currentId = -1
        model.moveDone(oldIndex, newIndex)
      }
      onMousePositionChanged: {
        index = grid.indexAt(mouseX, mouseY)
        if (gridMouseArea.currentId != -1 && index != -1 && index != newIndex) {
          model.moving(newIndex, newIndex = index) // This wouldn't affect model until moveDone() called
        }
      }
    }
  }

  Connections {
    target: grid.model
    onMoveToAnother: {
      grid.model.moveToAnotherFolder(bmGlobal.idHasMenu);
    }
    onRemoveItem: {
      dialogLoader.sourceComponent = bmItemDeleteDialog
      dialogLoader.item.parent = bmlistContainer
      bmGlobal.currentModel = grid.model
    }
    onEditItem: {
      dialogLoader.sourceComponent = bmItemEditDialog
      dialogLoader.item.parent = bmlistContainer
      bmGlobal.currentModel = grid.model
    }
  }
}

