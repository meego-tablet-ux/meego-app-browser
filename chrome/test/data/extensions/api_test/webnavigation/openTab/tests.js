// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function runTests() {
  var getURL = chrome.extension.getURL;
  chrome.tabs.getSelected(null, function(tab) {
    var tabId = tab.id;

    chrome.test.runTests([
      // Opens a new tab from javascript.
      function openTab() {
        expect([
          [ "onBeforeNavigate",
            { frameId: 0,
              requestId: "0",
              tabId: 0,
              timeStamp: 0,
              url: getURL('a.html') }],
          [ "onCommitted",
            { frameId: 0,
              tabId: 0,
              timeStamp: 0,
              transitionQualifiers: [],
              transitionType: "link",
              url: getURL('a.html') }],
          [ "onDOMContentLoaded",
            { frameId: 0,
              tabId: 0,
              timeStamp: 0,
              url: getURL('a.html') }],
          [ "onCompleted",
            { frameId: 0,
              tabId: 0,
              timeStamp: 0,
              url: getURL('a.html') }],
          [ "onBeforeRetarget",
            { sourceTabId: 0,
              sourceUrl: getURL('a.html'),
              timeStamp: 0,
              url: getURL('b.html') }],
          [ "onBeforeNavigate",
            { frameId: 0,
              requestId: "0",
              tabId: 1,
              timeStamp: 0,
              url: getURL('b.html') }],
          [ "onCommitted",
            { frameId: 0,
              tabId: 1,
              timeStamp: 0,
              transitionQualifiers: [],
              transitionType: "link",
              url: getURL('b.html') }],
          [ "onDOMContentLoaded",
            { frameId: 0,
              tabId: 1,
              timeStamp: 0,
              url: getURL('b.html') }],
          [ "onCompleted",
            { frameId: 0,
              tabId: 1,
              timeStamp: 0,
              url: getURL('b.html') }]]);
        chrome.tabs.update(tabId, { url: getURL('a.html') });
      },

      // Opens a new tab from javascript within an iframe.
      function openTabFrame() {
        expect([
          [ "onBeforeNavigate",
            { frameId: 0,
              requestId: "0",
              tabId: 0,
              timeStamp: 0,
              url: getURL('c.html') }],
          [ "onCommitted",
            { frameId: 0,
              tabId: 0,
              timeStamp: 0,
              transitionQualifiers: [],
              transitionType: "link",
              url: getURL('c.html') }],
          [ "onBeforeNavigate",
            { frameId: 1,
              requestId: "0",
              tabId: 0,
              timeStamp: 0,
              url: getURL('a.html') }],
          [ "onDOMContentLoaded",
            { frameId: 0,
              tabId: 0,
              timeStamp: 0,
              url: getURL('c.html') }],
          [ "onCommitted",
            { frameId: 1,
              tabId: 0,
              timeStamp: 0,
              transitionQualifiers: [],
              transitionType: "auto_subframe",
              url: getURL('a.html') }],
          [ "onDOMContentLoaded",
            { frameId: 1,
              tabId: 0,
              timeStamp: 0,
              url: getURL('a.html') }],
          [ "onCompleted",
            { frameId: 1,
              tabId: 0,
              timeStamp: 0,
              url: getURL('a.html') }],
          [ "onCompleted",
            { frameId: 0,
              tabId: 0,
              timeStamp: 0,
              url: getURL('c.html') }],
          [ "onBeforeRetarget",
            { sourceTabId: 0,
              sourceUrl: getURL('a.html'),
              timeStamp: 0,
              url: getURL('b.html') }],
          [ "onBeforeNavigate",
            { frameId: 0,
              requestId: "0",
              tabId: 1,
              timeStamp: 0,
              url: getURL('b.html') }],
          [ "onCommitted",
            { frameId: 0,
              tabId: 1,
              timeStamp: 0,
              transitionQualifiers: [],
              transitionType: "link",
              url: getURL('b.html') }],
          [ "onDOMContentLoaded",
            { frameId: 0,
              tabId: 1,
              timeStamp: 0,
              url: getURL('b.html') }],
          [ "onCompleted",
            { frameId: 0,
              tabId: 1,
              timeStamp: 0,
              url: getURL('b.html') }]]);
        chrome.tabs.update(tabId, { url: getURL('c.html') });
      },
    ]);
  });
}
