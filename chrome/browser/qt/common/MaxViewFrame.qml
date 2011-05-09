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
//import QtQuick 1.0
import Qt.labs.gestures 2.0

Item {
    id: sector
    property int topMargin: 0
    property int itemWidth: 210
    property int itemHeight: 130
    property int itemHMargin: 8
    property int itemVMargin: 8
    property int visibleRow: 2
    property bool collapseState: false
    property bool enableDrag: false
    property alias categoryTitle: title.text
    property alias categoryBottom: category.bottom
    property QtObject gridModel
    property bool gridScrollable: false
    width: maxViewLoader.width
    height: maxViewLoader.height + category.height

    Image {
      id: sector_bg
      anchors.fill: parent
      fillMode: Image.Stretch
      source: "image://themedimage/images/bg_application_p"
    }
 
    Loader {
      id: maxViewLoader
      anchors.top: category.bottom
      anchors.left: parent.left
      anchors.topMargin: 5
      width: item.viewWidth
      height: item.viewHeight
      sourceComponent: maxView
    }

    Image {
	id: category
	anchors.topMargin: topMargin
	anchors.top: parent.top
	anchors.left: parent.left
	width: parent.width
	height: title.height
        source: "image://themedimage/images/bg_application_p"
	
	Image {
	    id: arrow
     	    anchors.verticalCenter: title.verticalCenter
	    anchors.left: parent.left
	    source: "image://themedimage/images/notes/icn_dropdown_off"
	}
	
	Text {
	    id: title
	    anchors.top: parent.top
	    anchors.left: arrow.right
	    anchors.leftMargin: 20
            font.weight: Font.Bold
            font.pixelSize: 18
	    font.family: "Droid Sans"
	    color: "#4e4e4e"
	}

	Rectangle {
	    id: line
	    anchors.left: title.right
	    anchors.leftMargin: 20
     	    anchors.verticalCenter: title.verticalCenter
	    width: sector.width - title.width - arrow.width - 40 - sector.itemHMargin*2
	    height: 2
	    color: "#4e4e4e"
	    //radius: 1
	}
	states: [
	    State {
		name: "highlight"
		when: mouseArea.pressed
		PropertyChanges { target: title; color: "#7e7e7e" }
		PropertyChanges { target: line; color: "#7e7e7e" }
	    }
	]

        MouseArea {
	  id: mouseArea
          anchors.fill: category
          onClicked: {
	    mouse.accepted = true;
	    collapseState = !collapseState;
	    gridModel.setCollapsedState(collapseState);
	    //collapseMaxView();
          }
          onPressed: { 
	    sector.focus = true;
            mouse.accepted = true;
	  }
          onReleased: { 
	    mouse.accepted = true;
	  }

        }
    }

    states: [
	State {
	    name: "collapsed"
	    when: collapseState == true
	    PropertyChanges { target: maxViewLoader; sourceComponent: miniView }
	    PropertyChanges { target: arrow; rotation: 270 }
	},
	State {
	    name: "non-collapsed"
	    when: collapseState == false
	    PropertyChanges { target: maxViewLoader; sourceComponent: maxView }
	    PropertyChanges { target: arrow; rotation: 0 }
	}
    ]

/*
    function collapseMaxView() {
	collapseState = !collapseState;
	if(collapseState) {
	    maxViewLoader.sourceComponent = miniView;
	    arrow.rotation = 270;
	}
	else {
	    maxViewLoader.sourceComponent = maxView;
	    arrow.rotation = 0;
	}

        console.log("state: " + collapseState);
    }
*/
    Component {
	id: maxView
	Item {
	    property alias viewWidth: grid.width
	    property alias viewHeight: grid.height
	    states: [
		State {
		    name: "landscape"
		    when: scene.orientation == 1 || scene.orientation == 3
		    PropertyChanges {
			target: grid
			width: itemWidth*4 + itemHMargin*8   //4 coloums
			height: (sector.visibleRow == 2)?(itemHeight*2 + itemVMargin*4):(itemHeight + itemVMargin*2)
		    }
		},

		State {
		    name: "portrait"
		    when: scene.orientation == 2 || scene.orientation == 0
		    PropertyChanges {
			target: grid
			width: (newtab.width > (itemWidth*3 + itemHMargin*6))?(itemWidth*3 + itemHMargin*6):(itemWidth*2 + itemHMargin*4)
			height: (sector.visibleRow == 2)?((newtab.width > (itemWidth*3 + itemHMargin*2))?(itemHeight*3 + itemVMargin*6):(itemHeight*4 + itemVMargin*8)):
							 (((newtab.width > (itemWidth*3 + itemHMargin*2))?(itemHeight*3 + itemVMargin*6):(itemHeight*4 + itemVMargin*8))/2)
		    }
		}
	    ]

	    GridView {
		id: grid
		cellWidth: itemWidth + 2*itemHMargin
		cellHeight: itemHeight + 2*itemVMargin
		width: itemWidth*4 + itemHMargin*8   //4 coloums
		height: (sector.visibleRow == 2)?(itemHeight*2 + itemVMargin*4):(itemHeight + itemVMargin*2)
		delegate: MaxViewDelegate { }
		snapMode: GridView.SnapToRow
		model: sector.gridModel
		//model: MaxViewModel { }
		interactive: sector.gridScrollable

    		GestureArea {
        	    anchors.fill: parent
                    z: -6
        	    Tap {}
        	    TapAndHold {}
        	    Pan {}
        	    Pinch {}
    		    Swipe {}
    		}

		MouseArea {
		    anchors.fill: parent
		    property string longPressId: ""
		    property int newIndex:-1
		    property int oldIndex:-1
		    property int index:-1
		    property string pressId: ""
		    property bool draging: false
		    property string dragOverId: ""
		    id: gridMouseArea
                    z: -5
		    onClicked: {
			index = grid.indexAt(mouseX, mouseY);
			if(index != -1) {
	    		  scene.openWebPage(index, gridModel);
			}
		    }
		    onPressed: { 
		        sector.focus = true;
			index = grid.indexAt(mouseX, mouseY);
			if(index != -1) {
		          pressId = grid.model.getId(index);
			}
		    }
		    onPressAndHold: { 
			if(sector.enableDrag) {
			    index = grid.indexAt(mouseX, mouseY);
			    if(index != -1) {
			      oldIndex = newIndex = index;
			      longPressId = grid.model.getId(oldIndex);
			      //console.log("press and hold: " + longPressId);
			      draging = true;
			      grid.model.bringToFront(index);
			    }
			}
		    }
		    onReleased: {
			index = grid.indexAt(mouseX, mouseY);
			if(oldIndex != -1 && index != -1) {
			  grid.model.swap(oldIndex, index);
			}
			longPressId = "";
			dragOverId = "";
			newIndex = -1;
			oldIndex = -1;
			index = -1;
			pressId = "";
		    }
		    onMousePositionChanged: {
			//console.log("mouseX:" + mouseX + " mouseY:" + mouseY );
			index = grid.indexAt(mouseX, mouseY);
			if(index != -1 && index != newIndex && draging && longPressId != "") {
			  //console.log("index:" + index + "longPressId:" + longPressId + "newIndex: " + newIndex);
			  dragOverId = grid.model.getId(newIndex = index);			
			}
		    }
		}
	    }
	}
    }

    Component {
	id: miniView
	Item {
	    property alias viewWidth: list.width
	    property alias viewHeight: list.height

	    states: [
		State {
		    name: "landscape"
		    when: scene.orientation == 1 || scene.orientation == 3
		    PropertyChanges {
			target:list
			width: itemWidth*4 + itemHMargin*8   //4 coloums
			height: list.cellHeight * 2 + 15 //bottom margin
		    }
		},

		State {
		    name: "portrait"
		    when: scene.orientation == 2 || scene.orientation == 0
		    PropertyChanges {
			target: list
			width: (newtab.width > (itemWidth*3 + itemHMargin*6))?(itemWidth*3 + itemHMargin*6):(itemWidth*2 + itemHMargin*4)
			height: (newtab.width > (itemWidth*3 + itemHMargin*6))?(list.cellHeight * 3 + 15):(list.cellHeight * 4 + 15)
		    }
		}
	    ]
	    GridView {
		id: list
		cellWidth: itemWidth + 2*itemHMargin
		cellHeight: 50
      		anchors.bottomMargin: 15
		width: itemWidth*4 + itemHMargin*8   //4 coloums
		height: cellHeight * 2 + 15     //bottom margin
		delegate: MiniViewDelegate { }
		snapMode: GridView.SnapToRow
		model: sector.gridModel
		//model: MaxViewModel { }
		interactive: sector.gridScrollable
	    }
/*
	    ListView {
		id: list
		width: sector.itemWidth*4 + sector.itemHMargin*8   //4 coloums
      		anchors.bottomMargin: 5
		height: 50
		orientation: ListView.Horizontal
		delegate: MiniViewDelegate { }
		//delegate: Text { text: title }
		model: sector.gridModel
	    }
*/
	}
    }
}

