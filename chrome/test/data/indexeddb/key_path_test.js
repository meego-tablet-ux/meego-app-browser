// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


function cursorSuccess()
{
    debug("Cursor opened successfully.")
    // FIXME: check that we can iterate the cursor.
    shouldBe("event.target.result.direction", "0");
    shouldBe("event.target.result.key", "'myKey' + count");
    shouldBe("event.target.result.value.keyPath", "'myKey' + count");
    shouldBe("event.target.result.value.value", "'myValue' + count");
    if (++count >= 5)
        done();
    else
        openCursor();
}

function openCursor()
{
    debug("Opening cursor #" + count);
    keyRange = webkitIDBKeyRange.lowerBound("myKey" + count);
    request = objectStore.openCursor(keyRange);
    request.onsuccess = cursorSuccess;
    request.onerror = unexpectedErrorCallback;
}

function populateObjectStore()
{
    debug("Populating object store #" + count);
    obj = {'keyPath': 'myKey' + count, 'value': 'myValue' + count};
    request = objectStore.add(obj);
    request.onerror = unexpectedErrorCallback;
    if (++count >= 5) {
        count = 0;
        request.onsuccess = openCursor;
    } else {
        request.onsuccess = populateObjectStore;
    }
}

function createObjectStore()
{
    debug('createObjectStore');
    deleteAllObjectStores(db);
    window.objectStore = db.createObjectStore('test', {keyPath: 'keyPath'});
    count = 0;
    populateObjectStore();
}

function setVersion()
{
    debug('setVersion');
    window.db = event.target.result;
    var request = db.setVersion('new version');
    request.onsuccess = createObjectStore;
    request.onerror = unexpectedErrorCallback;
}

function test()
{
    debug("Test IndexedDB's KeyPath.");
    debug("Opening IndexedDB");
    request = webkitIndexedDB.open('name');
    request.onsuccess = setVersion;
    request.onerror = unexpectedErrorCallback;
}

var successfullyParsed = true;
