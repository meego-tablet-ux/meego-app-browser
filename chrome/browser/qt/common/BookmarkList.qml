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
  id: bookmarkListRoot
  anchors.fill: parent

  property bool portrait: false
  property int bottomMargin: 20
  property int searchMargin: 20
  property int headTextPixel: 21
  property int headTextHeight: 50
  property int headHeight: 115
  property alias textFocus: searchBox.textFocus

  Item {
    id: bmGlobal
    property bool portrait: bookmarkListRoot.portrait
    property bool dragging: false
    property bool exiting: false
    property bool exitDone: false
    //property int idxHasMenu: -1
    property int idHasMenu: -1
    property string currentTitle: ""
    property string currentUrl: ""
    property string currentFolderName: ""
    property variant currentModel
    property bool gridShow: false
    property int listHeight: 55
    property int leftMargin: 20
    property real parallaxWidthFactor: 0.75
    property bool hasSearchText: searchBox.text != ""
  }

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

  Loader {
    id: listLoader
    parent: bookmarkListRoot
    width: parent.width; height: parent.height - topContainer.height
    anchors { top: topContainer.bottom }
    source: portrait ? "BookmarkListTreeAll.qml" : bmGlobal.gridShow ? "BookmarkListGridList.qml" : "BookmarkListTreeList.qml"
    onLoaded: textChange(searchBox.text)
  }

  Item {
    id: topContainer
    width: parent.width
    height: headHeight
    MouseArea { anchors.fill: parent; onClicked: bmItemEditDialog.show() }
    Image {
      anchors.fill: parent
      source: "image://themedimage/images/bg_application_p"
    }

    Item {
      id: headTextContainer
      anchors { left: parent.left; leftMargin: bmGlobal.leftMargin; top: parent.top }
      width:head.width; height: headTextHeight
      Text {
        id: head
        anchors { verticalCenter: parent.verticalCenter }
        text: bookmarkManagerTitle
        font { pixelSize: headTextPixel }
      }
    }

    TextEntry {
      id: searchBox
      width: parent.width / 2
      height: headHeight - headTextHeight - bottomMargin
      anchors { top: headTextContainer.bottom; left: parent.left; leftMargin: bmGlobal.leftMargin } //verticalCenter: backButton.verticalCenter }
      defaultText: bookmarkManagerSearchHolder
      onTextChanged: textChange(text)
    }

    Rectangle {            color: "#bac4c8"; height: 1; anchors { bottom: wedge.top;           left: parent.left; right: parent.right } }
    Rectangle { id: wedge; color: "#ebebeb"; height: 1; anchors { bottom: topContainer.bottom; left: parent.left; right: parent.right } }
  }

  function textChange(text) {
    if (portrait) {
      listLoader.item.model.textChanged(text);
    } else {
      listLoader.item.barContainer.model.textChanged(text);
      listLoader.item.othersContainer.model.textChanged(text);
    }
  }

  ModalDialog {
    id: bmItemDeleteDialog
    title: qsTr("Delete bookmark")
    acceptButtonText: qsTr("Delete")
    acceptButtonImage:        "image://themedimage/widgets/common/button/button-negative"
    acceptButtonImagePressed: "image://themedimage/widgets/common/button/button-negative-pressed"
    cancelButtonText: qsTr("Cancel")
    content: Text {
      text: qsTr("Are you sure you want to delete this bookmark?"); //\"" + bmGlobal.currentTitle.toString().substring(0,30) + "\"?");
      anchors.fill: parent
      anchors.margins: bmGlobal.leftMargin
      wrapMode: Text.WordWrap
    }
    onAccepted: {
      bmGlobal.currentModel.remove(bmGlobal.idHasMenu)
    }
  }

  property string tmpTitle: ""
  property string tmpUrl: ""
  property int tmpFolderIdx: -1
  BookmarkListEditDialog {
    id: bmItemEditDialog
    h: folderGroup.expanded ? innerHeight+folderHeight : innerHeight
    title: qsTr("Edit bookmark"); //\"" + bmGlobal.currentTitle.toString().substring(0,30) + "\"");
    showCancelButton: false
    showAcceptButton: false
    property int innerHeight: 330
    property int folderHeight: 100
    buttonRow: Row {
      id: row
      anchors.verticalCenter: parent.verticalCenter
      spacing: 10
      Button {
        id: buttonDone
        text: qsTr("Done")
        bgSourceUp: "image://themedimage/widgets/common/button/button-default"
        bgSourceDn: "image://themedimage/widgets/common/button/button-default-pressed"
        onClicked: {
          if (tmpFolderIdx != -1) bmGlobal.currentModel.folderChanged(bmGlobal.idHasMenu, tmpFolderIdx)
          if (tmpTitle != "") bmGlobal.currentModel.titleChanged(bmGlobal.idHasMenu, tmpTitle);
          if (tmpUrl != "") bmGlobal.currentModel.urlChanged(bmGlobal.idHasMenu, tmpUrl);
          bmItemEditDialog.hide()
        }
      }
      Item { height: 10; width: bmItemEditDialog.width - buttonDone.width - buttonCancel.width - buttonDelete.width - 70 } // wedge
      Button {
        id: buttonCancel
        text: qsTr("Cancel")
        anchors.verticalCenter: parent.verticalCenter
        onClicked: {
          bmItemEditDialog.rejected()
          bmItemEditDialog.hide()
        }
      }
      Button {
        id: buttonDelete
        anchors.verticalCenter: parent.verticalCenter
        text: qsTr("Delete")
        bgSourceUp: "image://themedimage/widgets/common/button/button-negative"
        bgSourceDn: "image://themedimage/widgets/common/button/button-negative-pressed"
        onClicked: {
          bmItemEditDialog.hide()
          bmItemDeleteDialog.show()
        }
      }
    }
    content: Item {
      id: bmContent
      width: parent.width
      TextEntry {
        id: titleEditor
        anchors { top: bmContent.top; topMargin: bmGlobal.leftMargin/2; left: bmContent.left; leftMargin: bmGlobal.leftMargin; }
        width: parent.width - bmGlobal.leftMargin*2; height: 50; focus: true
        //defaultText: bmGlobal.currentTitle
        text: bmGlobal.currentTitle
        onTextChanged: tmpTitle = text
      }
      TextEntry {
        id: urlEditor
        anchors { top: titleEditor.bottom; topMargin: bmGlobal.leftMargin/2; left: titleEditor.left }
        width: titleEditor.width; height: titleEditor.height; focus: true
        text: bmGlobal.currentUrl
        onTextChanged: tmpUrl = text
      }
      BookmarkListGroupBox {
        id: folderGroup
        anchors { top: urlEditor.bottom; topMargin: bmGlobal.leftMargin/2; left: bmContent.left; leftMargin: 16 }
        infoText: bmGlobal.currentFolderName
        width: titleEditor.width + 8
        innerHeight: 100
        itemHeight: 50
        loader.sourceComponent: folderCombo
        Component {
          id: folderCombo
          ListView {
            id: view
            interactive: false
            delegate: Column {
              id: folderItem
              Text {
                text: modelData
                width: titleEditor.width
                height: folderGroup.itemHeight
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 16
                color: "#909090"
                MouseArea {
                  anchors.fill: parent
                  onClicked: {
                    bmGlobal.currentFolderName = modelData;
                    folderGroup.state = "elapsed"
                    tmpFolderIdx = index
                  }
                }
              }
            }
            model: bookmarkAllFolders
            focus: true
            orientation: ListView.Vertical
          }
        }
      }
    }
  }
}
