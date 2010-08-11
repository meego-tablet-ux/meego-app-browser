// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.define('options.contentSettings', function() {
  const List = cr.ui.List;
  const ListItem = cr.ui.ListItem;
  const ArrayDataModel = cr.ui.ArrayDataModel;

  /**
   * Creates a new exceptions list item.
   * @param {string} contentType The type of the list.
   * @param {Array} exception A pair of the form [filter, setting].
   * @constructor
   * @extends {cr.ui.ListItem}
   */
  function ExceptionsListItem(contentType, exception) {
    var el = cr.doc.createElement('li');
    el.contentType = contentType;
    el.dataItem = exception;
    el.__proto__ = ExceptionsListItem.prototype;
    el.decorate();

    return el;
  }

  ExceptionsListItem.prototype = {
    __proto__: ListItem.prototype,

    /**
     * Called when an element is decorated as a list item.
     */
    decorate: function() {
      ListItem.prototype.decorate.call(this);

      // Labels for display mode.
      var patternLabel = cr.doc.createElement('span');
      patternLabel.textContent = this.pattern;
      this.appendChild(patternLabel);

      var settingLabel = cr.doc.createElement('span');
      settingLabel.textContent = this.settingForDisplay();
      settingLabel.className = 'exceptionSetting';
      this.appendChild(settingLabel);

      // Elements for edit mode.
      var input = cr.doc.createElement('input');
      input.type = 'text';
      this.appendChild(input);
      input.className = 'exceptionInput hidden';

      var select = cr.doc.createElement('select');
      var optionAllow = cr.doc.createElement('option');
      optionAllow.textContent = templateData.allowException;
      select.appendChild(optionAllow);
      var optionBlock = cr.doc.createElement('option');
      optionBlock.textContent = templateData.blockException;
      select.appendChild(optionBlock);

      if (this.contentType == 'cookies') {
        var optionSession = cr.doc.createElement('option');
        optionSession.textContent = templateData.sessionException;
        select.appendChild(optionSession);
        this.optionSession = optionSession;
      }

      this.appendChild(select);
      select.className = 'exceptionSetting hidden';

      // Used to track whether the URL pattern in the input is valid.
      // This will be true if the browser process has informed us that the
      // current text in the input is valid. Changing the text resets this to
      // false, and getting a response from the browser sets it back to true.
      // It starts off as false for empty string (new exceptions) or true for
      // already-existing exceptions (which we assume are valid).
      this.inputValidityKnown = this.pattern;
      // This one tracks the actual validity of the pattern in the input. This
      // starts off as true so as not to annoy the user when he adds a new and
      // empty input.
      this.inputIsValid = true;

      this.patternLabel = patternLabel;
      this.settingLabel = settingLabel;
      this.input = input;
      this.select = select;
      this.optionAllow = optionAllow;
      this.optionBlock = optionBlock;

      this.updateEditables();

      var listItem = this;
      this.ondblclick = function(event) {
        listItem.editing = true;
      };

      // Handle events on the editable nodes.
      input.oninput = function(event) {
        listItem.inputValidityKnown = false;
        chrome.send('checkExceptionPatternValidity',
                    [listItem.contentType, input.value]);
      };

      // Handles enter and escape which trigger reset and commit respectively.
      function handleKeydown(e) {
        // Make sure that the tree does not handle the key.
        e.stopPropagation();

        // Calling list.focus blurs the input which will stop editing the list
        // item.
        switch (e.keyIdentifier) {
          case 'U+001B':  // Esc
            // Reset the inputs.
            listItem.updateEditables();
            if (listItem.pattern)
              listItem.maybeSetPatternValidity(listItem.pattern, true);
          case 'Enter':
            if (listItem.parentNode)
              listItem.parentNode.focus();
        }
      }

      function handleBlur(e) {
        // When the blur event happens we do not know who is getting focus so we
        // delay this a bit since we want to know if the other input got focus
        // before deciding if we should exit edit mode.
        var doc = e.target.ownerDocument;
        window.setTimeout(function() {
          var activeElement = doc.activeElement;
          if (!listItem.contains(activeElement)) {
            listItem.editing = false;
          }
        }, 50);
      }

      input.addEventListener('keydown', handleKeydown);
      input.addEventListener('blur', handleBlur);

      select.addEventListener('keydown', handleKeydown);
      select.addEventListener('blur', handleBlur);
    },

    /**
     * The pattern (e.g., a URL) for the exception.
     * @type {string}
     */
    get pattern() {
      return this.dataItem[0];
    },
    set pattern(pattern) {
      this.dataItem[0] = pattern;
    },

    /**
     * The setting (allow/block) for the exception.
     * @type {string}
     */
    get setting() {
      return this.dataItem[1];
    },
    set setting(setting) {
      this.dataItem[1] = setting;
    },

    /**
     * Gets a human-readable setting string.
     * @type {string}
     */
    settingForDisplay: function() {
      var setting = this.setting;
      if (setting == 'allow')
        return templateData.allowException;
      else if (setting == 'block')
        return templateData.blockException;
    },

    /**
     * Update this list item to reflect whether the input is a valid pattern
     * if |pattern| matches the text currently in the input.
     * @param {string} pattern The pattern.
     * @param {boolean} valid Whether said pattern is valid in the context of
     *     a content exception setting.
     */
    maybeSetPatternValid: function(pattern, valid) {
      // Don't do anything for messages where we are not the intended recipient,
      // or if the response is stale (i.e. the input value has changed since we
      // sent the request to analyze it).
      if (pattern != this.input.value)
        return;

      if (valid)
        this.input.setCustomValidity('');
      else
        this.input.setCustomValidity(' ');
      this.inputIsValid = valid;
      this.inputValidityKnown = true;
    },

    /**
     * Copy the data model values to the editable nodes.
     */
    updateEditables: function() {
      if (!(this.pattern && this.setting))
        return;

      this.input.value = this.pattern;

      if (this.setting == 'allow')
        this.optionAllow.selected = true;
      else if (this.setting == 'block')
        this.optionBlock.selected = true;
      else if (this.setting == 'session')
        this.optionSession.selected = true;
    },

    /**
     * Whether the user is currently able to edit the list item.
     * @type {boolean}
     */
    get editing() {
      return this.hasAttribute('editing');
    },
    set editing(editing) {
      var oldEditing = this.editing;
      if (oldEditing == editing)
        return;

      var listItem = this;
      var pattern = this.pattern;
      var setting = this.setting;
      var patternLabel = this.patternLabel;
      var settingLabel = this.settingLabel;
      var input = this.input;
      var select = this.select;
      var optionAllow = this.optionAllow;
      var optionBlock = this.optionBlock;
      var optionSession = this.optionSession;

      // Check that we have a valid pattern and if not we do not change the
      // editing mode.
      if (!editing && (!this.inputValidityKnown || !this.inputIsValid)) {
        if (this.pattern) {
          input.focus();
          input.select();
        } else {
          // Just delete this row if it was added via the Add button.
          var model = listItem.parentNode.dataModel;
          model.splice(model.indexOf(listItem.dataItem), 1);
        }

        return;
      }

      patternLabel.classList.toggle('hidden');
      settingLabel.classList.toggle('hidden');
      input.classList.toggle('hidden');
      select.classList.toggle('hidden');

      var doc = this.ownerDocument;
      if (editing) {
        this.setAttribute('editing', '');
        cr.ui.limitInputWidth(input, this, 20);
        input.focus();
        input.select();
      } else {
        this.removeAttribute('editing');

        var newPattern = input.value;

        var newSetting;
        if (optionAllow.selected)
          newSetting = 'allow';
        else if (optionBlock.selected)
          newSetting = 'block';
        else if (optionSession.selected)
          newSetting = 'session';

        // Empty edit - do nothing.
        if (pattern == newPattern && newSetting == this.setting)
          return;

        this.pattern = patternLabel.textContent = newPattern;
        this.setting = newSetting;
        settingLabel.textContent = this.settingForDisplay();

        var contentType = this.parentNode.contentType;

        if (pattern != this.pattern)
          chrome.send('removeExceptions', [contentType, pattern]);

        chrome.send('setException', [contentType, this.pattern, this.setting]);
      }
    }
  };

  /**
   * Creates a new exceptions list.
   * @constructor
   * @extends {cr.ui.List}
   */
  var ExceptionsList = cr.ui.define('list');

  ExceptionsList.prototype = {
    __proto__: List.prototype,

    /**
     * Called when an element is decorated as a list.
     */
    decorate: function() {
      List.prototype.decorate.call(this);

      this.dataModel = new ArrayDataModel([]);
    },

    /**
     * Creates an item to go in the list.
     * @param {Object} entry The element from the data model for this row.
     */
    createItem: function(entry) {
      return new ExceptionsListItem(this.contentType, entry);
    },

    /**
     * Adds an exception to the js model.
     * @param {Array} entry A pair of the form [filter, setting].
     */
    addException: function(entry) {
      this.dataModel.push(entry);

      // When an empty row is added, put it into editing mode.
      if (!entry[0] && !entry[1]) {
        var index = this.dataModel.length - 1;
        var sm = this.selectionModel;
        sm.anchorIndex = sm.leadIndex = sm.selectedIndex = index;
        this.scrollIndexIntoView(index);
        var li = this.getListItemByIndex(index);
        li.editing = true;
      }
    },

    /**
     * The browser has finished checking a pattern for validity. Update the
     * list item to reflect this.
     * @param {string} pattern The pattern.
     * @param {bool} valid Whether said pattern is valid in the context of
     *     a content exception setting.
     */
    patternValidityCheckComplete: function(pattern, valid) {
      for (var i = 0; i < this.dataModel.length; i++) {
        var listItem = this.getListItemByIndex(i);
        if (listItem)
          listItem.maybeSetPatternValid(pattern, valid);
      }
    },

    /**
     * Removes all exceptions from the js model.
     */
    clear: function() {
      this.dataModel = new ArrayDataModel([]);
    },

    /**
     * Removes all selected rows from browser's model.
     */
    removeSelectedRows: function() {
      // The first member is the content type; the rest of the values are
      // the patterns we are removing.
      var args = [this.contentType];
      var selectedItems = this.selectedItems;
      for (var i = 0; i < selectedItems.length; i++) {
        args.push(selectedItems[i][0]);
      }

      chrome.send('removeExceptions', args);
    },

    /**
     * Puts the selected row in editing mode.
     */
    editSelectedRow: function() {
      var li = this.getListItem(this.selectedItem);
      if (li)
        li.editing = true;
    }
  };

  var ExceptionsArea = cr.ui.define('div');

  ExceptionsArea.prototype = {
    __proto__: HTMLDivElement.prototype,

    decorate: function() {
      // TODO(estade): need some sort of visual indication when the list is
      // empty.
      this.exceptionsList = this.querySelector('list');
      this.exceptionsList.contentType = this.contentType;

      ExceptionsList.decorate(this.exceptionsList);
      this.exceptionsList.selectionModel.addEventListener(
          'change', cr.bind(this.handleOnSelectionChange_, this));

      var addRow = cr.doc.createElement('button');
      addRow.textContent = templateData.addExceptionRow;
      this.appendChild(addRow);

      var editRow = cr.doc.createElement('button');
      editRow.textContent = templateData.editExceptionRow;
      this.appendChild(editRow);
      this.editRow = editRow;

      var removeRow = cr.doc.createElement('button');
      removeRow.textContent = templateData.removeExceptionRow;
      this.appendChild(removeRow);
      this.removeRow = removeRow;

      var self = this;
      addRow.onclick = function(event) {
        self.exceptionsList.addException(['', '']);
      };

      editRow.onclick = function(event) {
        self.exceptionsList.editSelectedRow();
      };

      removeRow.onclick = function(event) {
        self.exceptionsList.removeSelectedRows();
      };

      this.updateButtonSensitivity();
    },

    /**
     * The content type for this exceptions area, such as 'images'.
     * @type {string}
     */
    get contentType() {
      return this.getAttribute('contentType');
    },
    set contentType(type) {
      return this.setAttribute('contentType', type);
    },

    /**
     * Update the enabled/disabled state of the editing buttons based on which
     * rows are selected.
     */
    updateButtonSensitivity: function() {
      var selectionSize = this.exceptionsList.selectedItems.length;
      this.editRow.disabled = selectionSize != 1;
      this.removeRow.disabled = selectionSize == 0;
    },

    /**
     * Callback from the selection model.
     * @param {!cr.Event} ce Event with change info.
     * @private
     */
    handleOnSelectionChange_: function(ce) {
      this.updateButtonSensitivity();
   },
  };

  return {
    ExceptionsListItem: ExceptionsListItem,
    ExceptionsList: ExceptionsList,
    ExceptionsArea: ExceptionsArea
  };
});
