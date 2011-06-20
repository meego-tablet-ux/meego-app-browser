import Qt 4.7

/// Delegate for drawing single item in the tab list
/// This is used in both portrait and landscape modes, since
/// the only difference is the grid geometry ( 2 or 1 columns )
Component {

    Item {
        anchors.bottomMargin: 10
        width: GridView.view.cellWidth
        height: GridView.view.cellHeight

        Image {
            id: divide0
            height: 2
            anchors.top: parent.top
            width: parent.width
            fillMode: Image.Stretch
            source: "image://themedimage/widgets/common/menu/menu-item-separator"
        }

        Rectangle {
            id: tabContainer
            height: 114
            width: parent.width
            anchors.margins: 10
            anchors.centerIn: parent
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
            Rectangle {
                id: titleRect
                width: parent.width
                height: 30
                anchors.bottom: parent.bottom
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
            } // Title Rectangle

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
        } // tabContainer Rectangle

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
