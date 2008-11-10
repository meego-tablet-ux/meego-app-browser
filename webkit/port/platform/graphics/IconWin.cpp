/*
* Copyright (C) 2006, 2007 Apple Inc.
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Library General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Library General Public License for more details.
*
* You should have received a copy of the GNU Library General Public License
* along with this library; see the file COPYING.LIB.  If not, write to
* the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
*
*/

#include "config.h"

#include <windows.h>
#include <shellapi.h>

#include "Icon.h"

#include "GraphicsContext.h"
#include "PlatformString.h"
#include "SkiaUtils.h"

namespace WebCore {

Icon::Icon(const PlatformIcon& icon)
    : m_icon(icon)
{
}

Icon::~Icon()
{
    if (m_icon)
        DestroyIcon(m_icon);
}

PassRefPtr<Icon> Icon::createIconForFile(const String& filename)
{
    SHFILEINFO sfi;
    memset(&sfi, 0, sizeof(sfi));

    String tmpFilename = filename;
    if (!SHGetFileInfo(tmpFilename.charactersWithNullTermination(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_SMALLICON))
        return 0;

    return adoptRef(new Icon(sfi.hIcon));
}

PassRefPtr<Icon> Icon::createIconForFiles(const Vector<String>& filenames)
{
    // TODO: support multiple files.
    // http://code.google.com/p/chromium/issues/detail?id=4092
    if (!filenames.size())
        return 0;

    return createIconForFile(filenames[0]);
}

void Icon::paint(GraphicsContext* context, const IntRect& rect)
{
    if (context->paintingDisabled())
        return;

    HDC hdc = context->platformContext()->canvas()->beginPlatformPaint();
    DrawIconEx(hdc, rect.x(), rect.y(), m_icon, rect.width(), rect.height(),
               0, 0, DI_NORMAL);
    context->platformContext()->canvas()->endPlatformPaint();
}

} // namespace WebCore
