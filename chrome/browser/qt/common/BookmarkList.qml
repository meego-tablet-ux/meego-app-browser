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
import MeeGo.Components 0.1
import Qt.labs.gestures 2.0

//Rectangle {  // for test in qmlviwer
//  width: 800; height: 600 // for test in qmlviewer
Item {
  id: bmlistContainer
  anchors.fill: parent

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

  Item {
    id: bmGlobal
    property bool dragging: false
    property bool exiting: false
    property bool exitDone: false
    property int idHasMenu: -1
    property int gridIdHasMenu: -1
    property string currentTitle: ""
    property string currentUrl: ""
    property variant currentModel
  }

  BookmarkListParallaxView {
    id: parallax
    width: bmlistContainer.width; height: bmlistContainer.height - headContainer.height
    anchors { top: topContainer.bottom }

    Item {
        property url icon: "image://themedimage/images/browser/icn_bookmarkbar"
        property string text: bookmarkBarFolderName
        width: parallax.width; height: parallax.height
        BookmarkListGridContainer {
          id: barContainer  // Items from bookmark bar
          model: bookmarkBarListModel
          anchors.fill: parent
        }
    }

    Item {
        property url icon: "image://themedimage/images/browser/icn_otherbookmarks"
        property string text: bookmarkBarOtherFolderName
        width: parallax.width; height: parallax.height
        BookmarkListGridContainer {
          id: othersContainer  // Items from bookmark folder others
          model: bookmarkOthersListModel
          anchors.fill: parent
        }
    }
  }

  Item {
    id: topContainer
    width: parent.width
    height: 60
    Rectangle {
      anchors.fill: parent
      color: "lightgray"
    }

    Item {
      id: backButton
      anchors { left: parent.left; top: parent.top; topMargin: 25 }
      width: 36; height: width
      property bool pressed: false
      Image {
        id: backButtonIcon
        anchors.centerIn: parent
        height: parent.height
        source: "image://themedimage/images/icn_toolbar_back_button_up"
        states: State {
          when: backButton.pressed
          PropertyChanges {
            target: backButtonIcon
            source: "image://themedimage/images/icn_toolbar_back_button_dn"
          }
        }
      }
    }
    MouseArea {
      anchors.fill: backButton
      onPressed: backButton.pressed = true
      onReleased: backButton.pressed = false
      onClicked: barContainer.model.backButtonTapped()
    }

    Item {
      id: headContainer
      anchors { left: backButton.right; leftMargin: 10; top: backButton.top }
      width: 270; height: parent.height
      Text {
        id: head
        width: parent.width; height: parent.height
        text: bookmarkManagerTitle
        font { bold: true; pixelSize: 28 }
      }
    }

    TextEntry {
      id: searchBox
      width: bmlistContainer.width-headContainer.width-backButton.width-40; height: headContainer.height*4/5
      anchors { left: headContainer.right; leftMargin: 10; verticalCenter: backButton.verticalCenter }
      defaultText: bookmarkManagerSearchHolder
      onTextChanged: {
        barContainer.model.textChanged(text);
        othersContainer.model.textChanged(text);
      }
    }
  }

  property string tmpTitle: ""
  property string tmpUrl: ""
  Loader {
    id: dialogLoader
  }
  Component {
    id: bmItemDeleteDialog
    ModalDialog {
      acceptButtonText: qsTr("Delete")
      cancelButtonText: qsTr("Cancel")
      title: qsTr("Are you sure you want to delete this bookmark?"); //\"" + bmGlobal.currentTitle.toString().substring(0,30) + "\"?");
      onAccepted: {
        bmGlobal.currentModel.remove(bmGlobal.idHasMenu)
      }
      onRejected: {
        dialogLoader.sourceComponent = undefined;
      }
    }
  }
 Component {
    id: bmItemEditDialog
    ModalDialog {
      acceptButtonText: qsTr("Save")
      cancelButtonText: qsTr("Cancel")
      title: qsTr("Edit bookmark"); //\"" + bmGlobal.currentTitle.toString().substring(0,30) + "\"");
      onAccepted: {
        if (button == 1) {
          if (tmpTitle != "") bmGlobal.currentModel.titleChanged(bmGlobal.gridIdHasMenu, tmpTitle);
          if (tmpUrl != "")   bmGlobal.currentModel.urlChanged  (bmGlobal.gridIdHasMenu, tmpUrl);
        }
      }
      onRejected: {
        dialogLoader.sourceComponent = undefined;
      }
      Component.onCompleted: {
        contentLoader.sourceComponent = bmEditorComponent;
      }
      Component {
        id: bmEditorComponent
        Item {
          id: bmEditorEntry
          anchors.fill: parent
          TextEntry{
            id: titleEditor
            width: parent.width; height: 50; focus: true
            //defaultText: bmGlobal.currentTitle
            text: bmGlobal.currentTitle
            onTextChanged: tmpTitle = text
          }
          TextEntry{
            id: urlEditor
            anchors { top: titleEditor.bottom; margins: 5 }
            width: parent.width; height: 50; focus: true
            text: bmGlobal.currentUrl
            onTextChanged: tmpUrl = text
          }
        }
      }
    }
  }
}
