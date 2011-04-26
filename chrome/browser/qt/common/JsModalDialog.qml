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

    // create ModalDialog:
    ModalDialog {
        id: jsDialog
	property alias contentLoader: contentLoader

        title : browserDialogModel.getDialogTitle()
        buttonWidth: 200
        buttonHeight: 35
        showCancelButton: true
        showAcceptButton: true
        cancelButtonText: browserDialogModel.getRightButtonText()
        acceptButtonText: browserDialogModel.getLeftButtonText()
        content: Loader { id: contentLoader
    			  anchors.fill: parent }

        // handle signals:
        onAccepted: {
            // do something
	    console.log("Accept Button clicked: ");
	    console.log("is Suppress: " + contentLoader.item.isSuppress);
	    if(browserDialogModel.getDialogType() == 4)
		browserDialogObject.OnButtonClicked(1, contentLoader.item.input1.toString(), contentLoader.item.input2.toString(), contentLoader.item.isSuppress);
	    else if(browserDialogModel.getDialogType() == 2)
		browserDialogObject.OnButtonClicked(1, contentLoader.item.input1.toString(), null, contentLoader.item.isSuppress);
	    else
		browserDialogObject.OnButtonClicked(1, null, null, contentLoader.item.isSuppress);
	
        }
        onRejected: {
            // do something
	    console.log("Cancel Button clicked: ");
	    console.log("is Suppress: " + contentLoader.item.isSuppress);
	    if(browserDialogModel.getDialogType() == 4)
		browserDialogObject.OnButtonClicked(2, contentLoader.item.input1.toString(), contentLoader.item.input2.toString(), contentLoader.item.isSuppress);
	    else if(browserDialogModel.getDialogType() == 2)
		browserDialogObject.OnButtonClicked(2, contentLoader.item.input1.toString(), null, contentLoader.item.isSuppress);
	    else
		browserDialogObject.OnButtonClicked(2, null, null, contentLoader.item.isSuppress);
	
         }
     }

     // show on signal:
     Component.onCompleted: {
         jsDialog.show()
     }

	states: [
		State {
		    name: "alert"
		    PropertyChanges {
			target: jsDialog.contentLoader
			sourceComponent: alertContent
		    }
		    PropertyChanges {
			target: jsDialog
			showCancelButton: false
		    }
		},

		State {
		    name: "confirm"
		    PropertyChanges {
			target: jsDialog.contentLoader
			sourceComponent: alertContent
		    }
		},

		State {
		    name: "prompt"
		    PropertyChanges {
			target: jsDialog.contentLoader;
			sourceComponent: promptContent
		    }
		},
		State {
		    name: "auth"
		    PropertyChanges {
			target: jsDialog.contentLoader;
			sourceComponent: authContent
		    }
		}
	]

	 Component {
	    id: alertContent
	    Item {
		property alias isSuppress: checkBox.isChecked

		Text {
		  id: content
		  anchors.left: parent.left
		  anchors.leftMargin: 10
		  anchors.topMargin: -10 
		  anchors.verticalCenter: parent.verticalCenter
		  width: parent.width-20
		  verticalAlignment: Text.AlignVCenter
		  text: browserDialogModel.getDialogContent() 
		  color: theme_dialogTitleFontColor
		  wrapMode: Text.Wrap
		}

		Row {
		  id: suppressBox
		  anchors.left: parent.left
		  anchors.bottom: parent.bottom
		  anchors.leftMargin: 10
		  anchors.topMargin: 10
		  anchors.bottomMargin: 1
		  width: parent.width
		  visible: browserDialogModel.isSuppress()
		  CheckBox {
		    id: checkBox
		  }
		  Text {
		    id: checkText
		    anchors.leftMargin: 4
		    width: parent.width - 20
		    color: theme_dialogTitleFontColor
    		    font.pixelSize: 15
		    verticalAlignment: Text.AlignVCenter
		    text: browserDialogModel.getSuppressText()
		  }
		}
	    }
	}

        Component {
	    id: promptContent
	    Item {
	        property alias isSuppress: checkBox.isChecked
	        property alias input1: textInput.text
		Text {
		  id: content
		  anchors.left: parent.left
		  anchors.top: parent.top
		  anchors.leftMargin: 10
		  anchors.topMargin: 10 
		  width: parent.width - 20
                  height: 25
		  //anchors.verticalCenter: parent.verticalCenter
		  text: browserDialogModel.getDialogContent()
		  color: theme_dialogTitleFontColor
		  wrapMode: Text.Wrap
		}

		InputBox {
		  id: textInput
		  anchors.left: parent.left
		  anchors.leftMargin: 10
		  anchors.topMargin: 10
		  anchors.top: content.bottom
		  width: parent.width - 20
    		  //font.pixelSize: 17
		  text: browserDialogModel.getDefaultPrompt()
		  color: theme_dialogTitleFontColor
		}

		Row {
		    id: suppressBox
		    anchors.left: parent.left
		    anchors.bottom: parent.bottom
		    anchors.leftMargin: 10
		    anchors.topMargin: 10
		    anchors.bottomMargin: 1
		    width: parent.width
		    visible: browserDialogModel.isSuppress()
		    CheckBox {
			id: checkBox
		    }
		    Text {
			id: checkText
		        anchors.leftMargin: 4
		        width: parent.width - 20
		  	color: theme_dialogTitleFontColor
    		        font.pixelSize: 15
		        verticalAlignment: Text.AlignVCenter
			text: browserDialogModel.getSuppressText()
		    }
		}
	    }
        }

        Component {
	    id: authContent
	    Item {
	        property alias isSuppress: checkBox.isChecked
	        property alias input1: textInput1.text
	        property alias input2: textInput2.text
		anchors.fill:parent
		Row {
		  id: sector1
		  anchors.left: parent.left
	          anchors.top: parent.top
	          anchors.topMargin: 10 
		  width: parent.width
		  height: 40
			Text {
			  id: username
			  anchors.leftMargin: 10
			  anchors.topMargin: 10 
			  anchors.top: parent.top
			  text: browserDialogModel.getUsernameText()
			  color: theme_dialogTitleFontColor
			  font.pixelSize: 17
			  width: 140
			}

			InputBox {
			  id: textInput1
			  anchors.top: parent.top
			  anchors.topMargin: 10
			  width: parent.width - 180
		  	  height: 30
			  color: theme_dialogTitleFontColor
			  font.pixelSize: 17
			}
		}

		Row {
		  id:sector2
		  anchors.left: parent.left
		  anchors.top: sector1.bottom 
	          anchors.topMargin: 10 
		  width: parent.width
		  height: 40
			Text {
			  id: password
			  anchors.leftMargin: 10
			  anchors.topMargin: 5 
			  anchors.top: parent.top
			  text: browserDialogModel.getPasswordText()
			  color: theme_dialogTitleFontColor
			  font.pixelSize: 17
			  width: 140
			}

			InputBox {
			  id: textInput2
			  anchors.top: parent.top
			  anchors.topMargin: 5
			  width: parent.width - 180
		  	  height: 30
			  color: theme_dialogTitleFontColor
			  font.pixelSize: 17
			  echoMode: TextInput.Password
			}
		}

		Row {
		  id: suppressBox
		  anchors.left: parent.left
		  anchors.bottom: parent.bottom
		  anchors.leftMargin: 10
		  anchors.topMargin: 10
		  anchors.bottomMargin: 1
		  width: parent.width
		  visible: false
		  CheckBox {
		    id: checkBox
		  }
		  Text {
		    id: checkText
		    anchors.leftMargin: 4
		    width: parent.width - 20
		    color: theme_dialogTitleFontColor
    		    font.pixelSize: 15
		    verticalAlignment: Text.AlignVCenter
		    text: browserDialogModel.getSuppressText()
		  }
		}

	    }
        }
}


