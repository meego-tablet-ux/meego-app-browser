import Qt 4.7

/// Delegate for drawing single item in the tab list
/// This is used in both portrait and landscape modes, since
/// the only difference is the grid geometry ( 2 or 1 columns )
Component {

    Item {
        anchors.bottomMargin: 5
        width: gridView.cellWidth
        height: gridView.cellHeight

        Image {
            id: divide0
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            //anchors.leftMargin: -10
            //anchors.rightMargin: -5
            fillMode: Image.Stretch
            height: 2
            source: "image://themedimage/widgets/common/menu/menu-item-separator"
        }

        BorderImage {
            id: tabContainer
            source:"image://themedimage/widgets/apps/browser/tabs-border-overlay"
            height: 124
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: divide0.bottom
            anchors.topMargin: 5
            //anchors.leftMargin: 2
            //anchors.rightMargin: 2
            verticalTileMode: BorderImage.Repeat
            horizontalTileMode: BorderImage.Repeat

            border.top: 8
            border.bottom: 8
            border.left: 8
            border.right: 8
            smooth: true

            property bool isCurrentTab: false
            property string thumbnailPath: selectThumbnail (pageType, index, thumbnail)
            function selectThumbnail (pageType, index, thumbnail) {
                thumbnails.width = tabContainer.width - tabContainer.border.left - tabContainer.border.right
                thumbnails.anchors.left = tabContainer.left
                thumbnails.anchors.top = tabContainer.top
                thumbnails.anchors.leftMargin = tabContainer.border.left
                thumbnails.anchors.topMargin = tabContainer.border.top
                if (pageType == "newtab") {
                    thumbnails.height = tabContainer.height - titleRect.height - tabContainer.border.top - tabContainer.border.bottom
                    newtabBg.visible = true
                    return "image://themedimage/widgets/apps/browser/web-favorite-medium"
                } else if (pageType == "bookmarks") {
                    thumbnails.height = tabContainer.height - titleRect.height - tabContainer.border.top - tabContainer.border.bottom
                    newtabBg.visible = true
                    return "image://themedimage/widgets/apps/browser/web-favorite-medium"
                } else if (pageType == "downloads") {
                    thumbnails.height = tabContainer.height - titleRect.height - tabContainer.border.top - tabContainer.border.bottom
                    newtabBg.visible = true
                    return "image://themedimage/widgets/apps/browser/web-favorite-medium"
                }
                thumbnails.height = tabContainer.height - tabContainer.border.top - tabContainer.border.bottom
                newtabBg.visible = false
                return "image://tabsidebar/thumbnail_" + index + "_" + thumbnail
            }

            Image {
                id:thumbnails
                height: parent.height - parent.border.top - parent.border.bottom
                width: parent.width - parent.border.left - parent.border.right
                z: newtabBg.z + 1
                fillMode: Image.PreserveAspectFit
                smooth: true
                source: parent.thumbnailPath
            }
            Image {
                id: newtabBg
                anchors.fill: parent
                anchors.leftMargin: tabContainer.border.left
                anchors.rightMargin: tabContainer.border.right
                anchors.topMargin: tabContainer.border.top
                anchors.bottomMargin: tabContainer.border.bottom
                source: "image://themedimage/widgets/apps/browser/new-tab-background"
                visible: false
            }

            // title of the tab
            Item {
                id: titleRect                
                height: 30
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: tabContainer.border.left
                anchors.rightMargin: tabContainer.border.right
                anchors.bottomMargin: tabContainer.border.bottom
                z: 1
                Image {
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
                    //anchors.left: tabtitle.right
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
                        source: "image://themedimage/widgets/apps/browser/stop-reload"
                        property bool pressed: false
                        states: [
                            State {
                                name: "pressed"
                                when: closeIcon.pressed
                                PropertyChanges {
                                    target: closeIcon
                                    source: "image://themedimage/widgets/apps/browser/stop-reload"
                                }
                            }
                        ]
                    } // close icon Image

                    SequentialAnimation {
                        id: tabSwitchAnim
                        PropertyAnimation {
                            target: innerContent
                            properties: "opacity"
                            from: 1; to: 0
                            duration: 150
                            easing.type: Easing.InOutQuad }
                        PauseAnimation { duration: 150 }
                        ScriptAction { script: { tabSideBarModel.go(index) } }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (index == gridView.currentIndex ) {
                                tabContainer.isCurrentTab = true;
                            }
                            tabChangeFromTabSideBar = true;
                            fadeOut()
                        }
                        onPressed: closeIcon.pressed = true
                        onReleased: closeIcon.pressed = false
                    }
                } // Close button Item
            } // Title

            MouseArea {
                anchors.fill: parent
                onClicked: SequentialAnimation {
                    PropertyAnimation {
                        target: innerContent
                        properties: "opacity"
                        from: 1; to: 0
                        duration: 150
                        easing.type: Easing.InOutQuad
                    }
                    PauseAnimation { duration: 150 }
                    ScriptAction { script: {
                        if (showqmlpanel && (pageType == "normal" || pageType == "newtab")) {
                            showbookmarkmanager = false;
                            showdownloadmanager = false;
                        }
                        tabChangeFromTabSideBar = true;
                        if(index != gridView.currentIndex)
                            tabSideBarModel.go(index);
                        else
                            tabSideBarModel.hideSideBar();
                    } }
                    PropertyAction { target: innerContent; property: "opacity"; value: 1 }
                }
            } // Tab selection MouseArea

            states: [
                State {
                    name: "highlight"
                    when: index == gridView.currentIndex
                    PropertyChanges {
                        target: tabContainer
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
        } // tabContainer

        // This function is used in place of GridView.onRemove
        // because GridView.onRemove is not called when removing blank tab
        // This must be called manually, but is usable for all cases (unlike onRemove)
        function fadeOut() {
            itemFadeOutAnim.running = true
        }

        SequentialAnimation {
            id: itemFadeOutAnim
            PropertyAnimation {
                target: tabContainer
                properties: "scale, opacity"; to: 0, 0
                duration: 300
                easing.type: Easing.OutQuad
            }
            // close tab here (and not in the close click) to allow this fadeout animation to complete
            ScriptAction { script: { tabSideBarModel.closeTab(index) } }
        } // itemFadeOutAnim

        GridView.onRemove: {
            if ( tabContainer.isCurrentTab ) {
                GridView.view.hideTabsLater()
            }
        }

    } // Item
} // Component
