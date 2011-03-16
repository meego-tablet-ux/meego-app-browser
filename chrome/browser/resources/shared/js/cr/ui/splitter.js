// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This implements a splitter element which can be used to resize
 * elements in split panes.
 *
 * The parent of the splitter should be an hbox (display: -webkit-box) with at
 * least one previous element sibling. The splitter controls the width of the
 * element before it.
 *
 * <div class=split-pane>
 *   <div class=left>...</div>
 *   <div class=splitter></div>
 *   ...
 * </div>
 *
 */

cr.define('cr.ui', function() {
  // TODO(arv): Currently this only supports horizontal layout.
  // TODO(arv): This ignores min-width and max-width of the elements to the
  // right of the splitter.

  /**
   * Returns the computed style width of an element.
   * @param {!Element} el The element to get the width of.
   * @return {number} The width in pixels.
   */
  function getComputedWidth(el) {
    return parseFloat(el.ownerDocument.defaultView.getComputedStyle(el).width) /
        getZoomFactor(el.ownerDocument);
  }

  /**
   * This uses a WebKit bug to work around the same bug. getComputedStyle does
   * not take the page zoom into account so it returns the physical pixels
   * instead of the logical pixel size.
   * @param {!Document} doc The document to get the page zoom factor for.
   * @param {number} The zoom factor of the document.
   */
  function getZoomFactor(doc) {
    var dummyElement = doc.createElement('div');
    dummyElement.style.cssText =
    'position:absolute;width:100px;height:100px;top:-1000px;overflow:hidden';
    doc.body.appendChild(dummyElement);
    var cs = doc.defaultView.getComputedStyle(dummyElement);
    var rect = dummyElement.getBoundingClientRect();
    var zoomFactor = parseFloat(cs.width) / 100;
    doc.body.removeChild(dummyElement);
    return zoomFactor;
  }

  /**
   * Creates a new splitter element.
   * @param {Object=} opt_propertyBag Optional properties.
   * @constructor
   * @extends {HTMLDivElement}
   */
  var Splitter = cr.ui.define('div');

  Splitter.prototype = {
    __proto__: HTMLDivElement.prototype,

    /**
     * Initializes the element.
     */
    decorate: function() {
      this.addEventListener('mousedown', this.handleMouseDown_.bind(this),
                            true);
    },

    /**
     * Starts the dragging of the splitter. Adds listeners for mouse move and
     * mouse up events and calls splitter drag start handler.
     * @param {!Event} e The mouse event that started the drag.
     */
    startDrag: function(e) {
      if (!this.boundHandleMouseMove_) {
        this.boundHandleMouseMove_ = this.handleMouseMove_.bind(this);
        this.boundHandleMouseUp_ = this.handleMouseUp_.bind(this);
      }

      var doc = this.ownerDocument;

      // Use capturing events on the document to get events when the mouse
      // leaves the document.
      doc.addEventListener('mousemove',this.boundHandleMouseMove_, true);
      doc.addEventListener('mouseup', this.boundHandleMouseUp_, true);

      this.startX_ = e.clientX;
      this.handleSplitterDragStart();
    },

    /**
     * Ends the dragging of the splitter. Removes listeners set in startDrag
     * and calls splitter drag end handler.
     */
    endDrag: function() {
      var doc = this.ownerDocument;
      doc.removeEventListener('mousemove', this.boundHandleMouseMove_, true);
      doc.removeEventListener('mouseup', this.boundHandleMouseUp_, true);
      this.handleSplitterDragEnd();
    },

    /**
     * Handles the mousedown event which starts the dragging of the splitter.
     * @param {!Event} e The mouse event.
     * @private
     */
    handleMouseDown_: function(e) {
      this.startDrag(e);
      // Default action is to start selection and to move focus.
      e.preventDefault();
    },

    /**
     * Handles the mousemove event which moves the splitter as the user moves
     * the mouse. Calls splitter drag move handler.
     * @param {!Event} e The mouse event.
     * @private
     */
    handleMouseMove_: function(e) {
      var rtl = this.ownerDocument.defaultView.getComputedStyle(this).
          direction == 'rtl';
      var dirMultiplier = rtl ? -1 : 1;
      var deltaX = dirMultiplier * (e.clientX - this.startX_);
      this.handleSplitterDragMove(deltaX);
    },

    /**
     * Handles the mouse up event which ends the dragging of the splitter.
     * @param {!Event} e The mouse event.
     * @private
     */
    handleMouseUp_: function(e) {
      this.endDrag();
    },

    /**
     * Handles start of the splitter dragging. Saves current width of the
     * element being resized.
     * @protected
     */
    handleSplitterDragStart: function() {
      // Use the computed width style as the base so that we can ignore what
      // box sizing the element has.
      var leftComponent = this.previousElementSibling;
      var doc = leftComponent.ownerDocument;
      this.startWidth_ = parseFloat(
          doc.defaultView.getComputedStyle(leftComponent).width);
    },

    /**
     * Handles splitter moves. Updates width of the element being resized.
     * @param {number} changeX The change of splitter horizontal position.
     * @protected
     */
    handleSplitterDragMove: function(deltaX) {
      var leftComponent = this.previousElementSibling;
      leftComponent.style.width = this.startWidth_ + deltaX + 'px';
    },

    /**
     * Handles end of the splitter dragging. This fires a 'resize' event if the
     * size changed.
     * @protected
     */
    handleSplitterDragEnd: function() {
      // Check if the size changed.
      var leftComponent = this.previousElementSibling;
      var doc = leftComponent.ownerDocument;
      var computedWidth = parseFloat(
          doc.defaultView.getComputedStyle(leftComponent).width);
      if (this.startWidth_ != computedWidth)
        cr.dispatchSimpleEvent(this, 'resize');
    },
  };

  return {
    Splitter: Splitter
  }
});
