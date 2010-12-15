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
   * @param {string} mode The browser mode, 'otr' or 'normal'.
   * @param {boolean} enableAskOption Whether to show an 'ask every time'
   *     option in the select.
   * @param {Object} exception A dictionary that contains the data of the
   *     exception.
   * @constructor
   * @extends {cr.ui.ListItem}
   */
  function ExceptionsListItem(contentType, mode, enableAskOption, exception) {
    var el = cr.doc.createElement('li');
    el.mode = mode;
    el.contentType = contentType;
    el.enableAskOption = enableAskOption;
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

      // Labels for display mode. |pattern| will be null for the 'add new
      // exception' row.
      if (this.pattern) {
        var patternLabel = cr.doc.createElement('span');
        patternLabel.textContent = this.pattern;
        patternLabel.className = 'exceptionPattern';
        this.appendChild(patternLabel);
        this.patternLabel = patternLabel;

        var settingLabel = cr.doc.createElement('span');
        settingLabel.textContent = this.settingForDisplay();
        settingLabel.className = 'exceptionSetting';
        this.appendChild(settingLabel);
        this.settingLabel = settingLabel;
      }

      // Elements for edit mode.
      var input = cr.doc.createElement('input');
      input.type = 'text';
      this.appendChild(input);
      input.className = 'exceptionPattern hidden';

      var select = cr.doc.createElement('select');
      var optionAllow = cr.doc.createElement('option');
      optionAllow.textContent = templateData.allowException;
      select.appendChild(optionAllow);

      if (this.enableAskOption) {
        var optionAsk = cr.doc.createElement('option');
        optionAsk.textContent = templateData.askException;
        select.appendChild(optionAsk);
        this.optionAsk = optionAsk;
      }

      if (this.contentType == 'cookies') {
        var optionSession = cr.doc.createElement('option');
        optionSession.textContent = templateData.sessionException;
        select.appendChild(optionSession);
        this.optionSession = optionSession;
      }

      var optionBlock = cr.doc.createElement('option');
      optionBlock.textContent = templateData.blockException;
      select.appendChild(optionBlock);

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

      this.input = input;
      this.select = select;
      this.optionAllow = optionAllow;
      this.optionBlock = optionBlock;

      this.updateEditables();

      var listItem = this;

      this.addEventListener('selectedChange', function(event) {
        // Editing notifications and geolocation is disabled for now.
        if (listItem.contentType == 'notifications' ||
            listItem.contentType == 'location')
          return;

        listItem.editing = listItem.selected;
      });

      // Handle events on the editable nodes.
      input.oninput = function(event) {
        listItem.inputValidityKnown = false;
        chrome.send('checkExceptionPatternValidity',
                    [listItem.contentType, listItem.mode, input.value]);
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
            listItem.setPatternValid(true);
          case 'Enter':
            listItem.ownerDocument.activeElement.blur();
        }
      }

      input.addEventListener('keydown', handleKeydown);
      select.addEventListener('keydown', handleKeydown);
    },

    /**
     * The pattern (e.g., a URL) for the exception.
     * @type {string}
     */
    get pattern() {
      return this.dataItem['displayPattern'];
    },
    set pattern(pattern) {
      this.dataItem['displayPattern'] = pattern;
    },

    /**
     * The setting (allow/block) for the exception.
     * @type {string}
     */
    get setting() {
      return this.dataItem['setting'];
    },
    set setting(setting) {
      this.dataItem['setting'] = setting;
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
      else if (setting == 'ask')
        return templateData.askException;
      else if (setting == 'session')
        return templateData.sessionException;
    },

    /**
     * Update this list item to reflect whether the input is a valid pattern.
     * @param {boolean} valid Whether said pattern is valid in the context of
     *     a content exception setting.
     */
    setPatternValid: function(valid) {
      if (valid || !this.input.value)
        this.input.setCustomValidity('');
      else
        this.input.setCustomValidity(' ');
      this.inputIsValid = valid;
      this.inputValidityKnown = true;
    },

    /**
     * Set the <input> to its original contents. Used when the user quits
     * editing.
     */
    resetInput: function() {
      this.input.value = this.pattern;
    },

    /**
     * Copy the data model values to the editable nodes.
     */
    updateEditables: function() {
      this.resetInput();

      if (this.setting == 'allow')
        this.optionAllow.selected = true;
      else if (this.setting == 'block')
        this.optionBlock.selected = true;
      else if (this.setting == 'session' && this.optionSession)
        this.optionSession.selected = true;
      else if (this.setting == 'ask' && this.optionAsk)
        this.optionAsk.selected = true;
    },

    /**
     * Fiddle with the display of elements of this list item when the editing
     * mode changes.
     */
    toggleVisibilityForEditing: function() {
      this.patternLabel.classList.toggle('hidden');
      this.settingLabel.classList.toggle('hidden');
      this.input.classList.toggle('hidden');
      this.select.classList.toggle('hidden');
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

      var input = this.input;

      this.toggleVisibilityForEditing();

      if (editing) {
        this.setAttribute('editing', '');
        cr.ui.limitInputWidth(input, this, 20);
        // When this is called in response to the selectedChange event,
        // the list grabs focus immediately afterwards. Thus we must delay
        // our focus grab.
        window.setTimeout(function() {
          input.focus();
          input.select();
        }, 50);

        // TODO(estade): should we insert example text here for the AddNewRow
        // input?
      } else {
        this.removeAttribute('editing');

        // Check that we have a valid pattern and if not we do not, abort
        // changes to the exception.
        if (!this.inputValidityKnown || !this.inputIsValid) {
          this.updateEditables();
          this.setPatternValid(true);
          return;
        }

        var newPattern = input.value;

        var newSetting;
        var optionAllow = this.optionAllow;
        var optionBlock = this.optionBlock;
        var optionSession = this.optionSession;
        var optionAsk = this.optionAsk;
        if (optionAllow.selected)
          newSetting = 'allow';
        else if (optionBlock.selected)
          newSetting = 'block';
        else if (optionSession && optionSession.selected)
          newSetting = 'session';
        else if (optionAsk && optionAsk.selected)
          newSetting = 'ask';

        this.finishEdit(newPattern, newSetting);
      }
    },

    /**
     * Editing is complete; update the model.
     * @type {string} newPattern The pattern that the user entered.
     * @type {string} newSetting The setting the user chose.
     */
    finishEdit: function(newPattern, newSetting) {
      // Empty edit - do nothing.
      if (newPattern == this.pattern && newSetting == this.setting)
        return;

      this.patternLabel.textContent = newPattern;
      this.settingLabel.textContent = this.settingForDisplay();
      var oldPattern = this.pattern;
      this.pattern = newPattern;
      this.setting = newSetting;

      if (oldPattern != newPattern) {
        chrome.send('removeExceptions',
                    [this.contentType, this.mode, oldPattern]);
      }

      chrome.send('setException',
                  [this.contentType, this.mode, newPattern, newSetting]);
    }
  };

  /**
   * Creates a new list item for the Add New Item row, which doesn't represent
   * an actual entry in the exceptions list but allows the user to add new
   * exceptions.
   * @param {string} contentType The type of the list.
   * @param {string} mode The browser mode, 'otr' or 'normal'.
   * @param {boolean} enableAskOption Whether to show an 'ask every time'
   *     option in the select.
   * @constructor
   * @extends {cr.ui.ExceptionsListItem}
   */
  function ExceptionsAddRowListItem(contentType, mode, enableAskOption) {
    var el = cr.doc.createElement('li');
    el.mode = mode;
    el.contentType = contentType;
    el.enableAskOption = enableAskOption;
    el.dataItem = [];
    el.__proto__ = ExceptionsAddRowListItem.prototype;
    el.decorate();

    return el;
  }

  ExceptionsAddRowListItem.prototype = {
    __proto__: ExceptionsListItem.prototype,

    decorate: function() {
      ExceptionsListItem.prototype.decorate.call(this);

      this.input.placeholder = templateData.addNewExceptionInstructions;
      this.input.classList.remove('hidden');
      this.select.classList.remove('hidden');

      // Do we always want a default of allow?
      this.setting = 'allow';
    },

    /**
     * Clear the <input> and let the placeholder text show again.
     */
    resetInput: function() {
      this.input.value = '';
    },

    /**
     * No elements show or hide when going into edit mode, so do nothing.
     */
    toggleVisibilityForEditing: function() {
      // No-op.
    },

    /**
     * Editing is complete; update the model. As long as the pattern isn't
     * empty, we'll just add it.
     * @type {string} newPattern The pattern that the user entered.
     * @type {string} newSetting The setting the user chose.
     */
    finishEdit: function(newPattern, newSetting) {
      if (newPattern == '')
        return;

      chrome.send('setException',
                  [this.contentType, this.mode, newPattern, newSetting]);
    },
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

      this.contentType = this.parentNode.getAttribute('contentType');
      this.mode = this.getAttribute('mode');

      var exceptionList = this;
      function handleBlur(e) {
        // When the blur event happens we do not know who is getting focus so we
        // delay this a bit until we know if the new focus node is outside the
        // list.
        var doc = e.target.ownerDocument;
        window.setTimeout(function() {
          var activeElement = doc.activeElement;
          if (!exceptionList.contains(activeElement))
            exceptionList.selectionModel.clear();
        }, 50);
      }

      this.addEventListener('blur', handleBlur, true);

      // Whether the exceptions in this list allow an 'Ask every time' option.
      this.enableAskOption = (this.contentType == 'plugins' &&
                              templateData.enable_click_to_play);

      this.reset();
    },

    /**
     * Creates an item to go in the list.
     * @param {Object} entry The element from the data model for this row.
     */
    createItem: function(entry) {
      if (entry) {
        return new ExceptionsListItem(this.contentType,
                                      this.mode,
                                      this.enableAskOption,
                                      entry);
      } else {
        return new ExceptionsAddRowListItem(this.contentType,
                                            this.mode,
                                            this.enableAskOption);
      }
    },

    /**
     * Adds an exception to the js model.
     * @param {Object} entry A dictionary of values for the exception.
     */
    addException: function(entry) {
      this.dataModel.push(entry);
    },

    /**
     * The browser has finished checking a pattern for validity. Update the
     * list item to reflect this.
     * @param {string} pattern The pattern.
     * @param {bool} valid Whether said pattern is valid in the context of
     *     a content exception setting.
     */
    patternValidityCheckComplete: function(pattern, valid) {
      var listItems = this.items;
      for (var i = 0; i < listItems.length; i++) {
        var listItem = listItems[i];
        // Don't do anything for messages for the item if it is not the intended
        // recipient, or if the response is stale (i.e. the input value has
        // changed since we sent the request to analyze it).
        if (pattern == listItem.input.value)
          listItem.setPatternValid(valid);
      }
    },

    /**
     * Removes all exceptions from the js model.
     */
    reset: function() {
      // The null creates the Add New Exception row.
      this.dataModel = new ArrayDataModel([null]);
    },

    /**
     * Removes all selected rows from browser's model.
     */
    removeSelectedRows: function() {
      // The first member is the content type; the rest of the values describe
      // the patterns we are removing.
      var args = [this.contentType];
      var selectedItems = this.selectedItems;
      for (var i = 0; i < selectedItems.length; i++) {
        if (this.contentType == 'location') {
          args.push(selectedItems[i]['origin']);
          args.push(selectedItems[i]['embeddingOrigin']);
        } else if (this.contentType == 'notifications') {
          args.push(selectedItems[i]['origin']);
          args.push(selectedItems[i]['setting']);
        } else {
          args.push(this.mode);
          args.push(selectedItems[i]['displayPattern']);
        }
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

  return {
    ExceptionsListItem: ExceptionsListItem,
    ExceptionsAddRowListItem: ExceptionsAddRowListItem,
    ExceptionsList: ExceptionsList,
  };
});
