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

Item { 
    id: main
    width: list.cellWidth
    height: list.cellHeight

    Image { 
	id: favIcon
	anchors.left: parent.left
	anchors.verticalCenter: parent.verticalCenter
	source: favicon
	fillMode: Image.PreserveAspectFit
    }

    Text { 
	id: titleText
	anchors.left: favIcon.right
	anchors.verticalCenter: parent.verticalCenter
        anchors { leftMargin:15; rightMargin:25}
	width: parent.width - favIcon.width - 40   // add margin
	text: title
	elide: Text.ElideRight
	color: theme_fontColorNormal
	font.family: "Droid Sans"
	font.pixelSize: 18

        states:[ 
	  State {
            name: 'highlight'
	    when: mouseArea.pressed
            PropertyChanges {
               target: titleText;
               color: "red";
            }
          }
	]

    }

    MouseArea {
      id: mouseArea
      anchors.fill: main
      onClicked: {
	scene.openWebPage(index, gridModel);
	mouse.accepted = true;
      }
      onPressed: { 
	    console.log("Thumbnail: onPressed");
	    main.focus = true;
	    mouse.accepted = true;
      }
      onReleased: { 
	    titleText.color = theme_fontColorNormal
	    mouse.accepted = true;
      }
    }
    states: [
      State {
        name: "landscape"
        when: (scene.orientation == 1 || scene.orientation == 3) && index == 9
        PropertyChanges {
          target: main
          visible: true
        }
      },
      State {
        name: "portrait"
        when: (scene.orientation == 2 || scene.orientation == 0) && index == 9
        PropertyChanges {
          target: main
          visible: false
        }
      }
    ]

}
