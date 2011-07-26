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

  // hardcoded property, should be replaced by property from themedata
  property color suggestionColor: "#4e4e4e"
  property int suggestionItemHeight: 45
  property int maxPopupHeight: suggestionItemHeight*4  // at most 4 items
  // 

  property alias model: view.model
  property int itemHeight: suggestionItemHeight
  property int maxHeight: maxPopupHeight
  property int iconWidth: 40
  //width: parent.width

  // Space left for autocomplete popup view when taking vkb's area into account
  property int spaceOutOfVkb: maxHeight

  //The margin size of menu items
  property int textMargin: 20
  //inner menu shadow margin
  property int shadowMargin: 4
 
  Image {
    id: finger
    source: "image://themedimage/widgets/common/menu/default-dropdown-triangle-top"
    anchors.top: parent.top
    anchors.horizontalCenter: parent.horizontalCenter
    z: menu.z + 1  // finger needs to be drawn on top of the menu shadow
  }

  Item {
    id: menu
    width: parent.width
    height: shadowMargin*2 + viewContainer.height
    anchors.top: finger.bottom
    anchors.topMargin: -shadowMargin //eliminate the gap between finger and shadow margin

    ThemeImage {
        // menu shadow
        id: menuShadow
        source: "image://themedimage/widgets/common/menu/menu-background-shadow"
        anchors.fill: parent
        // menu background inside the shadow
        ThemeImage {
             id: menuBg
             source: "image://themedimage/widgets/common/menu/menu-background"
             anchors.fill: parent
             anchors.margins: shadowMargin
         }
    }

    Component {
      id: suggestionDelegate
      Item {
        width: parent.width;
        height: 45;
        opacity: 1
          Text {
            id: urltext
            height: parent.height - 1
            anchors.left : urlIconWrapper.right
            color: theme_blockColorHighlight
	        elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: theme_fontPixelSizeNormal
            text: url

            Component.onCompleted: {
                if (paintedWidth > parent.width - urlIconWrapper.width - textMargin ){
                    width = parent.width - urlIconWrapper.width - textMargin;
                }else{
                    width = paintedWidth;
                }
            }
          }
          
          Text {
            id: urlDescription
            height: parent.height - 1
            anchors.left: urltext.right
            anchors.right: parent.right
            anchors.rightMargin: textMargin;
            anchors.top: parent.top
            color: theme_contextMenuFontColor
	        elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: theme_fontPixelSizeNormal
            text: desc
          }
          
          Item {
            id: urlIconWrapper
            width: iconWidth
            height: parent.height - 1
            anchors.left: parent.left
            anchors.top: parent.top
            Image {
                id: urlIcon
                fillMode: Image.PreserveAspectFit
                anchors.left: parent.left;
                anchors.top: parent.top;
                source: icon == 0 ? "image://themedimage/widgets/apps/browser/web-favorite-small" :
                        icon == 1 ? "image://themedimage/widgets/apps/browser/web-favorite-small" :
                        icon == 2 ? "image://themedimage/widgets/apps/browser/search" :
                        "image://themedimage/widgets/apps/browser/favourite-inactive"; //icon == 4
            }
            Component.onCompleted: {
                urlIcon.anchors.leftMargin = (urlIconWrapper.width - urlIcon.paintedWidth)/2 + 3 // 3 is for the space that shadow border covers
                urlIcon.anchors.topMargin = (urlIconWrapper.height - urlIcon.paintedHeight)/2
            }
          }
    	  Image {
            id: divider
            anchors.top : urltext.bottom
            width: parent.width - shadowMargin
            anchors.horizontalCenter: parent.horizontalCenter
            height: 1
            source: "image://themedimage/widgets/common/menu/menu-item-separator"
            visible: index < view.count -1
          }
          MouseArea {
            anchors.fill: parent
            onClicked: {
                container.model.openLine(line);
                innerContent.forceActiveFocus();
            }
          }
      }
    }

    Item{
        id:viewContainer
        width: parent.width
        height: Math.min (container.maxHeight, view.count * container.itemHeight,
                        spaceOutOfVkb - spaceOutOfVkb % container.itemHeight)
        anchors.verticalCenter: parent.verticalCenter
        anchors.topMargin: shadowMargin
        anchors.leftMargin: shadowMargin
        ListView {
        id: view
        anchors.fill: parent
        delegate: suggestionDelegate
        model: autocompletePopupViewModel
        focus: false
        opacity: 0.5
        clip: true
        interactive: count > 4 ? true : false
      }
    }
  }

  /*Connections {
    target: autocompletePopupViewModel
    onShow: container.opacity = 1
    onHide: container.opacity = 0
  }*/

  Connections {
    target: mainWindow
    onVkbHeight: {
      var map = menu.mapToItem(scene, 0, 0)
      spaceOutOfVkb = screenHeight - map.y - height
    }
  }
}
