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
import Qt.labs.gestures 2.0

Item {
  id: newtab
  anchors.fill: parent
  property alias mostVisitedModel: mostVisited.gridModel;
  property alias recentlyClosedModel: recentlyClosed.gridModel;

  Image {
    id: background
    anchors.fill: parent
    fillMode: Image.Stretch
    source: "image://theme/bg_application_p"
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
    visibleRow: 2
    enableDrag: true
    z:recentlyClosed.z + 1
    anchors.horizontalCenter: parent.horizontalCenter
    itemWidth:(newtab.height < 650)? 210 : 231
    itemHeight:(newtab.height < 650)? 130 : 143
    gridScrollable: false;
  }

  MaxViewFrame {
    id: recentlyClosed
    visibleRow: (newtab.height < 650)? 1 : 2
    enableDrag: false
    anchors.top: mostVisited.bottom 
    anchors.horizontalCenter: parent.horizontalCenter
    itemWidth:(newtab.height < 650)? 210 : 231
    itemHeight:(newtab.height < 650)? 130 : 143
    gridScrollable:newtab.height < 650
  }

  function updateModel(model, model2) {
    mostVisitedModel = model;
    recentlyClosedModel = model2;
    mostVisited.categoryTitle = model.GetCategoryName();
    mostVisited.collapseState = model.getCollapsedState();
    recentlyClosed.categoryTitle = model2.GetCategoryName();
    recentlyClosed.collapseState = model2.getCollapsedState();
    console.log("NewTabPage: update model signal received.");
  }
}
