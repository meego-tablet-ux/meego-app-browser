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
import MeeGo.Components 0.1

Item {
  id: container

  // hardcoded property, should be replaced by property from themedata
  property color suggestionColor: "#4e4e4e"
  property int suggestionItemHeight: 57
  property int maxPopupHeight: suggestionItemHeight*10  // at most 10 items
  // 

  property alias model: view.model
  property int itemHeight: suggestionItemHeight
  property int maxHeight: maxPopupHeight
  //width: parent.width

  // Space left for autocomplete popup view when taking vkb's area into account
  property int spaceOutOfVkb: maxHeight

  //The margin size of menu items
  property int textMargin: 16
 
  Image {
    id: finger
    source: "image://themedimage/images/popupbox_arrow_top"
    anchors.top: parent.top
    anchors.horizontalCenter: parent.horizontalCenter
  }

  Item {
    id: menu
    width: parent.width
    height: Math.min (container.maxHeight, view.count * container.itemHeight, 
                        spaceOutOfVkb - spaceOutOfVkb % container.itemHeight)
    anchors.top: finger.bottom
    clip: true

	BorderImage {
    	id: borderImage1
	    source: "image://themedimage/images/popupbox_1"
		border.left: 10
		border.right: 10
		//border.top: 5
		width: parent.width
		anchors.top: parent.top
	}
	
    BorderImage {
		anchors.top: borderImage1.bottom
		anchors.bottom: borderImage2.top
		source: "image://themedimage/images/popupbox_2"
		verticalTileMode: BorderImage.Repeat
		width: parent.width
		clip: true
		height: parent.height - borderImage1.height - borderImage2.height
		border.left: 10
		border.right: 10
	}
	
	BorderImage {
		id: borderImage2
		anchors.bottom: parent.bottom
		source: "image://themedimage/images/popupbox_3"
		width: parent.width
		border.left: 10
		border.right: 10
		//border.bottom: 34
	}

    Component {
      id: suggestionDelegate
      Item {
        width: parent.width;
        height: urltext.paintedHeight + textMargin*2
        opacity: 1
          Text {
            id: urltext
	    x: textMargin
            width: parent.width-textMargin*2
            height: parent.height - 1
            color: theme_contextMenuFontColor
	    elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: theme_contextMenuFontPixelSize
            text: url
          }
	  Image {
            id: divider
            anchors.top : urltext.bottom
            width: parent.width
            height: 1
            source: "image://themedimage/images/menu_item_separator"
            visible: index < view.count -1
          }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            container.model.openLine(line);
          }
        }
      }
    }

    ListView {
      id: view
      anchors.fill: parent
      delegate: suggestionDelegate
      model: autocompletePopupViewModel
      focus: false
      opacity: 0.5
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
