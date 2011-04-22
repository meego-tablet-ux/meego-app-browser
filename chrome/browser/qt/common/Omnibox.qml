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
  height: parent.height

  property int popupHeight: 300
  property int popupDirection: 0
  property bool showStarButton: true
  signal activeFocusChanged(bool activeFocus)

  Row {
    anchors.fill: parent
    anchors.margins: 7
    
  Image {
    id: left
    height: parent.height
    anchors.verticalCenter: parent.verticalCenter
    source: "image://themedimage/browser/urlinputbar_left"
  }
  BorderImage {
    id: omniBox
    width: parent.width - 10
    height: parent.height
    anchors.verticalCenter: parent.verticalCenter
    horizontalTileMode: BorderImage.Stretch
    source: "image://themedimage/browser/urlinputbar_middle"

    Loader {
      id: autocompletePopupLoader
//      width: scene.isLandscapeView() ? parent.width : toolbar.width * 0.9
      width: parent.width
      height: popupHeight
      anchors.top: parent.bottom
      anchors.topMargin: -8
      anchors.horizontalCenter: parent.horizontalCenter
      z: 10
    }
    Connections {
      target: autocompletePopupViewModel
      onShow: {
          autocompletePopupLoader.source = "AutocompletePopupView.qml"
          autocompletePopupLoader.item.opacity = 1
      }
      onHide: {
          autocompletePopupLoader.item.opacity = 0
      }
    }
    Rectangle{
        id: hlight
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height
        border.color: "#2CACE3"
        border.width:3
        radius:5
        visible:false
    }
    TextInput {
      id: urlTextInput
      objectName: "urlTextInput"
      anchors.left: parent.left
      anchors.right: starButton.left
      anchors.topMargin: 10
      anchors.rightMargin: 5
      //height: parent.height
      anchors.verticalCenter: parent.verticalCenter
      horizontalAlignment: TextInput.AlignLeft
      selectByMouse: true
      color: "gray"
      //font.pixelSize: height * 0.7
      font.pixelSize: theme_fontPixelSizeNormal
      property bool isDelete: false
      property bool shouldSelectAll: false
      autoScroll: false
      inputMethodHints: Qt.ImhUrlCharactersOnly
      z: parent.z+1

      Keys.onReturnPressed: {
        autocompleteEditViewModel.returnPressed();
      }
      Keys.onPressed: {
        if (event.key == Qt.Key_Backspace || event.key == Qt.Key_Delete)
          isDelete = true;
	if(activeFocus == true)
	  urlTextInput.autoScroll = true;
      }
      onTextChanged: {
	if(activeFocus == true)
	   urlTextInput.autoScroll = true ;
        autocompleteEditViewModel.textChanged(text, isDelete);
        isDelete = false;
      }
      onSelectedTextChanged: { 
	if (activeFocus == true)
	    urlTextInput.autoScroll = true ;                                                                      
        selectByMouse = !shouldSelectAll;
        if (shouldSelectAll == true) {
          selectAll();
          shouldSelectAll = false;
        }
      }
      onActiveFocusChanged: {
        if (activeFocus == true){
          autocompleteEditViewModel.focusGained();
          starButton.width = 0;
          starIcon.width = 0;
          urlTextInput.autoScroll = false;
	  hlight.visible = true;
	  left.visible = false;
	  right.visible = false;
	  urlTextInput.anchors.leftMargin = 5
        }
        else {
          autocompleteEditViewModel.focusLost();
          if (showStarButton) {
            starButton.width = starButton.height;
            starIcon.width = starButton.width;
          }
          // let the title text left alignment
          urlTextInput.cursorPosition = 0
          urlTextInput.autoScroll = false;
          // close VKB
          urlTextInput.closeSoftwareInputPanel();
	  hlight.visible = false;
	  left.visible = true;
	  right.visible = true;
        }
        container.activeFocusChanged(activeFocus);
      }
      Connections {
        target: autocompleteEditViewModel
        onSetFocus: urlTextInput.forceActiveFocus()
        onSetText: {urlTextInput.text = text;}
        onSetSelection: urlTextInput.select(start, end);
        onSelectAll: {
          if (urlTextInput.text.length > 0) {
            urlTextInput.selectAll();
            urlTextInput.shouldSelectAll = !urlTextInput.shouldSelectAll;
          }
        }
        onSetReadOnly: {urlTextInput.readOnly = readonly;}
      }
    }
    MouseArea {
      anchors.fill: parent
      anchors.topMargin: -5
      anchors.bottomMargin: -5
      onPressed: urlTextInput.forceActiveFocus();
    }
    Item {
      id: starButton
      objectName: "starButton"
      height: parent.height/2
      width: height
      anchors.verticalCenter: parent.verticalCenter
      anchors.right: parent.right
      anchors.leftMargin: 5
      anchors.rightMargin: 5
      Image {
        id: starIcon
        anchors.fill: parent
        source: "image://themedimage/browser/icn_favourite_off"
        fillMode: Image.Tile
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: starIcon.pressed
            PropertyChanges {
              target: starIcon
              source: "image://themedimage/browser/icn_favourite_on"
            }
          }
        ]
      }
      MouseArea {
        anchors.right: parent.right
        width: parent.width*2
        height: parent.height*2
        onClicked: {
          var px = 3* width / 4;
          var py = height;

          if (popupDirection == 0) {
            py = starButton.height
          } else if (popupDirection == 1) {
            py = py + starButton.height
          }

          var map = mapToItem(scene, px, py);

           scene.lastMousePos.mouseX = map.x;
           scene.lastMousePos.mouseY = map.y;
           console.log(map.x, map.y);
           browserToolbarModel.starButtonClicked();
        }
        onPressed: starIcon.pressed = true
        onReleased: starIcon.pressed = false
      }
      Connections {
        target: browserToolbarModel
        onUpdateStarButton: {
          if(is_starred)
            starIcon.source = "image://themedimage/browser/icn_favourite_on"
          else
            starIcon.source = "image://themedimage/browser/icn_favourite_off"
        }
        onShowStarButton: {
          if (!show) {
           starButton.width = 0;
           starIcon.width = 0;
           container.showStarButton = false;
          }
          else {
           starButton.width = starButton.height;
           starIcon.width = starButton.width;
           container.showStarButton = true;
          }
        }
      }
    }
  }

  Image {
     id: right
     height: parent.height
     source: "image://themedimage/browser/urlinputbar_right"
     anchors.verticalCenter: parent.verticalCenter
  }
  }
/*
  transitions: [
    Transition {
      reversible: true
      PropertyAnimation {
        properties: "anchors.left,width,opacity"
        duration: 500
      }
    }
  ]
*/
}
