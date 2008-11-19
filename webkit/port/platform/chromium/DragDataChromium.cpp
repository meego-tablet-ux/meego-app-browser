/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

// Modified from Apple's version to not directly call any windows methods as
// they may not be available to us in the multiprocess 

#include "config.h"
#include "DragData.h"

#include "ChromiumDataObject.h"
#include "Clipboard.h"
#include "ClipboardChromium.h"
#include "DocumentFragment.h"
#include "KURL.h"
#include "PlatformString.h"
#include "markup.h"

namespace {

bool containsHTML(const WebCore::ChromiumDataObject* drop_data) {
    return drop_data->text_html.length() > 0;
}

}

namespace WebCore {

PassRefPtr<Clipboard> DragData::createClipboard(ClipboardAccessPolicy policy) const
{
    RefPtr<ClipboardChromium> clipboard = ClipboardChromium::create(true,
        m_platformDragData, policy);

    return clipboard.release();
}

bool DragData::containsURL() const
{
    return m_platformDragData->url.isValid();
}

String DragData::asURL(String* title) const
{
    if (!m_platformDragData->url.isValid())
        return String();
 
    // |title| can be NULL
    if (title)
        *title = m_platformDragData->url_title;
    return m_platformDragData->url.string();
}

bool DragData::containsFiles() const
{
    return !m_platformDragData->filenames.isEmpty();
}

void DragData::asFilenames(Vector<String>& result) const
{
    for (size_t i = 0; i < m_platformDragData->filenames.size(); ++i)
        result.append(m_platformDragData->filenames[i]);
}

bool DragData::containsPlainText() const
{
    return !m_platformDragData->plain_text.isEmpty();
}

String DragData::asPlainText() const
{
    return m_platformDragData->plain_text;
}

bool DragData::containsColor() const
{
    return false;
}

bool DragData::canSmartReplace() const
{
    // Mimic the situations in which mac allows drag&drop to do a smart replace.
    // This is allowed whenever the drag data contains a 'range' (ie.,
    // ClipboardWin::writeRange is called).  For example, dragging a link
    // should not result in a space being added.
    return !m_platformDragData->plain_text.isEmpty() &&
           !m_platformDragData->url.isValid();
}

bool DragData::containsCompatibleContent() const
{
    return containsPlainText() || containsURL()
        || ::containsHTML(m_platformDragData)
        || containsColor();
}

PassRefPtr<DocumentFragment> DragData::asFragment(Document* doc) const
{     
    /*
     * Order is richest format first. On OSX this is:
     * * Web Archive
     * * Filenames
     * * HTML
     * * RTF
     * * TIFF
     * * PICT
     */

    if (containsFiles()) {
        // TODO(tc): Implement this.  Should be pretty simple to make some HTML
        // and call createFragmentFromMarkup.
        //if (RefPtr<DocumentFragment> fragment = createFragmentFromMarkup(doc,
        //    ?, KURL()))
        //    return fragment;
    }

    if (!m_platformDragData->text_html.isEmpty()) {
        RefPtr<DocumentFragment> fragment = createFragmentFromMarkup(doc,
            m_platformDragData->text_html, m_platformDragData->html_base_url);
        return fragment.release();
    }

    return 0;
}

Color DragData::asColor() const
{
    return Color();
}

}
