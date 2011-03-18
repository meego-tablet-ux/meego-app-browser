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
import MeeGo.Labs.Components 0.1

BrowserWindowHandset {
  id: scene
  width: screenWidth
  height: screenHeight
  //public  
  property alias lastMousePos: lastMousePos

  onStatusBarTriggered: {
        orientation = (orientation +1) % 4;
    }

  Loader {
    id: newTabLoader
  }

  Connections {
    target: browserNewTabObject
    onHideNewTab: { 
      newTabLoader.sourceComponent = undefined;
    }
    onShowNewTab: {
      scene.showNewTab(newTabLoader, browserMostVisitModel, browserRecentlyClosedModel);
    }
  }

  function openWebPage(index, model) {
      model.openWebPage(index);
  }

  function showNewTab(loader, mostVisitModel, recentlyClosedModel) {
      loader.source="NewTabUi.qml"
      loader.item.parent = content
      loader.item.z = content.z + 1
      loader.item.updateModel(mostVisitModel, recentlyClosedModel);
  }

  Loader {
    id: dialogLoader
    z: 100
  }
  
  Connections {
    target: browserDialogObject
    onDismiss: dialogLoader.sourceComponent = undefined
    onPopup: {
      scene.showDialog(dialogLoader, browserDialogModel);
      //scene.showDialog(dialogLoader);//, browserDialogModel);
      console.log("Dialog: onPopup signal received.");
    }
  }

  function showDialog(loader, model) {
      var type = model.getDialogType();
      console.log("type_ is: " + type);
      loader.source="JsModalDialog.qml"
      loader.item.parent = container
      loader.item.z = 100
      if(type == 0) {
    	  loader.item.state = "alert"
      }else if(type == 1) {
          loader.item.state = "confirm"
      }else if(type == 2) {
    	  loader.item.state = "prompt"
      }else{
    	  loader.item.state = "confirm"
      }

  }

  Loader {
    id: selectFileDialogLoader
    z: 100
  }

  Connections {
    target: selectFileDialogObject
    onDismiss: {
      selectFileDialogLoader.sourceComponent = undefined
    }
    onPopup: {
      selectFileDialogLoader.source = "AddAttachmentView.qml"
      selectFileDialogLoader.item.parent = container
      selectFileDialogLoader.item.z = 100
    }
  }

  Loader {
    id: fullscreenBubbleLoader
    z: 100
  }

  property int findbarOldY
  property int infobarOldY
  Connections {
    target: fullscreenBubbleObject
    onExitFullscreen: {
      fullscreenBubbleLoader.sourceComponent = undefined
      if (scene.hasfindbar) {
        scene.findbar.y = findbarOldY
        if (infobarOldY != 0 && scene.infobar.height == 0)
          scene.findbar.y -= infobarOldY
      }
    }
    onEnterFullscreen: {
      fullscreenBubbleLoader.source="FullscreenBubble.qml"
      fullscreenBubbleLoader.item.parent = container
      fullscreenBubbleLoader.item.z = 100
      if (scene.hasfindbar) {
        findbarOldY = scene.findbar.y
        infobarOldY = scene.infobar.height
        scene.findbar.y = scene.infobar.height
      }
    }
  }

  Loader {
    id: contextLoader
    objectName: "contextLoader"
    property int menuWidth : 400
    function close() {
      contextLoader.sourceComponent = undefined;
      wrenchmenushown = false;
    }
    function triggered(index) {
      browserMenuObject.activateAt(index);
      contextLoader.sourceComponent = undefined;
      wrenchmenushown = false;
    }
  }

  Item {
    id: lastMousePos
    property int mouseX: 0
    property int mouseY: 0
  }

  Connections {
    target: browserMenuObject
    onPopupAt: {
      if (x != 0 || y != 0)
      {			
        var map = content.mapToItem(scene, x, y);
        contextLoader.source = "BrowserContextMenu.qml"
        scene.openContextMenu(contextLoader,
                              map.x, map.y, undefined, browserMenuModel);

      }
      else
      {
        wrenchmenushown = true;
        contextLoader.source = "BrowserContextMenu.qml"
        scene.openContextMenu(contextLoader,
                              lastMousePos.mouseX, lastMousePos.mouseY, undefined, browserMenuModel);
      }
    }
  }   

  function clamp (val, min, max) {
      return Math.max (min, Math.min (max, val));
  }

  function openContextMenu(loader, mouseX, mouseY, payload,model)
  {
      var parentContainer = screenlayer;

      loader.item.parent = parentContainer;
      loader.item.targetParent = parentContainer;
      loader.item.model = model;

      var menuContainer = loader.item;
      menuContainer.width = parentContainer.width;
      menuContainer.height = parentContainer.height;
      menuContainer.z = 100;
      menuContainer.payload = payload;

      var map = mapToItem (parentContainer, mouseX, mouseY);
      menuContainer.displayContextMenu (map.x, map.y);
  }
  
  Loader {
    id: bubbleLoader
    objectName: "bubbleLoader"
    property int bubbleWidth : 400
    function close() {
      bubbleLoader.sourceComponent = undefined
    }
  }

  Connections {
    target: bookmarkBubbleObject
    onPopupAt: {
      if (x == -1 && y == -1) {
        bubbleLoader.source = "MyBubble.qml"
        scene.openBubble(bubbleLoader,
                         lastMousePos.mouseX, lastMousePos.mouseY, undefined, bookmarkBubbleObject);
      }
    }
  }

  function openBubble(loader, mouseX, mouseY, payload, model) {
    var parentContainer = screenlayer;
    
    loader.item.parent = parentContainer;
    loader.item.model = model;

    var bubbleContainer = loader.item;
    bubbleContainer.width = parentContainer.width;
    bubbleContainer.height = parentContainer.height;
    bubbleContainer.z = content.z + 1;
    bubbleContainer.payload = payload;
    
    // Ensure that the bubble will completely fit on the screen, and that the bubble finger is attached
    // to the correct corner so its obvious what element the bubble is associated with.
    //bubbleContainer.fingerMode:
    //0 - left
    //1 - right
    //2 - top
    //3 - bottom
    
    // These offsets are the distance from the point of the finger
    // (at the mouse click) to the appropriate edge of the bubble
    // images. They are different due to the amounts of dropshadow
    // and translucency around the different edges.
    var leftOffset = 1;
    var rightOffset = 1;
    var topOffset = 14;
    var bottomOffset = 15;
    var fmode = 0;
  
    bubbleContainer.visible = true;
//    mouseY -= scene.statusBar.height;

    // Step one, find the appropriate direction for the finger to point
    // Horizontal placement takes precedence over vertical.
    if (mouseX + leftOffset + bubbleContainer.bubbleWidth < parentContainer.width) {
        fmode = 0;

        // Check vertically
        if (mouseY + (bubbleContainer.bubbleHeight / 2) > parentContainer.height) {
            // Switch to bottom
            fmode = 3;
        } else if (mouseY - (bubbleContainer.bubbleHeight / 2) < 0) {
            // Switch to top
            fmode = 2;
        }
    } else if (mouseX - bubbleContainer.bubbleWidth - rightOffset >= 0) {
        fmode = 1;

        if (mouseY + (bubbleContainer.bubbleHeight / 2) > parentContainer.height) {
            fmode = 3;
        } else if (mouseY - (bubbleContainer.bubbleHeight / 2) < 0) {
            fmode = 2;
        }
    }
    
    bubbleContainer.fingerMode = fmode;
    // Step two, given the finger direction, find the correct location
    // for the bubble, keeping it onscreen.
    switch (bubbleContainer.fingerMode) {
      case 0:
      case 1:
          bubbleContainer.bubbleY = clamp (mouseY - bubbleContainer.bubbleHeight / 2, 0, bubbleContainer.height);
          if (bubbleContainer.fingerMode == 0) {
              bubbleContainer.bubbleX = mouseX + leftOffset;
          } else {
              bubbleContainer.bubbleX = mouseX - rightOffset - bubbleContainer.bubbleWidth;
          }
          break;
      case 2:
      case 3:
          // Clamp mouseX so that at the edges of the screen we don't
          // try putting the finger at a location where the bubble can't be
          // placed
          // We don't need to do something similar to mouseY because when
          // it would happen to mouseY, we have switched to top or bottom
          // fingermode, and so it because a mouseX issue
          mouseX = clamp (mouseX, 35, parentContainer.width - 35);

          bubbleContainer.bubbleX = clamp (mouseX - bubbleContainer.bubbleWidth / 2, 0, (bubbleContainer.width - bubbleContainer.bubbleWidth));
          if (bubbleContainer.fingerMode == 2) {
              bubbleContainer.bubbleY = mouseY + topOffset;
          } else {
              bubbleContainer.bubbleY = mouseY - bubbleContainer.bubbleHeight + bottomOffset;
          }
          break;
      }

      bubbleContainer.fingerX = mouseX - bubbleContainer.bubbleX;
      bubbleContainer.fingerY = mouseY - bubbleContainer.bubbleY;
  }

 //     source: {if (dpiX < 96) "BrowserWindowHandset.qml"; else "BrowserWindowTablet.qml";}

}  

