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
import Qt.labs.gestures 2.0

Item {
  id: newtab
  anchors.fill: parent
  property alias mostVisitedModel: mostVisited.gridModel;
  property alias recentlyClosedModel: recentlyClosed.gridModel;

  BorderImage {
    id: background
    anchors.fill: parent
    border.left:   8
    border.top:    8
    border.bottom: 20
    border.right:  8
    source: "image://themedimage/widgets/common/backgrounds/content-background-with-header-reverse"
  }

  GestureArea {
    anchors.fill: parent
    Tap {}
    TapAndHold {}
    Pan {}
    Pinch {}
    Swipe {}
  }

  MouseArea {
    id: mouseArea
    anchors.fill: parent
    acceptedButtons:Qt.RightButton | Qt.LeftButton
    onClicked: {
      mouse.accepted = true;
    }
    onPressed: { 
      mouse.accepted = true;
      newtab.focus = true;
    }
    onReleased: { 
      mouse.accepted = true;
    }
  }

  MaxViewFrame {
    id: mostVisited
    enableDrag: true
    width: parent.width
    z:recentlyClosed.z + 1
  }

  MaxViewFrame {
    id: recentlyClosed
    enableDrag: false
    width: parent.width
    anchors.top: mostVisited.bottom
  }
 
  function getLineTopMargin() {
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

  function updateModel(model, model2) {
    //console.log("NewTabPage: new tab width: " + newtab.width + " new tab height: " + newtab.height + "orientation: " + scene.orientation);
    mostVisitedModel = model;
    recentlyClosedModel = model2;
    mostVisited.categoryTitle = model.GetCategoryName();
    mostVisited.collapseState = model.getCollapsedState();
    recentlyClosed.categoryTitle = model2.GetCategoryName();
    recentlyClosed.collapseState = model2.getCollapsedState();
  }
}
