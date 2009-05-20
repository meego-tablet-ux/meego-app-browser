// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Provides communication interface to remote v8 debugger. See
 * protocol decription at http://code.google.com/p/v8/wiki/DebuggerProtocol
 */
goog.provide('devtools.DebuggerAgent');


/**
 * @constructor
 */
devtools.DebuggerAgent = function() {
  RemoteDebuggerAgent.DebuggerOutput =
      goog.bind(this.handleDebuggerOutput_, this);
  RemoteDebuggerAgent.DidGetContextId = 
      goog.bind(this.didGetContextId_, this);
  RemoteDebuggerAgent.DidGetLogLines =
      goog.bind(this.didGetLogLines_, this);
      
  /**
   * Id of the inspected page global context. It is used for filtering scripts.
   * @type {number} 
   */
  this.contextId_ = null;
      
  /**
   * Mapping from script id to script info.
   * @type {Object}
   */
  this.parsedScripts_ = null;
  
  /**
   * Mapping from the request id to the devtools.BreakpointInfo for the
   * breakpoints whose v8 ids are not set yet. These breakpoints are waiting for
   * 'setbreakpoint' responses to learn their ids in the v8 debugger.
   * @see #handleSetBreakpointResponse_
   * @type {Object}
   */
  this.requestNumberToBreakpointInfo_ = null;
  
  /**
   * Information on current stack top frame.
   * See JavaScriptCallFrame.idl.
   * @type {?devtools.CallFrame}
   */
  this.currentCallFrame_ = null;
  
  /**
   * Whether to stop in the debugger on the exceptions.
   * @type {boolean}
   */
  this.pauseOnExceptions_ = true;
  
  /**
   * Mapping: request sequence number->callback.
   * @type {Object}
   */
  this.requestSeqToCallback_ = null;  

  /**
   * Whether the scripts list has been requested.
   * @type {boolean}
   */
  this.scriptsCacheInitialized_ = false;
  
  /**
   * Whether user has stopped profiling and we are retrieving the rest of
   * profiler's log.
   * @type {boolean}
   */
  this.isProcessingProfile_ = false;
  
  /**
   * The position in log file to read from.
   * @type {number}
   */
  this.lastProfileLogPosition_ = 0;
  
  /**
   * Profiler processor instance.
   * @type {devtools.profiler.Processor}
   */
  this.profilerProcessor_ = new devtools.profiler.Processor();  
};


/**
 * Resets debugger agent to its initial state.
 */
devtools.DebuggerAgent.prototype.reset = function() {
  this.scriptsCacheInitialized_ = false;
  this.contextId_ = null;
  this.parsedScripts_ = {};
  this.requestNumberToBreakpointInfo_ = {};
  this.currentCallFrame_ = null;
  this.requestSeqToCallback_ = {};
};


/**
 * Requests scripts list if it has not been requested yet.
 */
devtools.DebuggerAgent.prototype.initializeScriptsCache = function() {
  if (!this.scriptsCacheInitialized_) {
    this.scriptsCacheInitialized_ = true;
    this.requestScripts();
  }
};


/**
 * Asynchronously requests for all parsed script sources. Response will be
 * processed in handleScriptsResponse_.
 */
devtools.DebuggerAgent.prototype.requestScripts = function() {
  if (this.contextId_ === null) {
    // Update context id first to filter the scripts.
    RemoteDebuggerAgent.GetContextId();
    return;
  }
  var cmd = new devtools.DebugCommand('scripts', {
    'includeSource': false
  });
  devtools.DebuggerAgent.sendCommand_(cmd);
  // Force v8 execution so that it gets to processing the requested command.
  devtools.tools.evaluateJavaScript('javascript:void(0)');
};


/**
 * Asynchronously requests the debugger for the script source.
 * @param {number} scriptId Id of the script whose source should be resolved.
 * @param {function(source:?string):void} callback Function that will be called
 *     when the source resolution is completed. 'source' parameter will be null
 *     if the resolution fails.
 */
devtools.DebuggerAgent.prototype.resolveScriptSource = function(
    scriptId, callback) {
  var script = this.parsedScripts_[scriptId];
  if (!script) {
    callback(null);
    return;
  }

  var cmd = new devtools.DebugCommand('scripts', {
    'ids': [scriptId],
    'includeSource': true
  });
  devtools.DebuggerAgent.sendCommand_(cmd);
  // Force v8 execution so that it gets to processing the requested command.
  devtools.tools.evaluateJavaScript('javascript:void(0)');
  
  this.requestSeqToCallback_[cmd.getSequenceNumber()] = function(msg) {
    if (msg.isSuccess()) {
      var scriptJson = msg.getBody()[0];
      callback(scriptJson.source);
    } else {
      callback(null);
    }
  };
};


/**
 * Tells the v8 debugger to stop on as soon as possible.
 */
devtools.DebuggerAgent.prototype.pauseExecution = function() {
  RemoteDebuggerAgent.DebugBreak();
};


/**
 * @param {number} sourceId Id of the script fot the breakpoint.
 * @param {number} line Number of the line for the breakpoint.
 */
devtools.DebuggerAgent.prototype.addBreakpoint = function(sourceId, line) {
  var script = this.parsedScripts_[sourceId];
  if (!script) {
    return;
  }
  
  line = devtools.DebuggerAgent.webkitToV8LineNumber_(line);
  
  var breakpointInfo = script.getBreakpointInfo(line);
  if (breakpointInfo) {
    return;
  }
  
  breakpointInfo = new devtools.BreakpointInfo(sourceId, line);
  script.addBreakpointInfo(breakpointInfo);

  var cmd = new devtools.DebugCommand('setbreakpoint', {
    'type': 'scriptId',
    'target': sourceId,
    'line': line
  });
  
  this.requestNumberToBreakpointInfo_[cmd.getSequenceNumber()] = breakpointInfo;
  
  devtools.DebuggerAgent.sendCommand_(cmd);
};


/**
 * @param {number} sourceId Id of the script fot the breakpoint.
 * @param {number} line Number of the line for the breakpoint.
 */
devtools.DebuggerAgent.prototype.removeBreakpoint = function(sourceId, line) {
  var script = this.parsedScripts_[sourceId];
  if (!script) {
    return;
  }
  
  line = devtools.DebuggerAgent.webkitToV8LineNumber_(line);

  var breakpointInfo = script.getBreakpointInfo(line);
  script.removeBreakpointInfo(breakpointInfo);
  breakpointInfo.markAsRemoved();

  var id = breakpointInfo.getV8Id();

  // If we don't know id of this breakpoint in the v8 debugger we cannot send
  // 'clearbreakpoint' request. In that case it will be removed in
  // 'setbreakpoint' response handler when we learn the id.
  if (id != -1) {
    this.requestClearBreakpoint_(id);
  }
};


/**
 * Tells the v8 debugger to step into the next statement.
 */
devtools.DebuggerAgent.prototype.stepIntoStatement = function() {
  this.stepCommand_('in');
};


/**
 * Tells the v8 debugger to step out of current function.
 */
devtools.DebuggerAgent.prototype.stepOutOfFunction = function() {
  this.stepCommand_('out');
};


/**
 * Tells the v8 debugger to step over the next statement.
 */
devtools.DebuggerAgent.prototype.stepOverStatement = function() {
  this.stepCommand_('next');
};


/**
 * Tells the v8 debugger to continue execution after it has been stopped on a
 * breakpoint or an exception.
 */
devtools.DebuggerAgent.prototype.resumeExecution = function() {
  var cmd = new devtools.DebugCommand('continue');
  devtools.DebuggerAgent.sendCommand_(cmd);
};


/**
 * @return {boolean} True iff the debugger will pause execution on the
 * exceptions.
 */
devtools.DebuggerAgent.prototype.pauseOnExceptions = function() { 
  return this.pauseOnExceptions_;
};


/**
 * Tells whether to pause in the debugger on the exceptions or not.
 * @param {boolean} value True iff execution should be stopped in the debugger
 * on the exceptions.
 */
devtools.DebuggerAgent.prototype.setPauseOnExceptions = function(value) {
  this.pauseOnExceptions_ = value;
};


/**
 * Current stack top frame.
 * @return {devtools.CallFrame}
 */
devtools.DebuggerAgent.prototype.getCurrentCallFrame = function() {
  return this.currentCallFrame_;
};


/**
 * Sends 'evaluate' request to the debugger.
 * @param {Object} arguments Request arguments map.
 * @param {function(devtools.DebuggerMessage)} callback Callback to be called
 *     when response is received.
 */
devtools.DebuggerAgent.prototype.requestEvaluate = function(
    arguments, callback) {
  var cmd = new devtools.DebugCommand('evaluate', arguments);
  devtools.DebuggerAgent.sendCommand_(cmd);
  this.requestSeqToCallback_[cmd.getSequenceNumber()] = callback;
};


/**
 * Sends 'lookup' request for each unresolved property of the object. When
 * response is received the properties will be changed with their resolved
 * values.
 * @param {Object} object Object whose properties should be resolved.
 * @param {function(devtools.DebuggerMessage)} Callback to be called when all
 *     children are resolved.
 */
devtools.DebuggerAgent.prototype.resolveChildren = function(object, callback) {
  if ('ref' in object) {
    this.requestLookup_([object.ref], function(msg) {
      var result = {};
      if (msg.isSuccess()) {
        var handleToObject = msg.getBody();
        var resolved = handleToObject[object.ref];
        devtools.DebuggerAgent.formatObjectProperties_(resolved, result);
      } else {
        result.error = 'Failed to resolve children: ' + msg.getMessage();
      }
      object.resolvedValue = result;
      callback(object);
    });
    
    return;
  } else {
    if (!object.resolvedValue) {
      var message = 'Corrupted object: ' + JSON.stringify(object);
      object.resolvedValue = {};
      object.resolvedValue.error = message;
    }
    callback(object);
  }
};


/**
 * Starts (resumes) profiling.
 */
devtools.DebuggerAgent.prototype.startProfiling = function() {
  if (this.isProcessingProfile_) {
    return;
  }  
  RemoteDebuggerAgent.StartProfiling();
  RemoteDebuggerAgent.GetLogLines(this.lastProfileLogPosition_);
  WebInspector.setRecordingProfile(true);
};


/**
 * Stops (pauses) profiling.
 */
devtools.DebuggerAgent.prototype.stopProfiling = function() {
  this.isProcessingProfile_ = true;
  RemoteDebuggerAgent.StopProfiling();
};


/**
 * Removes specified breakpoint from the v8 debugger.
 * @param {number} breakpointId Id of the breakpoint in the v8 debugger.
 */
devtools.DebuggerAgent.prototype.requestClearBreakpoint_ = function(
    breakpointId) {
  var cmd = new devtools.DebugCommand('clearbreakpoint', {
    'breakpoint': breakpointId
  });
  devtools.DebuggerAgent.sendCommand_(cmd);
};


/**
 * Sends 'backtrace' request to v8.
 */
devtools.DebuggerAgent.prototype.requestBacktrace_ = function() {
  var cmd = new devtools.DebugCommand('backtrace', {
    'compactFormat':true
  });
  devtools.DebuggerAgent.sendCommand_(cmd);
};


/**
 * Sends command to v8 debugger.
 * @param {devtools.DebugCommand} cmd Command to execute.
 */
devtools.DebuggerAgent.sendCommand_ = function(cmd) {
  RemoteDebuggerCommandExecutor.DebuggerCommand(cmd.toJSONProtocol());
};


/**
 * Tells the v8 debugger to make the next execution step.
 * @param {string} action 'in', 'out' or 'next' action.
 */
devtools.DebuggerAgent.prototype.stepCommand_ = function(action) {
  var cmd = new devtools.DebugCommand('continue', {
    'stepaction': action,
    'stepcount': 1
  });
  devtools.DebuggerAgent.sendCommand_(cmd);
};


/**
 * Sends 'lookup' request to v8.
 * @param {number} handle Handle to the object to lookup.
 */
devtools.DebuggerAgent.prototype.requestLookup_ = function(handles, callback) {
  var cmd = new devtools.DebugCommand('lookup', {
    'compactFormat':true,
    'handles': handles
  });
  devtools.DebuggerAgent.sendCommand_(cmd);
  this.requestSeqToCallback_[cmd.getSequenceNumber()] = callback;
};


/**
 * Handles GetContextId response.
 * @param {number} contextId Id of the inspected page global context.
 */
devtools.DebuggerAgent.prototype.didGetContextId_ = function(contextId) {
  this.contextId_ = contextId;
  // Update scripts.
  this.requestScripts();
};


/**
 * Handles output sent by v8 debugger. The output is either asynchronous event
 * or response to a previously sent request.  See protocol definitioun for more
 * details on the output format.
 * @param {string} output
 */
devtools.DebuggerAgent.prototype.handleDebuggerOutput_ = function(output) {
  var msg;
  try {
    msg = new devtools.DebuggerMessage(output);
  } catch(e) {
    debugPrint('Failed to handle debugger reponse:\n' + e);
    throw e;
  }
  
  
  if (msg.getType() == 'event') {
    if (msg.getEvent() == 'break') {
      this.handleBreakEvent_(msg);
    } else if (msg.getEvent() == 'exception') {
      this.handleExceptionEvent_(msg);
    } else if (msg.getEvent() == 'afterCompile') {
      this.handleAfterCompileEvent_(msg);
    }
  } else if (msg.getType() == 'response') {
    if (msg.getCommand() == 'scripts') {
      this.handleScriptsResponse_(msg);
    } else if (msg.getCommand() == 'setbreakpoint') {
      this.handleSetBreakpointResponse_(msg);
    } else if (msg.getCommand() == 'clearbreakpoint') {
      this.handleClearBreakpointResponse_(msg);
    } else if (msg.getCommand() == 'backtrace') {
      this.handleBacktraceResponse_(msg);
    } else if (msg.getCommand() == 'lookup') {
      this.invokeCallbackForResponse_(msg);
    } else if (msg.getCommand() == 'evaluate') {
      this.invokeCallbackForResponse_(msg);
    }
  }
};


/**
 * @param {devtools.DebuggerMessage} msg
 */
devtools.DebuggerAgent.prototype.handleBreakEvent_ = function(msg) {
  var body = msg.getBody();

  var line = devtools.DebuggerAgent.v8ToWwebkitLineNumber_(body.sourceLine);
  this.currentCallFrame_ = new devtools.CallFrame();
  this.currentCallFrame_.sourceID = body.script.id;
  this.currentCallFrame_.line = line;
  this.currentCallFrame_.script = body.script;
  this.requestBacktrace_();
};


/**
 * @param {devtools.DebuggerMessage} msg
 */
devtools.DebuggerAgent.prototype.handleExceptionEvent_ = function(msg) {
  var body = msg.getBody();
  debugPrint('Uncaught exception in ' + body.script.name + ':' +
             body.sourceLine + '\n' + body.sourceLineText);
  if (this.pauseOnExceptions_) {
    var body = msg.getBody();

    var sourceId = -1;
    // The exception may happen in native code in which case there is no script.
    if (body.script) {
      sourceId = body.script.id;
    }
    
    var line = devtools.DebuggerAgent.v8ToWwebkitLineNumber_(body.sourceLine);
    
    this.currentCallFrame_ = new devtools.CallFrame();
    this.currentCallFrame_.sourceID = sourceId;
    this.currentCallFrame_.line = line;
    this.currentCallFrame_.script = body.script;
    this.requestBacktrace_();
  } else {             
    this.resumeExecution();
  }
};


/**
 * @param {devtools.DebuggerMessage} msg
 */
devtools.DebuggerAgent.prototype.handleScriptsResponse_ = function(msg) {
  if (this.invokeCallbackForResponse_(msg)) {
    return;
  }

  var scripts = msg.getBody();
  for (var i = 0; i < scripts.length; i++) {
    var script = scripts[i];

    // Skip scripts from other tabs.
    if (!this.isScriptFromInspectedContext_(script, msg)) {
      continue;
    }

    // We may already have received the info in an afterCompile event.
    if (script.id in this.parsedScripts_) {
      continue;
    }
    this.addScriptInfo_(script);
  }
};


/**
 * @param {Object} script Json object representing script.
 * @param {devtools.DebuggerMessage} msg Debugger response.
 */
devtools.DebuggerAgent.prototype.isScriptFromInspectedContext_ = function(
    script, msg) {
  if (!script.context) {
    // Always ignore scripts from the utility context.
    return false;
  }
  var context = msg.lookup(script.context.ref);
  var scriptContextId = context.data;
  if (!goog.isDef(scriptContextId)) {
    return false; // Always ignore scripts from the utility context.
  }
  if (this.contextId_ === null) {
    return true;
  }
  return (scriptContextId == this.contextId_);
};


/**
 * @param {devtools.DebuggerMessage} msg
 */
devtools.DebuggerAgent.prototype.handleSetBreakpointResponse_ = function(msg) {
  var requestSeq = msg.getRequestSeq();
  var breakpointInfo = this.requestNumberToBreakpointInfo_[requestSeq]; 
  if (!breakpointInfo) {
    // TODO(yurys): handle this case
    return;
  }
  delete this.requestNumberToBreakpointInfo_[requestSeq];
  if (!msg.isSuccess()) {
    // TODO(yurys): handle this case
    return;
  }
  var idInV8 = msg.getBody().breakpoint;
  breakpointInfo.setV8Id(idInV8);
  
  if (breakpointInfo.isRemoved()) {
    this.requestClearBreakpoint_(idInV8);
  }
};


/**
 * @param {devtools.DebuggerMessage} msg
 */
devtools.DebuggerAgent.prototype.handleAfterCompileEvent_ = function(msg) {
  var script = msg.getBody().script;
  // Ignore scripts from other tabs.
  if (!this.isScriptFromInspectedContext_(script, msg)) {
    return;
  }
  this.addScriptInfo_(script);
};


/**
 * Handles a portion of a profiler log retrieved by GetLogLines call.
 * @param {string} log A portion of profiler log.
 * @param {number} newPosition The position in log file to read from
 *     next time.
 */
devtools.DebuggerAgent.prototype.didGetLogLines_ = function(
    log, newPosition) {
  if (log.length > 0) {
    this.profilerProcessor_.processLogChunk(log);
    this.lastProfileLogPosition_ = newPosition;
  } else if (this.isProcessingProfile_) {
    this.isProcessingProfile_ = false;
    WebInspector.setRecordingProfile(false);           
    WebInspector.addProfile(this.profilerProcessor_.createProfileForView());
    return;
  }
  setTimeout(function() { RemoteDebuggerAgent.GetLogLines(newPosition); },
    this.isProcessingProfile_ ? 100 : 1000);
};


/**
 * Adds the script info to the local cache. This method assumes that the script
 * is not in the cache yet.
 * @param {Object} script Script json object from the debugger message.
 */
devtools.DebuggerAgent.prototype.addScriptInfo_ = function(script) {
  this.parsedScripts_[script.id] = new devtools.ScriptInfo(
      script.id, script.lineOffset);
  WebInspector.parsedScriptSource(
      script.id, script.name, script.source, script.lineOffset);
};


/**
 * @param {devtools.DebuggerMessage} msg
 */
devtools.DebuggerAgent.prototype.handleClearBreakpointResponse_ = function(
    msg) {
  // Do nothing.
};


/**
 * Handles response to 'backtrace' command.
 * @param {devtools.DebuggerMessage} msg
 */
devtools.DebuggerAgent.prototype.handleBacktraceResponse_ = function(msg) {
  if (!this.currentCallFrame_) {
    return;
  }
  
  var script = this.currentCallFrame_.script;
  
  var callerFrame = null;
  var f = null;
  var frames = msg.getBody().frames;
  for (var i = frames.length - 1; i>=0; i--) {
    var nextFrame = frames[i];
    var f = devtools.DebuggerAgent.formatCallFrame_(nextFrame, script, msg);
    f.frameNumber = i;
    f.caller = callerFrame;
    callerFrame = f;
  }
  
  this.currentCallFrame_ = f;
  
  WebInspector.pausedScript();
  DevToolsHost.activateWindow();
};


/**
 * Handles response to a command by invoking its callback (if any).
 * @param {devtools.DebuggerMessage} msg
 * @return {boolean} Whether a callback for the given message was found and
 *     excuted.
 */
devtools.DebuggerAgent.prototype.invokeCallbackForResponse_ = function(msg) {
  var callback = this.requestSeqToCallback_[msg.getRequestSeq()];
  if (!callback) {
    // It may happend if reset was called.
    return false;
  }
  delete this.requestSeqToCallback_[msg.getRequestSeq()];
  callback(msg);
  return true;
};


devtools.DebuggerAgent.prototype.evaluateInCallFrame_ = function(expression) {
};


/**
 * @param {Object} stackFrame Frame json object from 'backtrace' response.
 * @param {Object} script Script json object from 'break' event.
 * @param {devtools.DebuggerMessage} msg Parsed 'backtrace' response.
 * @return {!devtools.CallFrame} Object containing information related to the
 *     call frame in the format expected by ScriptsPanel and its panes.
 */
devtools.DebuggerAgent.formatCallFrame_ = function(stackFrame, script, msg) {
  var sourceId = script.id;
  
  var func = stackFrame.func;
  var sourceId = func.scriptId;
  var funcName = func.name || func.inferredName || '(anonymous function)';
  
  var scope = {};
  
  // Add arguments.
  devtools.DebuggerAgent.argumentsArrayToMap_(stackFrame.arguments, scope);
  
  // Add local variables.
  devtools.DebuggerAgent.propertiesToMap_(stackFrame.locals, scope);

  var thisObject = devtools.DebuggerAgent.formatObjectReference_(
      stackFrame.receiver); 
  // Add variable with name 'this' to the scope.
  scope['this'] = thisObject;
  
  var line = devtools.DebuggerAgent.v8ToWwebkitLineNumber_(stackFrame.line);
  var result = new devtools.CallFrame();
  result.sourceID = sourceId;
  result.line = line;
  result.type = 'function';
  result.functionName = funcName;
  result.localScope = scope;
  result.scopeChain = [scope];
  result.thisObject = thisObject;
  return result;
};


/**
 * Collects properties for an object from the debugger response.
 * @param {Object} object An object from the debugger protocol response.
 * @param {Object} result A map to put the properties in.
 */
devtools.DebuggerAgent.formatObjectProperties_ = function(object, result) {
  devtools.DebuggerAgent.propertiesToMap_(object.properties, result);
  result.protoObject = devtools.DebuggerAgent.formatObjectReference_(
      object.protoObject);
  result.prototypeObject = devtools.DebuggerAgent.formatObjectReference_(
      object.prototypeObject);
  result.constructorFunction = devtools.DebuggerAgent.formatObjectReference_(
      object.constructorFunction);
};


/**
 * For each property in 'properties' puts its name and user-friendly value into
 * 'map'.
 * @param {Array.<Object>} properties Receiver properties or locals array from
 *     'backtrace' response.
 * @param {Object} map Result holder.
 */
devtools.DebuggerAgent.propertiesToMap_ = function(properties, map) {
  for (var j = 0; j < properties.length; j++) {
    var nextValue = properties[j];
    // Skip unnamed properties. They may appear e.g. when number of actual
    // parameters is greater the that of formal. In that case the superfluous
    // parameters will be present in the arguments list as elements without
    // names.
    if (nextValue.name) {
      map[nextValue.name] =
          devtools.DebuggerAgent.formatObjectReference_(nextValue.value);
    }
  }
};


/**
 * Puts arguments from the protocol arguments array to the map assigning names
 * to the anonymous arguments.
 * @param {Array.<Object>} array Arguments array from 'backtrace' response.
 * @param {Object} map Result holder.
 */
devtools.DebuggerAgent.argumentsArrayToMap_ = function(array, map) {
  for (var j = 0; j < array.length; j++) {
    var nextValue = array[j];
    // Skip unnamed properties. They may appear e.g. when number of actual
    // parameters is greater the that of formal. In that case the superfluous
    // parameters will be present in the arguments list as elements without
    // names.
    var name = nextValue.name ? nextValue.name : '<arg #' + j + '>';
    map[name] = devtools.DebuggerAgent.formatObjectReference_(nextValue.value);
  }
};


/**
 * @param {Object} v An object reference from the debugger response. 
 * @return {*} The value representation expected by ScriptsPanel.
 */
devtools.DebuggerAgent.formatObjectReference_ = function(v) {
  if (v.type == 'object') {
    return v;
  } else if (v.type == 'function') {
    var f = function() {};
    f.ref = v.ref;
    return f;
  } else if (goog.isDef(v.value)) {
    return v.value;
  } else if (v.type == 'undefined') {
    return 'undefined';
  } else if (v.type == 'null') {
    return 'null';
  } else if (v.name) {
    return v.name;
  } else if (v.className) {
    return v.className;
  } else {
    return '<unresolved ref: ' + v.ref + ', type: ' + v.type + '>';
  }
};


/**
 * Converts line number from Web Inspector UI(1-based) to v8(0-based).
 * @param {number} line Resource line number in Web Inspector UI.
 * @return {number} The line number in v8.
 */
devtools.DebuggerAgent.webkitToV8LineNumber_ = function(line) {
  return line - 1;
};


/**
 * Converts line number from v8(0-based) to Web Inspector UI(1-based).
 * @param {number} line Resource line number in v8.
 * @return {number} The line number in Web Inspector.
 */
devtools.DebuggerAgent.v8ToWwebkitLineNumber_ = function(line) {
  return line + 1;
};


/**
 * @param {number} scriptId Id of the script.
 * @param {number} lineOffset First line 0-based offset in the containing
 *     document.
 * @constructor
 */
devtools.ScriptInfo = function(scriptId, lineOffset) {
  this.scriptId_ = scriptId;
  this.lineOffset_ = lineOffset;
  
  this.lineToBreakpointInfo_ = {};
};


/**
 * @return {number}
 */
devtools.ScriptInfo.prototype.getLineOffset = function() {
  return this.lineOffset_;
};


/**
 * @param {number} line 0-based line number in the script.
 * @return {?devtools.BreakpointInfo} Information on a breakpoint at the
 *     specified line in the script or undefined if there is no breakpoint at
 *     that line.
 */
devtools.ScriptInfo.prototype.getBreakpointInfo = function(line) {
  return this.lineToBreakpointInfo_[line];
};


/**
 * Adds breakpoint info to the script.
 * @param {devtools.BreakpointInfo} breakpoint
 */
devtools.ScriptInfo.prototype.addBreakpointInfo = function(breakpoint) {
  this.lineToBreakpointInfo_[breakpoint.getLine()] = breakpoint;
};


/**
 * @param {devtools.BreakpointInfo} breakpoint Breakpoint info to be removed.
 */
devtools.ScriptInfo.prototype.removeBreakpointInfo = function(breakpoint) {
  var line = breakpoint.getLine();
  delete this.lineToBreakpointInfo_[line];
};



/**
 * @param {number} scriptId Id of the owning script.
 * @param {number} line Breakpoint 0-based line number in the containing script.
 * @constructor
 */
devtools.BreakpointInfo = function(sourceId, line) {
  this.sourceId_ = sourceId;
  this.line_ = line; 
  this.v8id_ = -1;
  this.removed_ = false;
};


/**
 * @return {number}
 */
devtools.BreakpointInfo.prototype.getSourceId = function(n) {
  return this.sourceId_;
};


/**
 * @return {number}
 */
devtools.BreakpointInfo.prototype.getLine = function(n) {
  return this.line_;
};


/**
 * @return {number} Unique identifier of this breakpoint in the v8 debugger.
 */
devtools.BreakpointInfo.prototype.getV8Id = function(n) {
  return this.v8id_;
};


/**
 * Sets id of this breakpoint in the v8 debugger.
 * @param {number} id
 */
devtools.BreakpointInfo.prototype.setV8Id = function(id) {
  this.v8id_ = id;
};


/**
 * Marks this breakpoint as removed from the  front-end.
 */
devtools.BreakpointInfo.prototype.markAsRemoved = function() {
  this.removed_ = true;
};


/**
 * @return {boolean} Whether this breakpoint has been removed from the
 *     front-end.
 */
devtools.BreakpointInfo.prototype.isRemoved = function() {
  return this.removed_;
};


/**
 * Call stack frame data.
 * @construnctor
 */
devtools.CallFrame = function() {
  this.sourceID = null;
  this.line = null;
  this.type = 'function';
  this.functionName = null;
  this.caller = null;
  this.localScope = null;
  this.scopeChain = [];
  this.thisObject = {};
  this.frameNumber = null;
};


/**
 * This method is called by
 * WebInspector.ScriptsPanel.evaluateInSelectedCallFrame. This method issues
 * asynchronous evaluate request.
 * @param {string} expression An expression to be evaluated in the context of
 *     this call frame.
 * @return {string} User message that the expression is being evaluated.
 */
devtools.CallFrame.prototype.evaluate = function(expression) {
  devtools.tools.getDebuggerAgent().requestEvaluate({
        'expression': expression,
        'frame': this.frameNumber,
        'global': false,
        'disable_break': false
      },
      devtools.CallFrame.handleEvaluateResponse_);
  return 'evaluating...';
};


/**
 * Handles 'evaluate' response for a call frame
 * @param {devtools.DebuggerMessage} response
 */
devtools.CallFrame.handleEvaluateResponse_ = function(response) {
  var body = response.getBody();
  var value = devtools.DebuggerAgent.formatObjectReference_(body);
  WebInspector.addMessageToConsole(new WebInspector.ConsoleCommandResult(
      value, false /* exception */, null /* commandMessage */));
};


/**
 * JSON based commands sent to v8 debugger.
 * @param {string} command Name of the command to execute.
 * @param {Object} opt_arguments Command-specific arguments map.
 * @constructor
 */
devtools.DebugCommand = function(command, opt_arguments) {
  this.command_ = command;
  this.type_ = 'request';	
  this.seq_ = ++devtools.DebugCommand.nextSeq_;
  if (opt_arguments) {
    this.arguments_ = opt_arguments;
  }
};


/**
 * Next unique number to be used as debugger request sequence number.
 * @type {number}
 */
devtools.DebugCommand.nextSeq_ = 1;


/**
 * @return {number}
 */
devtools.DebugCommand.prototype.getSequenceNumber = function() {
  return this.seq_;
};


/**
 * @return {string}
 */
devtools.DebugCommand.prototype.toJSONProtocol = function() {
  var json = {
    'seq': this.seq_,
    'type': this.type_,
    'command': this.command_
  }
  if (this.arguments_) {
    json.arguments = this.arguments_;
  }
  return JSON.stringify(json);
};


/**
 * JSON messages sent from v8 debugger. See protocol definition for more
 * details: http://code.google.com/p/v8/wiki/DebuggerProtocol
 * @param {string} msg Raw protocol packet as JSON string.
 * @constructor
 */
devtools.DebuggerMessage = function(msg) {
  var jsExpression = '[' + msg + '][0]';
  this.packet_ = eval(jsExpression);
  this.refs_ = [];
  if (this.packet_.refs) {
    for (var i = 0; i < this.packet_.refs.length; i++) {
      this.refs_[this.packet_.refs[i].handle] = this.packet_.refs[i];
    }
  }
};


/**
 * @return {string} The packet type.
 */
devtools.DebuggerMessage.prototype.getType = function() {
  return this.packet_.type;
};


/**
 * @return {?string} The packet event if the message is an event.
 */
devtools.DebuggerMessage.prototype.getEvent = function() {
  return this.packet_.event;
};


/**
 * @return {?string} The packet command if the message is a response to a
 *     command.
 */
devtools.DebuggerMessage.prototype.getCommand = function() {
  return this.packet_.command;
};


/**
 * @return {number} The packet request sequence.
 */
devtools.DebuggerMessage.prototype.getRequestSeq = function() {
  return this.packet_.request_seq;
};


/**
 * @return {number} Whether the v8 is running after processing the request.
 */
devtools.DebuggerMessage.prototype.isRunning = function() {
  return this.packet_.running ? true : false;
};


/**
 * @return {boolean} Whether the request succeeded.
 */
devtools.DebuggerMessage.prototype.isSuccess = function() {
  return this.packet_.success ? true : false;
};


/**
 * @return {string}
 */
devtools.DebuggerMessage.prototype.getMessage = function() {
  return this.packet_.message;
};


/**
 * @return {Object} Parsed message body json.
 */
devtools.DebuggerMessage.prototype.getBody = function() {
  return this.packet_.body;
};


/**
 * @param {number} handle Object handle.
 * @return {?Object} Returns the object with the handle if it was sent in this
 *    message(some objects referenced by handles may be missing in the message).
 */
devtools.DebuggerMessage.prototype.lookup = function(handle) {
  return this.refs_[handle];
};
