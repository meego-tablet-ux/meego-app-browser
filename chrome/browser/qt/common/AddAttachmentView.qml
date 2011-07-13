import Qt 4.7
import MeeGo.Components 0.1
import Qt.labs.gestures 2.0

Item {
    id: container

    anchors.fill: parent

    GestureArea {
        anchors.fill: parent
        Tap {}
        TapAndHold {}
        Pan {}
        Pinch {}
        Swipe {}
    }

    function pickerRejected () {
        selectFileDialogObject.OnPickerCancelled()
    }

    function pickerSelected (uri) {
        selectFileDialogObject.OnPickerSelected(uri)
    }

    function pickerMultiSelected (uri) {
        selectFileDialogObject.OnPickerMultiSelected(uri)
    }

    function addPicker (pickerComponent) {
        var picker = pickerComponent.createObject (container);
        picker.multiSelection = selectFileDialogObject.IsMultiSelection();
        picker.show ();
        picker.rejected.connect (pickerRejected);
        picker.selected.connect (pickerSelected);
        picker.multiselected.connect (pickerMultiSelected);
    }

    ModalFogBrowser {}

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton|Qt.LeftButton
        onClicked: {
            mouse.accepted = true;
        }
        onPressed: {
            mouse.accepted = true;
        }
        onReleased: {
            mouse.accepted = true;
        }
    }

  
    BorderImage {
        id: fileTypeDialog
        source: "image://themedimage/images/notificationBox_bg"

        x: (container.width - width) / 2
        y: (container.height - height) / 2
        height: 80

        Item {
            Column {
                anchors.fill: parent
                Button {
                    text: qsTr ("Photos")
                    width: fileTypeDialog.width
                    height: 40

                    onClicked: {
                        addPicker (photoPicker);
                    }
                }

                Button {
                    text: qsTr ("Movies")
                    width: fileTypeDialog.width
                    height: 40

                    onClicked: {
                        addPicker (videoPicker);
                    }
                }

/*
                Button {
                    text: qsTr ("Musics")
                    width: fileTypeDialog.width
                    height: 40

                    onClicked: {
                        addPicker (musicPicker);
                    }
                }
*/
            }
        }
    }

    Component {
        id: documentPicker
        Rectangle {
            anchors.fill: parent
            color: "pink"
        }
    }

    Component {
        id: musicPicker
        MusicPicker {
            anchors.fill: parent

            showPlaylists: false
            showAlbums: false

            signal selected (string uri)
            onSongSelected: {
                selected (song);
            }
        }
    }

    Component {
        id: photoPicker
        PhotoPicker {
            anchors.fill: parent

            signal selected (string uri)
            signal multiselected (string uris)
            onPhotoSelected: {
                selected (uri);
            }
            onMultiplePhotosSelected: {
                multiselected (uris)           
            }
        }
    }

    Component {
        id: videoPicker
        VideoPicker {
            anchors.fill: parent

            signal selected (string uri)
            signal multiselected (string uris)
            onVideoSelected: {
                selected (uri);
            }
            onMultipleVideosSelected: {
                multiselected (uris)
            }

        }
    }
}
