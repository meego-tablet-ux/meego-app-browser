// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This script contains privileged chrome extension related javascript APIs.
// It is loaded by pages whose URL has the chrome-extension protocol.

var chrome = chrome || {};
(function() {
  native function GetExtensionAPIDefinition();
  native function StartRequest();
  native function GetCurrentPageActions(extensionId);
  native function GetExtensionViews();
  native function GetChromeHidden();
  native function GetNextRequestId();
  native function OpenChannelToTab();
  native function GetRenderViewId();
  native function GetPopupParentWindow();
  native function GetPopupView();
  native function SetExtensionActionIcon();
  native function IsExtensionProcess();

  var chromeHidden = GetChromeHidden();

  // These bindings are for the extension process only. Since a chrome-extension
  // URL can be loaded in an iframe of a regular renderer, we check here to
  // ensure we don't expose the APIs in that case.
  if (!IsExtensionProcess()) {
    chromeHidden.onLoad.addListener(function (extensionId) {
      chrome.initExtension(extensionId, false);
    });
    return;
  }

  if (!chrome)
    chrome = {};

  // Validate arguments.
  chromeHidden.validationTypes = [];
  chromeHidden.validate = function(args, schemas) {
    if (args.length > schemas.length)
      throw new Error("Too many arguments.");

    for (var i = 0; i < schemas.length; i++) {
      if (i in args && args[i] !== null && args[i] !== undefined) {
        var validator = new chromeHidden.JSONSchemaValidator();
        validator.addTypes(chromeHidden.validationTypes);
        validator.validate(args[i], schemas[i]);
        if (validator.errors.length == 0)
          continue;

        var message = "Invalid value for argument " + i + ". ";
        for (var i = 0, err; err = validator.errors[i]; i++) {
          if (err.path) {
            message += "Property '" + err.path + "': ";
          }
          message += err.message;
          message = message.substring(0, message.length - 1);
          message += ", ";
        }
        message = message.substring(0, message.length - 2);
        message += ".";

        throw new Error(message);
      } else if (!schemas[i].optional) {
        throw new Error("Parameter " + i + " is required.");
      }
    }
  }

  // Callback handling.
  var requests = [];
  chromeHidden.handleResponse = function(requestId, name,
                                         success, response, error) {
    try {
      var request = requests[requestId];
      if (success) {
        delete chrome.extension.lastError;
      } else {
        if (!error) {
          error = "Unknown error."
        }
        console.error("Error during " + name + ": " + error);
        chrome.extension.lastError = {
          "message": error
        };
      }

      if (request.customCallback) {
        request.customCallback(name, request, response);
      }

      if (request.callback) {
        // Callbacks currently only support one callback argument.
        var callbackArgs = response ? [chromeHidden.JSON.parse(response)] : [];

        // Validate callback in debug only -- and only when the
        // caller has provided a callback. Implementations of api
        // calls my not return data if they observe the caller
        // has not provided a callback.
        if (chromeHidden.validateCallbacks && !error) {
          try {
            if (!request.callbackSchema.parameters) {
              throw "No callback schemas defined";
            }

            if (request.callbackSchema.parameters.length > 1) {
              throw "Callbacks may only define one parameter";
            }

            chromeHidden.validate(callbackArgs,
                request.callbackSchema.parameters);
          } catch (exception) {
            return "Callback validation error during " + name + " -- " +
                   exception;
          }
        }

        if (response) {
          request.callback(callbackArgs[0]);
        } else {
          request.callback();
        }
      }
    } finally {
      delete requests[requestId];
      delete chrome.extension.lastError;
    }
  };

  chromeHidden.setViewType = function(type) {
    var modeClass = "chrome-" + type;
    var className = document.documentElement.className;
    if (className && className.length) {
      var classes = className.split(" ");
      var new_classes = [];
      classes.forEach(function(cls) {
        if (cls.indexOf("chrome-") != 0) {
          new_classes.push(cls);
        }
      });
      new_classes.push(modeClass);
      document.documentElement.className = new_classes.join(" ");
    } else {
      document.documentElement.className = modeClass;
    }
  };

  function prepareRequest(args, argSchemas) {
    var request = {};
    var argCount = args.length;

    // Look for callback param.
    if (argSchemas.length > 0 &&
        args.length == argSchemas.length &&
        argSchemas[argSchemas.length - 1].type == "function") {
      request.callback = args[argSchemas.length - 1];
      request.callbackSchema = argSchemas[argSchemas.length - 1];
      --argCount;
    }

    request.args = [];
    for (var k = 0; k < argCount; k++) {
      request.args[k] = args[k];
    }

    return request;
  }

  // Send an API request and optionally register a callback.
  function sendRequest(functionName, args, argSchemas, customCallback) {
    var request = prepareRequest(args, argSchemas);
    if (customCallback) {
      request.customCallback = customCallback;
    }
    // JSON.stringify doesn't support a root object which is undefined.
    if (request.args === undefined)
      request.args = null;

    var sargs = chromeHidden.JSON.stringify(request.args);

    var requestId = GetNextRequestId();
    requests[requestId] = request;
    var hasCallback = (request.callback || customCallback) ? true : false;
    return StartRequest(functionName, sargs, requestId, hasCallback);
  }

  // Send a special API request that is not JSON stringifiable, and optionally
  // register a callback.
  function sendCustomRequest(nativeFunction, functionName, args, argSchemas) {
    var request = prepareRequest(args, argSchemas);
    var requestId = GetNextRequestId();
    requests[requestId] = request;
    return nativeFunction(functionName, request.args, requestId,
                          request.callback ? true : false);
  }

  function bind(obj, func) {
    return function() {
      return func.apply(obj, arguments);
    };
  }

  // Helper function for positioning pop-up windows relative to DOM objects.
  // Returns the absolute position of the given element relative to the hosting
  // browser frame.
  function findAbsolutePosition(domElement) {
    var left = domElement.offsetLeft;
    var top = domElement.offsetTop;

    // Ascend through the parent hierarchy, taking into account object nesting
    // and scoll positions.
    for (var parentElement = domElement.offsetParent; parentElement;
         parentElement = parentElement.offsetParent) {
      left += parentElement.offsetLeft;
      top += parentElement.offsetTop;

      left -= parentElement.scrollLeft;
      top -= parentElement.scrollTop;
    }

    return {
      top: top,
      left: left
    };
  }

  // Returns the coordiates of the rectangle encompassing the domElement,
  // in browser coordinates relative to the frame hosting the element.
  function getAbsoluteRect(domElement) {
    var rect = findAbsolutePosition(domElement);
    rect.width = domElement.offsetWidth || 0;
    rect.height = domElement.offsetHeight || 0;
    return rect;
  }

  // --- Setup additional api's not currently handled in common/extensions/api

  // Page action events send (pageActionId, {tabId, tabUrl}).
  function setupPageActionEvents(extensionId) {
    var pageActions = GetCurrentPageActions(extensionId);

    var oldStyleEventName = "pageActions/" + extensionId;
    // TODO(EXTENSIONS_DEPRECATED): only one page action
    for (var i = 0; i < pageActions.length; ++i) {
      // Setup events for each extension_id/page_action_id string we find.
      chrome.pageActions[pageActions[i]] = new chrome.Event(oldStyleEventName);
    }

    // Note this is singular.
    var eventName = "pageAction/" + extensionId;
    chrome.pageAction = chrome.pageAction || {};
    chrome.pageAction.onClicked = new chrome.Event(eventName);
  }

  // Browser action events send {windowpId}.
  function setupBrowserActionEvent(extensionId) {
    var eventName = "browserAction/" + extensionId;
    chrome.browserAction = chrome.browserAction || {};
    chrome.browserAction.onClicked = new chrome.Event(eventName);
  }

  function setupToolstripEvents(renderViewId) {
    chrome.toolstrip = chrome.toolstrip || {};
    chrome.toolstrip.onExpanded =
        new chrome.Event("toolstrip.onExpanded." + renderViewId);
    chrome.toolstrip.onCollapsed =
        new chrome.Event("toolstrip.onCollapsed." + renderViewId);
  }

  function setupPopupEvents(renderViewId) {
    chrome.experimental.popup = chrome.experimental.popup || {};
    chrome.experimental.popup.onClosed =
      new chrome.Event("experimental.popup.onClosed." + renderViewId);
  }

  function setupHiddenContextMenuEvent(extensionId) {
    var eventName = "contextMenu/" + extensionId;
    chromeHidden.contextMenuEvent = new chrome.Event(eventName);
    chromeHidden.contextMenuHandlers = {};
    chromeHidden.contextMenuEvent.addListener(function() {
      var menuItemId = arguments[0].menuItemId;
      var onclick = chromeHidden.contextMenuHandlers[menuItemId];
      if (onclick) {
        onclick.apply(onclick, arguments);
      }

      var parentMenuItemId = arguments[0].parentMenuItemId;
      var parentOnclick = chromeHidden.contextMenuHandlers[parentMenuItemId];
      if (parentOnclick) {
        parentOnclick.apply(parentOnclick, arguments);
      }
    });
  }

  function setupOmniboxEvents(extensionId) {
    chrome.experimental.omnibox.onInputEntered =
        new chrome.Event("experimental.omnibox.onInputEntered/" + extensionId);

    chrome.experimental.omnibox.onInputChanged =
        new chrome.Event("experimental.omnibox.onInputChanged/" + extensionId);
    chrome.experimental.omnibox.onInputChanged.dispatch =
        function(text, requestId) {
      var suggestCallback = function(suggestions) {
        chrome.experimental.omnibox.sendSuggestions(requestId, suggestions);
      }
      chrome.Event.prototype.dispatch.apply(this, [text, suggestCallback]);
    };
  }

  chromeHidden.onLoad.addListener(function (extensionId) {
    chrome.initExtension(extensionId, false);

    // |apiFunctions| is a hash of name -> object that stores the
    // name & definition of the apiFunction. Custom handling of api functions
    // is implemented by adding a "handleRequest" function to the object.
    var apiFunctions = {};

    // Read api definitions and setup api functions in the chrome namespace.
    // TODO(rafaelw): Consider defining a json schema for an api definition
    //   and validating either here, in a unit_test or both.
    // TODO(rafaelw): Handle synchronous functions.
    // TOOD(rafaelw): Consider providing some convenient override points
    //   for api functions that wish to insert themselves into the call.
    var apiDefinitions = chromeHidden.JSON.parse(GetExtensionAPIDefinition());

    apiDefinitions.forEach(function(apiDef) {
      var module = chrome;
      var namespaces = apiDef.namespace.split('.');
      for (var index = 0, name; name = namespaces[index]; index++) {
        module[name] = module[name] || {};
        module = module[name];
      };

      // Add types to global validationTypes
      if (apiDef.types) {
        apiDef.types.forEach(function(t) {
          chromeHidden.validationTypes.push(t);
        });
      }

      // Setup Functions.
      if (apiDef.functions) {
        apiDef.functions.forEach(function(functionDef) {
          // Module functions may have been defined earlier by hand. Don't
          // clobber them.
          if (module[functionDef.name])
            return;

          var apiFunction = {};
          apiFunction.definition = functionDef;
          apiFunction.name = apiDef.namespace + "." + functionDef.name;
          apiFunctions[apiFunction.name] = apiFunction;

          module[functionDef.name] = bind(apiFunction, function() {
            var args = arguments;
            if (this.updateArguments) {
              // Functions whose signature has changed can define an
              // |updateArguments| function to transform old argument lists
              // into the new form, preserving compatibility.
              // TODO(skerner): Once optional args can be omitted (crbug/29215),
              // this mechanism will become unnecessary.  Consider removing it
              // when crbug/29215 is fixed.
              args = this.updateArguments.apply(this, args);
            }
            chromeHidden.validate(args, this.definition.parameters);

            var retval;
            if (this.handleRequest) {
              retval = this.handleRequest.apply(this, args);
            } else {
              retval = sendRequest(this.name, args,
                                   this.definition.parameters,
                                   this.customCallback);
            }

            // Validate return value if defined - only in debug.
            if (chromeHidden.validateCallbacks &&
                chromeHidden.validate &&
                this.definition.returns) {
              chromeHidden.validate([retval], [this.definition.returns]);
            }
            return retval;
          });
        });
      }

      // Setup Events
      if (apiDef.events) {
        apiDef.events.forEach(function(eventDef) {
          // Module events may have been defined earlier by hand. Don't clobber
          // them.
          if (module[eventDef.name])
            return;

          var eventName = apiDef.namespace + "." + eventDef.name;
          module[eventDef.name] = new chrome.Event(eventName,
              eventDef.parameters);
        });
      }


      // getTabContentses is retained for backwards compatibility
      // See http://crbug.com/21433
      chrome.extension.getTabContentses = chrome.extension.getExtensionTabs
    });

    apiFunctions["tabs.connect"].handleRequest = function(tabId, connectInfo) {
      var name = "";
      if (connectInfo) {
        name = connectInfo.name || name;
      }
      var portId = OpenChannelToTab(
          tabId, chromeHidden.extensionId, name);
      return chromeHidden.Port.createPort(portId, name);
    };

    apiFunctions["tabs.sendRequest"].handleRequest =
        function(tabId, request, responseCallback) {
      var port = chrome.tabs.connect(tabId,
                                     {name: chromeHidden.kRequestChannel});
      port.postMessage(request);
      port.onMessage.addListener(function(response) {
        if (responseCallback)
          responseCallback(response);
        port.disconnect();
      });
    };

    apiFunctions["extension.getViews"].handleRequest = function(properties) {
      var windowId = -1;
      var type = "ALL";
      if (typeof(properties) != "undefined") {
        if (typeof(properties.type) != "undefined") {
          type = properties.type;
        }
        if (typeof(properties.windowId) != "undefined") {
          windowId = properties.windowId;
        }
      }
      return GetExtensionViews(windowId, type) || null;
    };

    apiFunctions["extension.getBackgroundPage"].handleRequest = function() {
      return GetExtensionViews(-1, "BACKGROUND")[0] || null;
    };

    apiFunctions["extension.getToolstrips"].handleRequest =
        function(windowId) {
      if (typeof(windowId) == "undefined")
        windowId = -1;
      return GetExtensionViews(windowId, "TOOLSTRIP");
    };

    apiFunctions["extension.getExtensionTabs"].handleRequest =
        function(windowId) {
      if (typeof(windowId) == "undefined")
        windowId = -1;
      return GetExtensionViews(windowId, "TAB");
    };

    apiFunctions["devtools.getTabEvents"].handleRequest = function(tabId) {
      var tabIdProxy = {};
      var functions = ["onPageEvent", "onTabClose"];
      functions.forEach(function(name) {
        // Event disambiguation is handled by name munging.  See
        // chrome/browser/extensions/extension_devtools_events.h for the C++
        // equivalent of this logic.
        tabIdProxy[name] = new chrome.Event("devtools." + tabId + "." + name);
      });
      return tabIdProxy;
    };

    apiFunctions["experimental.popup.show"].handleRequest =
        function(url, showDetails, callback) {
      // Second argument is a transform from HTMLElement to Rect.
      var internalSchema = [
        this.definition.parameters[0],
        {
          type: "object",
          name: "showDetails",
          properties: {
            domAnchor: {
              type: "object",
              properties: {
                top: { type: "integer", minimum: 0 },
                left: { type: "integer", minimum: 0 },
                width: { type: "integer", minimum: 0 },
                height: { type: "integer", minimum: 0 }
              }
            },
            giveFocus: {
              type: "boolean",
              optional: true
            },
            borderStyle: {
              type: "string",
              optional: true,
              enum: ["bubble", "rectangle"]
            }
          }
        },
        this.definition.parameters[2]
      ];
      return sendRequest(this.name,
                         [url,
                          {
                            domAnchor: getAbsoluteRect(showDetails.relativeTo),
                            giveFocus: showDetails.giveFocus,
                            borderStyle: showDetails.borderStyle
                          },
                          callback],
                         internalSchema);
    };

    apiFunctions["experimental.extension.getPopupView"].handleRequest =
        function() {
      return GetPopupView();
    };

    apiFunctions["experimental.popup.getParentWindow"].handleRequest =
        function() {
      return GetPopupParentWindow();
    };

    var canvas;
    function setIconCommon(details, name, parameters, actionType) {
      var EXTENSION_ACTION_ICON_SIZE = 19;

      if ("iconIndex" in details) {
        sendRequest(name, [details], parameters);
      } else if ("imageData" in details) {
        // Verify that this at least looks like an ImageData element.
        // Unfortunately, we cannot use instanceof because the ImageData
        // constructor is not public.
        //
        // We do this manually instead of using JSONSchema to avoid having these
        // properties show up in the doc.
        if (!("width" in details.imageData) ||
            !("height" in details.imageData) ||
            !("data" in details.imageData)) {
          throw new Error(
              "The imageData property must contain an ImageData object.");
        }

        if (details.imageData.width > EXTENSION_ACTION_ICON_SIZE ||
            details.imageData.height > EXTENSION_ACTION_ICON_SIZE) {
          throw new Error(
              "The imageData property must contain an ImageData object that " +
              "is no larger than 19 pixels square.");
        }

        sendCustomRequest(SetExtensionActionIcon, name, [details], parameters);
      } else if ("path" in details) {
        var img = new Image();
        img.onerror = function() {
          console.error("Could not load " + actionType + " icon '" +
                        details.path + "'.");
        }
        img.onload = function() {
          var canvas = document.createElement("canvas");
          canvas.width = img.width > EXTENSION_ACTION_ICON_SIZE ?
              EXTENSION_ACTION_ICON_SIZE : img.width;
          canvas.height = img.height > EXTENSION_ACTION_ICON_SIZE ?
              EXTENSION_ACTION_ICON_SIZE : img.height;

          var canvas_context = canvas.getContext('2d');
          canvas_context.clearRect(0, 0, canvas.width, canvas.height);
          canvas_context.drawImage(img, 0, 0, canvas.width, canvas.height);
          delete details.path;
          details.imageData = canvas_context.getImageData(0, 0, canvas.width,
                                                          canvas.height);
          sendCustomRequest(SetExtensionActionIcon, name, [details], parameters);
        }
        img.src = details.path;
      } else {
        throw new Error(
            "Either the path or imageData property must be specified.");
      }
    }

    apiFunctions["browserAction.setIcon"].handleRequest = function(details) {
      setIconCommon(
          details, this.name, this.definition.parameters, "browser action");
    };

    apiFunctions["pageAction.setIcon"].handleRequest = function(details) {
      setIconCommon(
          details, this.name, this.definition.parameters, "page action");
    };

    apiFunctions["experimental.contextMenu.create"].customCallback =
        function(name, request, response) {
      if (chrome.extension.lastError || !response) {
        return;
      }

      // Set up the onclick handler if we were passed one in the request.
      if (request.args.onclick) {
        var menuItemId = chromeHidden.JSON.parse(response);
        chromeHidden.contextMenuHandlers[menuItemId] = request.args.onclick;
      }
    };

    apiFunctions["experimental.contextMenu.remove"].customCallback =
        function(name, request, response) {
      // Remove any onclick handler we had registered for this menu item.
      if (request.args.length > 0) {
        var menuItemId = request.args[0];
        delete chromeHidden.contextMenuHandlers[menuItemId];
      }
    };

    apiFunctions["tabs.captureVisibleTab"].updateArguments = function() {
      // Old signature:
      //    captureVisibleTab(int windowId, function callback);
      // New signature:
      //    captureVisibleTab(int windowId, object details, function callback);
      //
      // TODO(skerner): The next step to omitting optional arguments is the
      // replacement of this code with code that matches arguments by type.
      // Once this is working for captureVisibleTab() it can be enabled for
      // the rest of the API. See crbug/29215 .
      if (arguments.length == 2 && typeof(arguments[1]) == "function") {
        // If the old signature is used, add a null details object.
        newArgs = [arguments[0], null, arguments[1]];
      } else {
        newArgs = arguments;
      }
      return newArgs;
    };

    if (chrome.test) {
      chrome.test.getApiDefinitions = GetExtensionAPIDefinition;
    }

    setupBrowserActionEvent(extensionId);
    setupPageActionEvents(extensionId);
    setupToolstripEvents(GetRenderViewId());
    setupPopupEvents(GetRenderViewId());
    setupHiddenContextMenuEvent(extensionId);
    setupOmniboxEvents(extensionId);
  });

  if (!chrome.experimental)
    chrome.experimental = {};

  if (!chrome.experimental.accessibility)
    chrome.experimental.accessibility = {};
})();
