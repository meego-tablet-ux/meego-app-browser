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

Item {
  id: container

  // hardcoded property, should be replaced by property from themedata
  property color suggestionColor: "#4e4e4e"
  property int suggestionItemHeight: 43
  property int maxPopupHeight: suggestionItemHeight*10  // at most 10 items
  // 

  property alias model: view.model
  property int itemHeight: suggestionItemHeight
  property int maxHeight: maxPopupHeight
  //width: parent.width

  Image {
	id: finger
	source: "image://theme/popupbox_arrow_top"
    anchors.top: parent.top
    anchors.horizontalCenter: parent.horizontalCenter
  }

  Item {
    id: menu
    width: parent.width
    height: {(view.count * container.itemHeight > container.maxHeight ? 
       	      container.maxHeight : view.count * container.itemHeight) }
    anchors.top: finger.bottom
    clip: true

	BorderImage {
    	id: borderImage1
	    source: "image://theme/dropdown_white_pressed_1"
		border.left: 20
		border.right: 20
		border.top: 5
		width: parent.width
		anchors.top: parent.top
	}
	
    BorderImage {
		anchors.top: borderImage1.bottom
		anchors.bottom: borderImage2.top
		source: "image://theme/dropdown_white_pressed_2"
		verticalTileMode: BorderImage.Repeat
		width: parent.width
		clip: true
		height: parent.height - borderImage1.height - borderImage2.height
		border.left: 20
		border.right: 20
	}
	
	BorderImage {
		id: borderImage2
		anchors.bottom: parent.bottom
		source: "image://theme/dropdown_white_pressed_3"
		width: parent.width
		border.left: 20
		border.right: 20
		border.bottom: 34
	}

    Component {
      id: suggestionDelegate
      Item {
        width: parent.width;
        height: container.itemHeight
        opacity: 1
          Text {
            id: urltext
            color: container.suggestionColor
			anchors.fill: parent
    		anchors.margins: 10
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: container.itemHeight * 0.6
            text: url
          }
/*          Image {
            id: divider
            source: "image://theme/contacts/contact_divider"
            anchors.top : urltext.bottom
          }*/
          Rectangle {
			anchors.fill: parent
			radius: 4
			//color: container.color
			opacity: 0.3
			border.width: 2
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

}
