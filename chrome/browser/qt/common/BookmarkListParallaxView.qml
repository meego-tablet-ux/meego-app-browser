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
  id: root

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

  Image {
    id: selectorPad
    width: parent.width
    height: 20
    anchors.top: selectorContainer.bottom
    source: "image://themedimage/bg_application_p"
  }

  Image {
    id: leftPad
    height: parent.height
    width: 10
    anchors.top: selectorContainer.bottom
    source: "image://themedimage/bg_application_p"
  }

  Item {
    id: listContainer
    width: parent.width
    height: parent.height - selectorContainer.height
    anchors.top: selectorPad.bottom 
    anchors.left: leftPad.right
    Image {
      anchors.fill: parent
      source: "image://themedimage/bg_application_p"
    }
    ListView {
      id: list
      anchors.fill: parent
      interactive: (barContainer.interactive && othersContainer.interactive) //!stateGlobal.enableDrag

      currentIndex: root.currentIndex
      onCurrentIndexChanged: root.currentIndex = currentIndex

      orientation: Qt.Horizontal
      boundsBehavior: Flickable.DragOverBounds
      model: VisualItemModel { id: visualModel }

      highlightRangeMode: ListView.StrictlyEnforceRange
      highlightMoveDuration: 200
      snapMode: ListView.SnapOneItem
    }
  }

  Item {
    id: selectorContainer
    property int iSize: 60
    property int iSpace: 20
    width: parent.width
    height: iSize + iSpace
    anchors.top: parent.top
    Rectangle {
      anchors.fill: parent
      color: "lightgray"
    }
    ListView {
      id: selector
      property int iTitleWidth: 220

      anchors { top: parent.top; margins: selectorContainer.iSpace }
      x: 10
      height: parent.height
      width: Math.min(count * (selectorContainer.iSize+iTitleWidth), parent.width - selectorContainer.iSpace) * 2
      interactive: width == parent.width - selectorContainer.iSpace
      orientation: Qt.Horizontal

      currentIndex: root.currentIndex
      onCurrentIndexChanged: root.currentIndex = currentIndex

      model: visualModel.children
      delegate: Item {
        id: delegateRoot
        width: selectorContainer.iSize + selector.iTitleWidth

        Image {
          id: image
          source: modelData.icon
          smooth: true
          scale: 0.6
        }
        Text {
          id: tabTitle
          anchors { left: image.right; verticalCenter: image.verticalCenter }
          text: modelData.text
          elide: Text.ElideRight
          width: selector.iTitleWidth - selectorContainer.iSpace/3
          color: "gray"
        }
        Rectangle {
          id: rect
          color: "gray"
          width: selectorContainer.iSize + selector.iTitleWidth - 5; height: selectorContainer.iSize - 6;
          z: -1; radius: 5; opacity: 0.2
        }

        MouseArea {
          anchors.fill: rect
          onClicked: { root.currentIndex = index }
        }

        states: State {
          name: "Selected"
          when: delegateRoot.ListView.isCurrentItem == true
          PropertyChanges {
            target: image
            scale: 0.85
          }
          PropertyChanges {
            target: tabTitle
            color: "white"
          }
          PropertyChanges {
            target: rect
            opacity: 0.75
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
