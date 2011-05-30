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
  // Represent a history stack view, showing thumbnails of visual history
  // Call c++ methods to open the clicked page and decide whether to show
  // historybar and listen to signals from c++ to update its showness
  id: container
  // default height is 135
  property int rectHeight: 135
  height: rectHeight
  anchors.left: parent.left
  anchors.right: parent.right

  // default margin is 5
  property int commonMargin: 5
  property int innerCommonMargin: 1

  // delegate to show thumbnail of each web page in the history list
  Component {
    id: historyItemDelegate
    Item {
      id: innerItem

      height: 114 
      width: 180

      // flag to indicate whether the current item is the highlighted one in the list
      property bool isCurrent: false
      Rectangle {
        id: historyItem
        height: parent.height
        border.width: innerCommonMargin
        border.color: "grey"

        width: parent.width
        anchors.fill: parent
        anchors.margins: innerCommonMargin

        Rectangle {
            id: image 
            height: parent.height - historyTitle.height - innerCommonMargin
            width:  parent.width - innerCommonMargin 

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.leftMargin: innerCommonMargin
            anchors.topMargin: innerCommonMargin

            Image {
              height: parent.height
              width: parent.width                      
              id: thumbImage               
              source: thumbSrc             
              smooth: true
            }
        }

        Rectangle {
            id: historyTitle
            height: 30
            width: parent.width - innerCommonMargin
            anchors.bottom: parent.bottom
            anchors.left: parent.left	
            anchors.leftMargin: innerCommonMargin
            Image {
              id: textBg
              source: "image://themedimage/widgets/apps/browser/tabs-background"
              anchors.fill: parent
            }
            Text {
              id: titleText
              width: parent.width - 20
              x: 10
              y: 7
              font.pointSize: 10
              text: title
              clip: true
              color: "#ffffff"
              elide: Text.ElideRight
              wrapMode: Text.NoWrap
            }     
        }

          MouseArea {
            anchors.fill: parent
            onClicked: {
              console.log("click page " + index)
              // set current item is the highlighted one, and after some time,
              // open the page
              historyView.currentIndex = index
              timer.running = true
            }
          }
          // timer to delay some time to show the new clicked thumbnail
          Timer {
            id: timer
            interval: 100
            running: false
            onTriggered: {
              console.log("timer is ended")
              historyView.model.openPage(index)
            }
          }
      }
      // highlighted the selected current thumbnail with a border
      states: State {
        name: "highlight"
        when: innerItem.ListView.isCurrentItem
        PropertyChanges {
          target: historyItem
          radius: 2
          border.width: 1
          border.color: "#2CACE3"
        }
	PropertyChanges {
	  target: titleText
	  color: "#383838"
	}
	PropertyChanges {
	  target: historyTitle
	  color: "white"
	}
	PropertyChanges {
	  target: textBg
	  source: "image://themedimage/widgets/apps/browser/tabs-background-active"
	}
      }
    }
  }

  ListView {
    id: historyView
    anchors.fill: parent
    spacing: 5
    anchors.topMargin: commonMargin*2
    anchors.bottomMargin: commonMargin*2
    delegate: historyItemDelegate
    model: historyStackModel
    focus: true
    clip: true
    currentIndex: 0
    property real itemHeight: height
    property real itemWidth: innerItem.width
    orientation: ListView.Horizontal
    // only interactive when items are full of container
    interactive: itemWidth * count < container.width ? false : true
    // control the scrollbar
    states: State {
       name: "ShowBars"
       when: historyView.movingHorizontally
       PropertyChanges { target: horizontalScrollBar; opacity: 1; height: 5}
    }
  }
  ScrollBar {
    id: horizontalScrollBar
    width: historyView.width; height: 0
    anchors.bottom: parent.bottom
    opacity: 0
    orientation: Qt.Horizontal
    position: historyView.visibleArea.xPosition
    pageSize: historyView.visibleArea.widthRatio
  }
  Connections {
    target: historyStackModel
    onCurrent: {
        historyView.currentIndex = index
    }
  }
  signal hideOverlay
  signal showOverlay
  Connections {
    target: historyStackModel
    onShowHistory: {
        showOverlay()
    }
    onHideHistory: {
        hideOverlay()
    }
  }
}
