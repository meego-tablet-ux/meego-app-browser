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
import MeeGo.Components 0.1

Item {
  id: container
  height: parent.height
  property int popupHeight: 300
  property int popupDirection: 0
  property bool showStarButton: true
  signal activeFocusChanged(bool activeFocus)

  property int starButtonHeight: 45
  property int starButtonWidth: 40
  property int indicatorWidth: 40

  Row {
    anchors.fill: parent
    anchors.margins: 5
    
  Image {
    id: left
    height: parent.height
    anchors.verticalCenter: parent.verticalCenter
    source: "image://themedimage/images/browser/urlinputbar_left"
  }
  BorderImage {
    id: omniBox
    width: parent.width - left.width - right.width
    height: parent.height
    z: parent.z + 1 //to avoid hlight overlap by left and right
    anchors.verticalCenter: parent.verticalCenter
    horizontalTileMode: BorderImage.Stretch
    source: "image://themedimage/images/browser/urlinputbar_middle"
    Loader {
      id: autocompletePopupLoader
//      width: scene.isLandscapeView() ? parent.width : toolbar.width * 0.9
      width: parent.width
      height: popupHeight
      anchors.top: parent.bottom
      anchors.topMargin: -30 + 5 + 6 // 30: finger.height 5: bottomMargin of omnibox 6: margin below toolbar
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
        anchors.leftMargin: -3 // to cover left
        anchors.rightMargin: -3 // to cover right
        height: parent.height
        border.color: "#2CACE3"
        border.width:3
        radius:5
        visible:false
    }
    MouseArea {
      anchors.fill: parent
      anchors.topMargin: -5
      anchors.bottomMargin: -5
      onPressed: {
        if (!showqmlpanel) 
          urlTextInput.forceActiveFocus();
      }
    }
    Rectangle{
      id: sslArea
      height: parent.height - 12
      width:  {!showqmlpanel ? sslIcon.width+sslTitle.paintedWidth + widthFix:0}
      opacity: {!showqmlpanel ? 1:0}
      anchors.verticalCenter: parent.verticalCenter
      anchors.left: parent.left
      property int widthFix: 16
      anchors.leftMargin: 10
      anchors.rightMargin: 5
      radius: 5
      smooth: true
      BorderImage {
        id: sslBackground
        border.left: 6
        border.top: 0
        border.bottom: 0
        border.right: 6
        anchors.fill: parent
        smooth: true
      }
      Image {
        id: sslIcon
        height: omniBox.height/2
        width: height
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 7
      }
      Text{
        id:sslTitle
        anchors.left: sslIcon.right
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: theme_fontPixelSizeNormal
        anchors.leftMargin: 3
        anchors.rightMargin: 3
      }
   }
    TextInput {
      id: panelTitle
      anchors.left: sslArea.right
      anchors.right: starButton.left
      anchors.leftMargin: 5
      anchors.rightMargin: 5
      anchors.verticalCenter: parent.verticalCenter
      horizontalAlignment: TextInput.AlignLeft
      color: "gray"
      opacity: {!showqmlpanel ? 0:1}
      text: panelstring
      activeFocusOnPress: false
      font.pixelSize: theme_fontPixelSizeNormal
    }
    TextInput {
      id: urlTextInput
      objectName: "urlTextInput"
      anchors.left: sslArea.right
      anchors.right: starButton.left
      anchors.leftMargin: 5
      anchors.rightMargin: 5
      anchors.verticalCenter: parent.verticalCenter
      horizontalAlignment: TextInput.AlignLeft
      selectByMouse: true
      color: "gray"
      opacity: {!showqmlpanel ? 1:0}
      font.pixelSize: theme_fontPixelSizeNormal
      property bool isDelete: false
      property bool shouldSelectAll: false
      autoScroll: false
      inputMethodHints: Qt.ImhUrlCharactersOnly

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
          //starButton.width = 0;
          urlTextInput.autoScroll = false;
          hlight.visible = true;
        }
        else {
          autocompleteEditViewModel.focusLost();
          if (showStarButton) {
            starButton.width = starButtonWidth;
          }
          // let the title text left alignment
          urlTextInput.cursorPosition = 0
          urlTextInput.autoScroll = false;
          // close VKB
          urlTextInput.closeSoftwareInputPanel();
          hlight.visible = false;
        }
        container.activeFocusChanged(activeFocus);
      }
      Connections {
        target: autocompleteEditViewModel
        onSetFocus: urlTextInput.forceActiveFocus()
        onSetText: urlTextInput.text = text;
        onSetSelection: urlTextInput.select(start, end);
        onSelectAll: {
          if (urlTextInput.text.length > 0) {
            urlTextInput.selectAll();
            urlTextInput.shouldSelectAll = !urlTextInput.shouldSelectAll;
          }
        }
        onSetReadOnly: {urlTextInput.readOnly = readonly;}
        onSecurityUpdate: {
          if(state){
            sslIcon.width = sslIcon.height;
            sslTitle.text = domain;
            sslArea.widthFix = domain=="" ? 16 : 20
            if (state == 1){
              sslBackground.source = "image://themedimage/widgets/apps/browser/ssl-safe-url-background"
              sslTitle.color = "#1da611";
              sslIcon.source="image://themedimage/widgets/apps/browser/ssl-safe"
            }
            else{
              sslBackground.source = "image://themedimage/widgets/apps/browser/ssl-unsafe-url-background"
              sslTitle.color = "#d42f2f";
              if (state ==2)
                sslIcon.source="image://themedimage/widgets/apps/browser/close-small"
              else
                sslIcon.source="image://themedimage/widgets/apps/browser/close-small"
            }
            urlTextInput.anchors.leftMargin = 8;
          }
          else{
            sslIcon.width = 0;
            sslTitle.text= "";
            sslArea.widthFix = 0;
            urlTextInput.anchors.leftMargin = 5;
          }
        }
      }
    }
    Item {
      id: starButton
      objectName: "starButton"
      height: starButtonHeight
      width: {!showqmlpanel? starButtonWidth:0}
      opacity: {!showqmlpanel? 1:0}
      anchors.right: parent.right
      anchors.rightMargin: -1 * right.width
      Image {
        id: starIcon
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        source: "image://themedimage/images/browser/icn_favourite_off"
        property bool pressed: false
        states: [
          State {
            name: "pressed"
            when: starIcon.pressed
            PropertyChanges {
              target: starIcon
              source: "image://themedimage/images/browser/icn_favourite_on"
            }
          }
        ]
      }
      MouseArea {
        anchors.fill: parent
        onClicked: {
          var px = width/2;
          var py = height;

          if (popupDirection == 0) {
            py = starButtonHeight - (30/2 - 5 - 6) // 30: finger.height 5: bottomMargin of omnibox 6: margin below toolbar
          } else if (popupDirection == 1) {
            py = py + starButton.height
          }

          var map = mapToItem(scene, px, py);

           scene.lastMousePos.mouseX = map.x;
           scene.lastMousePos.mouseY = map.y;
           focus = true;
           browserToolbarModel.starButtonClicked();
        }
        onPressed: starIcon.pressed = true
        onReleased: starIcon.pressed = false
      }
      Connections {
        target: browserToolbarModel
        onUpdateStarButton: {
          if(is_starred)
            starIcon.source = "image://themedimage/images/browser/icn_favourite_on"
          else
            starIcon.source = "image://themedimage/images/browser/icn_favourite_off"
        }
        onShowStarButton: {
          if (!show) {
           //starButton.width = 0;
           //container.showStarButton = false;
          }
          else {
           starButton.width = starButtonWidth;
           container.showStarButton = true;
          }
        }
      }
    }

    Item {
      id: processContainer
      width: indicatorWidth
      height: parent.height
      anchors.right: parent.right
      anchors.rightMargin: -1 * right.width
      visible: false
      Spinner{
        id: processIndicator
        visible: false
        spinning: false
        onSpinningChanged: {
          if (!spinning)
            spinning = true;
        }

        Connections {
          target: browserToolbarModel
          onUpdateReloadButton: {
            if(is_loading){
              processContainer.visible = true;
              processIndicator.visible = true;
              processIndicator.spinning = true;
              starButton.visible = false;
              urlTextInput.anchors.right = processContainer.left
            }
            else{
              processContainer.visible = false;
              processIndicator.visible = false;
              processIndicator.spinning = false;
              starButton.visible = true;
              urlTextInput.anchors.right = starButton.left
            }
          }
        }
      }
    }//end of processContainer Item
  }

  Image {
     id: right
     height: parent.height
     source: "image://themedimage/images/browser/urlinputbar_right"
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
