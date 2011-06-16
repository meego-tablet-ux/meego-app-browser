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
  // Represent the tab list view, show thumbnails of each tab
  // Call c++ methods to open/close pages
  id: container
  property variant model
  property int maxHeight: parent.maxHeight

  property int innertabHeight: 40
  property int titleHeight: 50

  property int tabTitleHeight: 30
  property int countSize: 40
  property int thumbnailMargin: 5
  property int counterSpacing: 5

  property int tabHeight: 134

  width: parent.width
  height: tabSideBarListView.count * tabHeight > maxHeight? maxHeight:tabSideBarListView.count*tabHeight

  // default margin is 4
  property int commonMargin : 4

  clip: true

  Component {
    id: tabDelegate
    Column{
      id: tabColumn
      spacing: 10
      width: parent.width
      height: 134 + divide0.height
      anchors.left: parent.left
      anchors.right: parent.right
      Image{
        id: divide0
        height: 2
        width: parent.width
        fillMode: Image.Stretch
        source: "image://themedimage/widgets/common/menu/menu-item-separator"
      }
      Rectangle{
        id: tabContainer
        height: 114
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.bottomMargin: 10
        property bool isCurrentTab: false
        property string thumbnailPath: selectThumbnail (pageType, index, thumbnail)
        function selectThumbnail (pageType, index, thumbnail) {
          if (pageType == "newtab") {
            thumbnails.height = tabContainer.height - titleRect.height
            newtabBg.visible = true
            return "image://themedimage/widgets/apps/browser/web-favorite-medium"
          } else if (pageType == "bookmarks") {
            thumbnails.height = tabContainer.height - titleRect.height
            newtabBg.visible = true
            return "image://themedimage/widgets/apps/browser/web-favorite-medium"
          } else if (pageType == "downloads") {
            thumbnails.height = tabContainer.height - titleRect.height
            newtabBg.visible = true
            return "image://themedimage/widgets/apps/browser/web-favorite-medium"
          }
          thumbnails.height = tabContainer.height
          newtabBg.visible = false
          return "image://tabsidebar/thumbnail_" + index + "_" + thumbnail
        }
        Image {
          id:thumbnails
          height: parent.height
          width: parent.width
          z: newtabBg.z + 1
          fillMode: Image.PreserveAspectFit
          smooth: true
          source: parent.thumbnailPath
        }
        Image {
          id: tabPageBg
          anchors.fill: parent
          source: "image://themedimage/widgets/apps/browser/tabs-border-overlay"
        }
        Image {
          id: newtabBg
          anchors.fill: parent
          source: "image://themedimage/widgets/apps/browser/new-tab-background"
          visible: false
        }
        // title of the tab
        Rectangle{
          id: titleRect
          width: parent.width
          height: 30
          anchors.bottom: parent.bottom
          z: 1
          Image{
            id: textBg
            source: "image://themedimage/widgets/apps/browser/tabs-background"
            anchors.fill: parent
          }
          Text {
            id: tabtitle
            height: parent.height
            width: parent.width - 50
            anchors.left: parent.left   
            anchors.leftMargin: 10

            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignLeft

            font.pixelSize: 15
            font.family: "Droid Sans"
            elide: Text.ElideRight
            text: title
            color: "#ffffff"
          }
          
          Image {
            id: sepImage
            source: "image://themedimage/widgets/apps/browser/tabs-line-spacer"
            height: parent.height
            width: 2
            anchors.left: tabtitle.right
            anchors.right: close.left
          }

          Item {
            id: close
            z: 1
            height: parent.height
            width: height
            anchors.right: parent.right
            Image {
              id: closeIcon
              anchors.centerIn: parent
              source: "image://themedimage/images/icn_close_up"
              property bool pressed: false
              states: [
                State {
                  name: "pressed"
                  when: closeIcon.pressed
                  PropertyChanges {
                    target: closeIcon
                    source: "image://themedimage/images/icn_close_dn"
                  }
                }
              ]
            }
            MouseArea {
              anchors.fill: parent
              onClicked: {
                if (index == tabSideBarListView.currentIndex ) {
                  tabContainer.isCurrentTab = true;
                }
                tabChangeFromTabSideBar = true;
                tabSideBarModel.closeTab(index);
              }
              onPressed: closeIcon.pressed = true
              onReleased: closeIcon.pressed = false
            }
          } // Item of close
 
        }

        SequentialAnimation {
          id: tabSwitchAnim
          PropertyAnimation { target: innerContent; properties: "opacity"; from: 1; to: 0; duration: 150; easing.type: Easing.InOutQuad }
          PauseAnimation { duration: 150}
          ScriptAction { script: tabSideBarModel.go(index) }
        }
        MouseArea {
          anchors.fill: parent
          onClicked: {
            if (showqmlpanel && (pageType == "normal" || pageType == "newtab")) {
              showbookmarkmanager = false;
              showdownloadmanager = false;
            }
            tabChangeFromTabSideBar = true;
            if(index != tabSideBarListView.currentIndex)
              tabSwitchAnim.running = true;
            else
              tabSideBarModel.hideSideBar();
          }
        }

        states: [
          State {
            name: "highlight"
            when: index == tabSideBarListView.currentIndex
            PropertyChanges {
              target: tabPageBg
              source: "image://themedimage/widgets/apps/browser/tabs-border-overlay-active"
            }
            PropertyChanges {
              target: textBg
              source: "image://themedimage/widgets/apps/browser/tabs-background-active"
            }
            PropertyChanges {
              target: sepImage
              source: "image://themedimage/widgets/apps/browser/tabs-line-spacer-active"
            }
            PropertyChanges {
              target: tabtitle
              color: "#383838"
            }
          } 
        ]
      } // BorderImage of tabContainer
      ListView.onRemove: SequentialAnimation {
          PropertyAction { target: tabColumn; property: "ListView.delayRemove"; value: true }
          NumberAnimation { target: tabColumn; property: "height"; to: 0; duration: 500; easing.type: Easing.InOutQuad }
          PauseAnimation { duration: 500 }
          ScriptAction { script: if(tabContainer.isCurrentTab) { tabSideBarModel.hideSideBar(); tabContainer.isCurrentTab = false;} }

          // Make sure delayRemove is set back to false so that the item can be destroyed
          PropertyAction { target: tabColumn; property: "ListView.delayRemove"; value: false }
        }
    }//column
  } // Component


  ListView {
    id: tabSideBarListView
    
    anchors.fill: parent

    interactive: container.height < container.maxHeight ? false:true

    contentY: tabHeight*currentIndex

    model: tabSideBarModel
    delegate: tabDelegate

  }
  Connections {
    target: tabSideBarModel
    onSelectTab: {
        tabSideBarListView.currentIndex = index
    }
  }
}
