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
//import QtQuick 1.0
import Qt.labs.gestures 2.0

Item {
    id: sector
    property int collapseState: 1
    property bool enableDrag: false
    property alias categoryTitle: title.text
    property alias categoryBottom: category.bottom
    property QtObject gridModel
    width: parent.width
    height: maxViewLoader.height + category.height + getGridTopMargin()
 
    Loader {
      id: maxViewLoader
      anchors.top: category.bottom
      anchors.left: parent.left
      anchors.topMargin: getGridTopMargin()
      width: parent.width
      height: item.viewHeight
      sourceComponent: maxView
    }

    function getGridTopMargin() {
      if(newtab.width > newtab.height) {
        if(newtab.width>1200)  //For 1280x800
           return 19;
        else if(newtab.width<1000)  //For handset
          return 12;
        else			//For 1024x600
          return 15;
      }else{
        if(newtab.width>750)  //For 1280x800
          return 19;
        else if(newtab.width<550)  //For handset
          return 12;
        else			//For 1024x600
          return 15;
      }
    }

    function getGridLeftMargin() {
      if(newtab.width > newtab.height) {
        if(newtab.width>1200)  //For 1280x800
           return 53;
        else if(newtab.width<1000)  //For handset
          return 32;
        else			//For 1024x600
          return 42;
      }else{
        if(newtab.width>750)  //For 1280x800
          return 50;
        else if(newtab.width<550)  //For handset
          return 16;
        else			//For 1024x600
          return 20;
      }

    }

    Image {
        id: category
        anchors.top: parent.top
	anchors.left: parent.left
	width: parent.width
	height: getCategoryHeight()
        source: "image://themedimage/widgets/common/backgrounds/content-subheader-reverse"
	
	Text {
            id: title
	    anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: theme_fontPixelSizeLarge
	    font.family: "Droid Sans"
	    color: theme_fontColorNormal
	}

        function getCategoryHeight() {
          if(newtab.width > newtab.height) {
            if(newtab.width>1200)  //For 1280x800
              return 50;
            else if(newtab.width<1000)  //For handset
              return 32;
            else			//For 1024x600
              return 40
          }else{
            if(newtab.width>750)  //For 1280x800
              return 50;
            else if(newtab.width<550)  //For handset
              return 32;
            else			//For 1024x600
              return 40;
          }
        }
/*
        Rectangle {
            id: line
            anchors.left: title.right
            anchors.leftMargin: 20
            anchors.verticalCenter: title.verticalCenter
            width: sector.width - title.width - arrow.width - 50
            height: 2
            color: theme_fontColorNormal
            //radius: 1
        }
*/

    }

    states: [
	State {
	    name: "non-collapsed"
	    when: collapseState == 1
            PropertyChanges { target: maxViewLoader; anchors.topMargin: getGridTopMargin(); sourceComponent: maxView }
	},
	State {
	    name: "list"
	    when: collapseState == 3
            PropertyChanges { target: maxViewLoader; anchors.topMargin: 0; sourceComponent: listView }
        }
    ]


    Component {
	id: maxView
	Item {
            id: maxViewRoot
	    property int viewHeight: grid.height
            property int itemWidth: getItemWidth();
            property int itemHeight: getItemHeight();
            property int itemHMargin: getItemHMargin();
            property int itemVMargin: itemHMargin

            function getItemWidth() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 225;
                else if(newtab.width<1000)  //For handset
                  return 144;
                else			//For 1024x600
                  return 180;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 225;
                else if(newtab.width<550)  //For handset
                  return 144;
                else			//For 1024x600
                  return 180;
              }
            }

            function getItemHeight() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 143;
                else if(newtab.width<1000)  //For handset
                  return 91;
                else			//For 1024x600
                  return 114;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 143;
                else if(newtab.width<550)  //For handset
                  return 91;
                else			//For 1024x600
                  return 114;
              }
            }

            function getItemHMargin() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 12;
                else if(newtab.width<1000)  //For handset
                  return 8;
                else			//For 1024x600
                  return 10;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 12;
                else if(newtab.width<550)  //For handset
                  return 8;
                else			//For 1024x600
                  return 10;
              }
            }

	    states: [
		State {
		    name: "landscape"
		    when: scene.orientation == 1 || scene.orientation == 3
		    PropertyChanges {
			target: grid
			width: cellWidth*5   //5 coloums
			height: cellHeight*2
		    }
		},

		State {
		    name: "portrait"
		    when: scene.orientation == 2 || scene.orientation == 0
		    PropertyChanges {
			target: grid
			width: cellWidth*3   //3 coloums
			height: cellHeight*3
		    }
		}
	    ]

	    GridView {
		id: grid
		cellWidth: itemWidth + itemHMargin
		cellHeight: itemHeight + itemVMargin
                anchors.left: parent.left
                anchors.leftMargin: sector.getGridLeftMargin()
                //anchors.horizontalCenter: parent.horizontalCenter
		width: cellWidth*5
		height: cellHeight*2
		delegate: MaxViewDelegate { }
		snapMode: GridView.SnapToRow
		model: sector.gridModel
		interactive: false

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
	    property int viewHeight: list.height
            property int itemWidth: getItemWidth();
            property int itemHeight: getItemHeight();
            property int itemHMargin: getItemHMargin();
            property int itemVMargin: itemHMargin

            function getItemWidth() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 225;
                else if(newtab.width<1000)  //For handset
                  return 144;
                else			//For 1024x600
                  return 180;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 225;
                else if(newtab.width<550)  //For handset
                  return 144;
                else			//For 1024x600
                  return 180;
              }
            }

            function getItemHeight() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 40;
                else if(newtab.width<1000)  //For handset
                  return 20;
                else			//For 1024x600
                  return 30;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 40;
                else if(newtab.width<550)  //For handset
                  return 20;
                else			//For 1024x600
                  return 30;
              }
            }

            function getItemHMargin() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 12;
                else if(newtab.width<1000)  //For handset
                  return 8;
                else			//For 1024x600
                  return 10;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 12;
                else if(newtab.width<550)  //For handset
                  return 8;
                else			//For 1024x600
                  return 10;
              }
            }

	    states: [
		State {
		    name: "landscape"
		    when: scene.orientation == 1 || scene.orientation == 3
		    PropertyChanges {
			target:list
			width: cellWidth * 5   //5 coloums
			height: cellHeight * 2
		    }
		},

		State {
		    name: "portrait"
		    when: scene.orientation == 2 || scene.orientation == 0
		    PropertyChanges {
			target: list
			width: cellWidth * 3   //3 coloums
			height: cellHeight * 3
		    }
		}
	    ]
	    GridView {
		id: list
                anchors.left: parent.left
                anchors.leftMargin: sector.getGridLeftMargin()
                //anchors.horizontalCenter: parent.horizontalCenter
		cellWidth: itemWidth + itemHMargin
		cellHeight: itemHeight + itemVMargin
		width: cellWidth * 5   //5 coloums
		height: cellHeight * 2
		delegate: MiniViewDelegate { }
		snapMode: GridView.SnapToRow
		model: sector.gridModel
		interactive: true
	    }
	}
    }

    Component {
	id: listView
        Item {
	    property int viewHeight: list.height + list.anchors.topMargin
            property int itemWidth: getItemWidth();
            property int itemHMargin: getItemHMargin();

            function getItemWidth() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 225;
                else if(newtab.width<1000)  //For handset
                  return 144;
                else			//For 1024x600
                  return 180;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 225;
                else if(newtab.width<550)  //For handset
                  return 144;
                else			//For 1024x600
                  return 180;
              }
            }

            function getItemHMargin() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 12;
                else if(newtab.width<1000)  //For handset
                  return 8;
                else			//For 1024x600
                  return 10;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 12;
                else if(newtab.width<550)  //For handset
                  return 8;
                else			//For 1024x600
                  return 10;
              }
            }

            function getListViewTopMargin() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 25;
                else if(newtab.width<1000)  //For handset
                  return 16;
                else			//For 1024x600
                  return 20;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 25;
                else if(newtab.width<550)  //For handset
                  return 16;
                else			//For 1024x600
                  return 20;
              }
            }

            ListView {
              id: list
              anchors.topMargin: getListViewTopMargin()
              width: parent.width
              height: newtab.height - mostVisited.height - category.height - anchors.topMargin
              anchors.left: parent.left
              orientation: ListView.Vertical
              delegate: ListViewDelegate { }
              model: sector.gridModel
              interactive: count * ( getRowHeight() +1 ) > height ? true : false
              clip: true
            }
            function getRowHeight() {
              if(newtab.width > newtab.height) {
                if(newtab.width>1200)  //For 1280x800
                  return 44;
                else if(newtab.width<1000)  //For handset
                  return 28;
                else			//For 1024x600
                  return 35;
              }else{
                if(newtab.width>750)  //For 1280x800
                  return 44;
                else if(newtab.width<550)  //For handset
                  return 28;
                else			//For 1024x600
                  return 35;
              }
            }
        }
    }
}

