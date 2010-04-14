var assertEq = chrome.test.assertEq;

function pageUrl(letter) {
  return chrome.extension.getURL(letter + ".html");
}

function onCompleteGetCurrentTab(tab) {
  assertEq(tab.url, pageUrl("a"));
  chrome.tabs.remove(tab.id, function() {
    chrome.test.runNextTest();
  });
}

chrome.test.runTests([
  function backgroundPageGetCurrentTab() {
    chrome.tabs.getCurrent(function(tab) {
      // There should be no tab.
      assertEq(tab, undefined);
      chrome.test.runNextTest();
    });
  },

  function openedTabGetCurrentTab() {
    chrome.tabs.create({url: pageUrl("a")});
    // Completes with onCompleteGetCurrentTab.
  }
]);
