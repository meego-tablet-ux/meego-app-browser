// Copyright (c) 2008, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "config.h"

#pragma warning(push, 0)
#include "PopupMenu.h"

#include "CharacterNames.h"
#include "ChromeClientChromium.h"
#include "Document.h"
#include "Font.h"
#include "FrameView.h"
#include "FontSelector.h"
#include "Frame.h"
#include "FramelessScrollView.h"
#include "FramelessScrollViewClient.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "KeyboardCodes.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformScreen.h"
#include "PlatformWheelEvent.h"
#include "RenderBlock.h"
#include "RenderTheme.h"
#include "ScrollbarTheme.h"
#include "SystemTime.h"
#include "Widget.h"
#pragma warning(pop)

#include "webkit/port/platform/chromium/PopupMenuChromium.h"

using namespace WTF;
using namespace Unicode;

using std::min;
using std::max;

namespace WebCore {

typedef unsigned long long TimeStamp;

static const int kMaxVisibleRows = 20;
static const int kMaxHeight = 500;
static const int kBorderSize = 1;
static const TimeStamp kTypeAheadTimeoutMs = 1000;

// This class uses WebCore code to paint and handle events for a drop-down list
// box ("combobox" on Windows).
class PopupListBox : public FramelessScrollView, public RefCounted<PopupListBox> {
public:
    // FramelessScrollView
    virtual void paint(GraphicsContext* gc, const IntRect& rect);
    virtual bool handleMouseDownEvent(const PlatformMouseEvent& event);
    virtual bool handleMouseMoveEvent(const PlatformMouseEvent& event);
    virtual bool handleMouseReleaseEvent(const PlatformMouseEvent& event);
    virtual bool handleWheelEvent(const PlatformWheelEvent& event);
    virtual bool handleKeyEvent(const PlatformKeyboardEvent& event);

    // ScrollView
    virtual HostWindow* hostWindow() const;

    // PopupListBox methods

    // Show the popup
    void showPopup();

    // Hide the popup.  Do not call this directly: use client->hidePopup().
    void hidePopup();

    // Update our internal list to match the client.
    void updateFromElement();

    // Free any allocated resources used in a particular popup session. 
    void clear();

    // Set the index of the option that is displayed in the <select> widget in the page
    void setOriginalIndex(int index);

    // Get the index of the item that the user is currently moused over or has selected with
    // the keyboard. This is not the same as the original index, since the user has not yet
    // accepted this input.
    int selectedIndex() const { return m_selectedIndex; }

    // Move selection down/up the given number of items, scrolling if necessary.
    // Positive is down.  The resulting index will be clamped to the range
    // [0, numItems), and non-option items will be skipped.
    void adjustSelectedIndex(int delta);

    // Returns the number of items in the list.
    int numItems() const { return static_cast<int>(m_items.size()); }

    void setBaseWidth(int width)
    {
        m_baseWidth = width;
    }

    // Compute size of widget and children.
    void layout();

    // Returns whether the popup wants to process events for the passed key.
    bool isInterestedInEventForKey(int key_code);

    // Sets whether the PopupMenuClient should be told to change its text when a
    // new item is selected (by using the arrow keys).  Default is true.
    void setTextOnIndexChange(bool value) { m_setTextOnIndexChange = value; }

private:
    friend class PopupContainer;
    friend class RefCounted<PopupListBox>;

    // A type of List Item
    enum ListItemType {
        TYPE_OPTION,
        TYPE_GROUP,
        TYPE_SEPARATOR
    };

    // A item (represented by <option> or <optgroup>) in the <select> widget. 
    struct ListItem {
        ListItem(const String& label, ListItemType type)
            : label(label.copy()), type(type), y(0) {}
        String label;
        ListItemType type;
        int y;  // y offset of this item, relative to the top of the popup.
    };

    PopupListBox(PopupMenuClient* client)
        : m_originalIndex(0)
        , m_selectedIndex(0)
        , m_acceptOnAbandon(false)
        , m_visibleRows(0)
        , m_popupClient(client)
        , m_repeatingChar(0)
        , m_lastCharTime(0)
        , m_setTextOnIndexChange(true)
    {
        setScrollbarModes(ScrollbarAlwaysOff, ScrollbarAlwaysOff);
    }

    ~PopupListBox()
    {
        clear();
    }

    void disconnectClient() { m_popupClient = 0; }

    // Closes the popup
    void abandon();
    // Select an index in the list, scrolling if necessary.
    void selectIndex(int index);
    // Accepts the selected index as the value to be displayed in the <select> widget on
    // the web page, and closes the popup.
    void acceptIndex(int index);

    // Returns true if the selection can be changed to index.
    // Disabled items, or labels cannot be selected.
    bool isSelectableItem(int index);

    // Scrolls to reveal the given index.
    void scrollToRevealRow(int index);
    void scrollToRevealSelection() { scrollToRevealRow(m_selectedIndex); }

    // Invalidates the row at the given index. 
    void invalidateRow(int index);

    // Gets the height of a row.
    int getRowHeight(int index);
    // Get the bounds of a row. 
    IntRect getRowBounds(int index);

    // Converts a point to an index of the row the point is over
    int pointToRowIndex(const IntPoint& point);

    // Paint an individual row
    void paintRow(GraphicsContext* gc, const IntRect& rect, int rowIndex);

    // Test if the given point is within the bounds of the popup window.
    bool isPointInBounds(const IntPoint& point);

    // Called when the user presses a text key.  Does a prefix-search of the items.
    void typeAheadFind(const PlatformKeyboardEvent& event);

    // Returns the font to use for the given row
    Font getRowFont(int index);

    // This is the index of the item marked as "selected" - i.e. displayed in the widget on the
    // page. 
    int m_originalIndex;

    // This is the index of the item that the user is hovered over or has selected using the 
    // keyboard in the list. They have not confirmed this selection by clicking or pressing 
    // enter yet however.
    int m_selectedIndex;

    // True if we should accept the selectedIndex as chosen, even if the popup
    // is "abandoned".  This is used for keyboard navigation, where we want the
    // selection to change immediately.
    bool m_acceptOnAbandon;

    // This is the number of rows visible in the popup. The maximum number visible at a time is
    // defined as being kMaxVisibleRows. For a scrolled popup, this can be thought of as the
    // page size in data units. 
    int m_visibleRows;

    // Our suggested width, not including scrollbar.
    int m_baseWidth;

    // A list of the options contained within the <select>
    Vector<ListItem*> m_items;

    // The <select> PopupMenuClient that opened us.
    PopupMenuClient* m_popupClient;

    // The scrollbar which has mouse capture.  Mouse events go straight to this
    // if non-NULL.
    RefPtr<Scrollbar> m_capturingScrollbar;

    // The last scrollbar that the mouse was over.  Used for mouseover highlights.
    RefPtr<Scrollbar> m_lastScrollbarUnderMouse;

    // The string the user has typed so far into the popup. Used for typeAheadFind.
    String m_typedString;

    // The char the user has hit repeatedly.  Used for typeAheadFind.
    UChar m_repeatingChar;

    // The last time the user hit a key.  Used for typeAheadFind.
    TimeStamp m_lastCharTime;

    bool m_setTextOnIndexChange;
};

static PlatformMouseEvent constructRelativeMouseEvent(const PlatformMouseEvent& e,
                                                      FramelessScrollView* parent,
                                                      FramelessScrollView* child)
{
    IntPoint pos = parent->convertSelfToChild(child, e.pos());

    // FIXME(beng): This is a horrible hack since PlatformMouseEvent has no setters for x/y.
    //              Need to add setters and get patch back upstream to webkit source. 
    PlatformMouseEvent relativeEvent = e;
    IntPoint& relativePos = const_cast<IntPoint&>(relativeEvent.pos());
    relativePos.setX(pos.x());
    relativePos.setY(pos.y());
    return relativeEvent;
}

static PlatformWheelEvent constructRelativeWheelEvent(const PlatformWheelEvent& e,
                                                      FramelessScrollView* parent,
                                                      FramelessScrollView* child)
{
    IntPoint pos = parent->convertSelfToChild(child, e.pos());

    // FIXME(beng): This is a horrible hack since PlatformWheelEvent has no setters for x/y.
    //              Need to add setters and get patch back upstream to webkit source. 
    PlatformWheelEvent relativeEvent = e;
    IntPoint& relativePos = const_cast<IntPoint&>(relativeEvent.pos());
    relativePos.setX(pos.x());
    relativePos.setY(pos.y());
    return relativeEvent;
}

///////////////////////////////////////////////////////////////////////////////
// PopupContainer implementation

// static
PassRefPtr<PopupContainer> PopupContainer::create(PopupMenuClient* client,
                                                  bool focusOnShow)
{
    return adoptRef(new PopupContainer(client, focusOnShow));
}

PopupContainer::PopupContainer(PopupMenuClient* client, bool focusOnShow)
    : m_listBox(new PopupListBox(client)),
      m_focusOnShow(focusOnShow)
{
    // FrameViews are created with a refcount of 1 so it needs releasing after we
    // assign it to a RefPtr.
    m_listBox->deref();

    setScrollbarModes(ScrollbarAlwaysOff, ScrollbarAlwaysOff);
}

PopupContainer::~PopupContainer()
{
    if (m_listBox)
        removeChild(m_listBox.get());
}

void PopupContainer::showPopup(FrameView* view)
{
    // Pre-layout, our size matches the <select> dropdown control.
    int selectHeight = frameRect().height();

    // Lay everything out to figure out our preferred size, then tell the view's
    // WidgetClient about it.  It should assign us a client.
    layout();

    ChromeClientChromium* chromeClient = static_cast<ChromeClientChromium*>(
        view->frame()->page()->chrome()->client());
    if (chromeClient) {
        // If the popup would extend past the bottom of the screen, open upwards
        // instead.
        FloatRect screen = screenRect(view);
        IntRect widgetRect = chromeClient->windowToScreen(frameRect());
        if (widgetRect.bottom() > static_cast<int>(screen.bottom()))
            widgetRect.move(0, -(widgetRect.height() + selectHeight));

        chromeClient->popupOpened(this, widgetRect, m_focusOnShow);
    }

    // Must get called after we have a client and containingWindow.
    addChild(m_listBox.get());

    // Enable scrollbars after the listbox is inserted into the hierarchy, so
    // it has a proper WidgetClient.
    m_listBox->setVerticalScrollbarMode(ScrollbarAuto);

    m_listBox->scrollToRevealSelection();

    invalidate();
}

void PopupContainer::hidePopup()
{
    invalidate();

    m_listBox->disconnectClient();
    removeChild(m_listBox.get());
    m_listBox = 0;
    
    if (client())
        client()->popupClosed(this);
}

void PopupContainer::layout()
{
    m_listBox->layout();

    // Place the listbox within our border.
    m_listBox->move(kBorderSize, kBorderSize);

    // Size ourselves to contain listbox + border.
    resize(m_listBox->width() + kBorderSize*2, m_listBox->height() + kBorderSize*2);

    invalidate();
}

bool PopupContainer::handleMouseDownEvent(const PlatformMouseEvent& event)
{
    return m_listBox->handleMouseDownEvent(
        constructRelativeMouseEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleMouseMoveEvent(const PlatformMouseEvent& event)
{
    return m_listBox->handleMouseMoveEvent(
        constructRelativeMouseEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleMouseReleaseEvent(const PlatformMouseEvent& event)
{
    return m_listBox->handleMouseReleaseEvent(
        constructRelativeMouseEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleWheelEvent(const PlatformWheelEvent& event)
{
    return m_listBox->handleWheelEvent(
        constructRelativeWheelEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleKeyEvent(const PlatformKeyboardEvent& event)
{
    return m_listBox->handleKeyEvent(event);
}

void PopupContainer::hide() {
    m_listBox->abandon();
}

void PopupContainer::paint(GraphicsContext* gc, const IntRect& rect)
{
    // adjust coords for scrolled frame
    IntRect r = intersection(rect, frameRect());
    int tx = x();
    int ty = y();

    r.move(-tx, -ty);

    gc->translate(static_cast<float>(tx), static_cast<float>(ty));
    m_listBox->paint(gc, r);
    gc->translate(-static_cast<float>(tx), -static_cast<float>(ty));

    paintBorder(gc, rect);
}

void PopupContainer::paintBorder(GraphicsContext* gc, const IntRect& rect)
{
    // FIXME(mpcomplete): where do we get the border color from?
    Color borderColor(127, 157, 185);

    gc->setStrokeStyle(NoStroke);
    gc->setFillColor(borderColor);

    int tx = x();
    int ty = y();

    // top, left, bottom, right
    gc->drawRect(IntRect(tx, ty, width(), kBorderSize));
    gc->drawRect(IntRect(tx, ty, kBorderSize, height()));
    gc->drawRect(IntRect(tx, ty + height() - kBorderSize, width(), kBorderSize));
    gc->drawRect(IntRect(tx + width() - kBorderSize, ty, kBorderSize, height()));
}

bool PopupContainer::isInterestedInEventForKey(int key_code) {
    return m_listBox->isInterestedInEventForKey(key_code);
}

void PopupContainer::show(const IntRect& r, FrameView* v, int index) {
    // The rect is the size of the select box. It's usually larger than we need.
    // subtract border size so that usually the container will be displayed 
    // exactly the same width as the select box.
    listBox()->setBaseWidth(max(r.width() - kBorderSize * 2, 0));

    listBox()->updateFromElement();

    // We set the selected item in updateFromElement(), and disregard the
    // index passed into this function (same as Webkit's PopupMenuWin.cpp)
    // TODO(ericroman): make sure this is correct, and add an assertion.
    // DCHECK(popupWindow(popup)->listBox()->selectedIndex() == index);

    // Convert point to main window coords.
    IntPoint location = v->contentsToWindow(r.location());

    // Move it below the select widget.
    location.move(0, r.height());

    IntRect popupRect(location, r.size());
    setFrameRect(popupRect);
    showPopup(v);
}

void PopupContainer::setTextOnIndexChange(bool value) {
  listBox()->setTextOnIndexChange(value);
}

///////////////////////////////////////////////////////////////////////////////
// PopupListBox implementation

bool PopupListBox::handleMouseDownEvent(const PlatformMouseEvent& event)
{
    Scrollbar* scrollbar = scrollbarUnderMouse(event);
    if (scrollbar) {
        m_capturingScrollbar = scrollbar;
        m_capturingScrollbar->mouseDown(event);
        return true;
    }

    if (!isPointInBounds(event.pos()))
        abandon();

    return true;
}

bool PopupListBox::handleMouseMoveEvent(const PlatformMouseEvent& event)
{
    if (m_capturingScrollbar) {
        m_capturingScrollbar->mouseMoved(event);
        return true;
    }

    Scrollbar* scrollbar = scrollbarUnderMouse(event);
    if (m_lastScrollbarUnderMouse != scrollbar) {
        // Send mouse exited to the old scrollbar.
        if (m_lastScrollbarUnderMouse)
            m_lastScrollbarUnderMouse->mouseExited();
        m_lastScrollbarUnderMouse = scrollbar;
    }

    if (scrollbar) {
        scrollbar->mouseMoved(event);
        return true;
    }

    if (!isPointInBounds(event.pos()))
        return false;

    selectIndex(pointToRowIndex(event.pos()));
    return true;
}

bool PopupListBox::handleMouseReleaseEvent(const PlatformMouseEvent& event)
{
    if (m_capturingScrollbar) {
        m_capturingScrollbar->mouseUp();
        m_capturingScrollbar = 0;
        return true;
    }

    if (!isPointInBounds(event.pos()))
        return true;

    acceptIndex(pointToRowIndex(event.pos()));
    return true;
}

bool PopupListBox::handleWheelEvent(const PlatformWheelEvent& event)
{
    if (!isPointInBounds(event.pos())) {
        abandon();
        return true;
    }

    // Pass it off to the scroll view.
    // Sadly, WebCore devs don't understand the whole "const" thing.
    wheelEvent(const_cast<PlatformWheelEvent&>(event));
    return true;
}

// Should be kept in sync with handleKeyEvent().
bool PopupListBox::isInterestedInEventForKey(int key_code) {
  switch (key_code) {
    case VKEY_ESCAPE:
    case VKEY_RETURN:
    case VKEY_UP:
    case VKEY_DOWN:
    case VKEY_PRIOR:
    case VKEY_NEXT:
    case VKEY_HOME:
    case VKEY_END:
      return true;
    default:
      return false;
  }
}

bool PopupListBox::handleKeyEvent(const PlatformKeyboardEvent& event)
{
    if (event.type() == PlatformKeyboardEvent::KeyUp)
        return true;

    if (numItems() == 0 && event.windowsVirtualKeyCode() != VKEY_ESCAPE)
        return true;

    switch (event.windowsVirtualKeyCode()) {
    case VKEY_ESCAPE:
        abandon();  // may delete this
        return true;
    case VKEY_RETURN:
        acceptIndex(m_selectedIndex);  // may delete this
        return true;
    case VKEY_UP:
        adjustSelectedIndex(-1);
        break;
    case VKEY_DOWN:
        adjustSelectedIndex(1);
        break;
    case VKEY_PRIOR:
        adjustSelectedIndex(-m_visibleRows);
        break;
    case VKEY_NEXT:
        adjustSelectedIndex(m_visibleRows);
        break;
    case VKEY_HOME:
        adjustSelectedIndex(-m_selectedIndex);
        break;
    case VKEY_END:
        adjustSelectedIndex(m_items.size());
        break;
    default:
        if (!event.ctrlKey() && !event.altKey() && !event.metaKey() &&
            isPrintableChar(event.windowsVirtualKeyCode())) {
            typeAheadFind(event);
        }
        break;
    }

    if (m_originalIndex != m_selectedIndex) {
        // Keyboard events should update the selection immediately (but we don't
        // want to fire the onchange event until the popup is closed, to match
        // IE).  We change the original index so we revert to that when the
        // popup is closed.
        m_acceptOnAbandon = true;
        setOriginalIndex(m_selectedIndex);
        if (m_setTextOnIndexChange)
          m_popupClient->setTextFromItem(m_selectedIndex);
    }

    return true;
}

HostWindow* PopupListBox::hostWindow() const
{
    // Our parent is the root ScrollView, so it is the one that has a
    // HostWindow.  FrameView::hostWindow() works similarly.
    return parent() ? parent()->hostWindow() : 0;
}

// From HTMLSelectElement.cpp
static String stripLeadingWhiteSpace(const String& string)
{
    int length = string.length();
    int i;
    for (i = 0; i < length; ++i)
        if (string[i] != noBreakSpace &&
            (string[i] <= 0x7F ? !isspace(string[i]) : (direction(string[i]) != WhiteSpaceNeutral)))
            break;

    return string.substring(i, length - i);
}

// From HTMLSelectElement.cpp, with modifications
void PopupListBox::typeAheadFind(const PlatformKeyboardEvent& event)
{
    TimeStamp now = static_cast<TimeStamp>(currentTime() * 1000.0f);
    TimeStamp delta = now - m_lastCharTime;

    m_lastCharTime = now;

    UChar c = event.windowsVirtualKeyCode();

    String prefix;
    int searchStartOffset = 1;
    if (delta > kTypeAheadTimeoutMs) {
        m_typedString = prefix = String(&c, 1);
        m_repeatingChar = c;
    } else {
        m_typedString.append(c);

        if (c == m_repeatingChar)
            // The user is likely trying to cycle through all the items starting with this character, so just search on the character
            prefix = String(&c, 1);
        else {
            m_repeatingChar = 0;
            prefix = m_typedString;
            searchStartOffset = 0;
        }
    }

    int itemCount = numItems();
    int index = (m_selectedIndex + searchStartOffset) % itemCount;
    for (int i = 0; i < itemCount; i++, index = (index + 1) % itemCount) {
        if (!isSelectableItem(index))
            continue;

        if (stripLeadingWhiteSpace(m_items[index]->label).startsWith(prefix, false)) {
            selectIndex(index);
            return;
        }
    }
}

void PopupListBox::paint(GraphicsContext* gc, const IntRect& rect)
{
    // adjust coords for scrolled frame
    IntRect r = intersection(rect, frameRect());
    int tx = x() - scrollX();
    int ty = y() - scrollY();

    r.move(-tx, -ty);

    // set clip rect to match revised damage rect
    gc->save();
    gc->translate(static_cast<float>(tx), static_cast<float>(ty));
    gc->clip(r);

    // TODO(mpcomplete): Can we optimize scrolling to not require repainting the
    // entire window?  Should we?
    for (int i = 0; i < numItems(); ++i)
        paintRow(gc, r, i);

    // Special case for an empty popup.
    if (numItems() == 0)
        gc->fillRect(r, Color::white);

    gc->restore();

    ScrollView::paint(gc, rect);
}

static const int separatorPadding = 4;
static const int separatorHeight = 1;

void PopupListBox::paintRow(GraphicsContext* gc, const IntRect& rect, int rowIndex)
{
    // This code is based largely on RenderListBox::paint* methods.

    IntRect rowRect = getRowBounds(rowIndex);
    if (!rowRect.intersects(rect))
        return;

    PopupMenuStyle style = m_popupClient->itemStyle(rowIndex);

    // Paint background
    Color backColor, textColor;
    if (rowIndex == m_selectedIndex) {
        backColor = theme()->activeListBoxSelectionBackgroundColor();
        textColor = theme()->activeListBoxSelectionForegroundColor();
    } else {
        backColor = style.backgroundColor();
        textColor = style.foregroundColor();
    }

    // If we have a transparent background, make sure it has a color to blend
    // against.
    if (backColor.hasAlpha())
        gc->fillRect(rowRect, Color::white);

    gc->fillRect(rowRect, backColor);
    
    if (m_popupClient->itemIsSeparator(rowIndex)) {
        IntRect separatorRect(
            rowRect.x() + separatorPadding,
            rowRect.y() + (rowRect.height() - separatorHeight) / 2,
            rowRect.width() - 2 * separatorPadding, separatorHeight);
        gc->fillRect(separatorRect, textColor);
        return;
    }
    
    gc->setFillColor(textColor);

    Font itemFont = getRowFont(rowIndex);
    gc->setFont(itemFont);

    // Bunch of shit to deal with RTL text...
    String itemText = m_popupClient->itemText(rowIndex);
    unsigned length = itemText.length();
    const UChar* str = itemText.characters();

    TextRun textRun(str, length, false, 0, 0, itemText.defaultWritingDirection() == WTF::Unicode::RightToLeft);

    // TODO(ojan): http://b/1210481 We should get the padding of individual
    // option elements.  This probably implies changes to PopupMenuClient.

    // Draw the item text
    if (style.isVisible()) {
        int textX = max(0, m_popupClient->clientPaddingLeft() - m_popupClient->clientInsetLeft());
        int textY = rowRect.y() + itemFont.ascent() + (rowRect.height() - itemFont.height()) / 2;
        gc->drawBidiText(textRun, IntPoint(textX, textY));
    }
}

Font PopupListBox::getRowFont(int rowIndex)
{
    Font itemFont = m_popupClient->itemStyle(rowIndex).font();
    if (m_popupClient->itemIsLabel(rowIndex)) {
        // Bold-ify labels (ie, an <optgroup> heading).
        FontDescription d = itemFont.fontDescription();
        d.setWeight(FontWeightBold);
        Font font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
        font.update(0);
        return font;
    }

    return itemFont;
}

void PopupListBox::abandon()
{
    RefPtr<PopupListBox> keepAlive(this);

    m_selectedIndex = m_originalIndex;

    if (m_acceptOnAbandon)
        m_popupClient->valueChanged(m_selectedIndex);

    // valueChanged may have torn down the popup!
    if (m_popupClient)
        m_popupClient->hidePopup();
}

int PopupListBox::pointToRowIndex(const IntPoint& point)
{
    int y = scrollY() + point.y();

    // TODO(mpcomplete): binary search if perf matters.
    for (int i = 0; i < numItems(); ++i) {
        if (y < m_items[i]->y)
            return i-1;
    }

    // Last item?
    if (y < contentsHeight())
        return m_items.size()-1;

    return -1;
}

void PopupListBox::acceptIndex(int index)
{
    ASSERT(index >= 0 && index < numItems());

    if (isSelectableItem(index)) {
        RefPtr<PopupListBox> keepAlive(this);

        // Tell the <select> PopupMenuClient what index was selected, and hide ourself.
        m_popupClient->valueChanged(index);

        // valueChanged may have torn down the popup!
        if (m_popupClient)
            m_popupClient->hidePopup();
    }
}

void PopupListBox::selectIndex(int index)
{
    ASSERT(index >= 0 && index < numItems());

    if (index != m_selectedIndex && isSelectableItem(index)) {
        invalidateRow(m_selectedIndex);
        m_selectedIndex = index;
        invalidateRow(m_selectedIndex);

        scrollToRevealSelection();
    }
}

void PopupListBox::setOriginalIndex(int index)
{
    m_originalIndex = m_selectedIndex = index;
}

int PopupListBox::getRowHeight(int index)
{
    return m_popupClient->itemStyle(index).font().height();
}

IntRect PopupListBox::getRowBounds(int index)
{
    if (index >= 0) {
        return IntRect(0, m_items[index]->y, visibleWidth(), getRowHeight(index));
    } else {
        return IntRect(0, 0, visibleWidth(), getRowHeight(index));
    }
}

void PopupListBox::invalidateRow(int index)
{
    if (index < 0)
        return;

    invalidateRect(getRowBounds(index));
}

void PopupListBox::scrollToRevealRow(int index)
{
    if (index < 0)
        return;

    IntRect rowRect = getRowBounds(index);
 
    if (rowRect.y() < scrollY()) {
        // Row is above current scroll position, scroll up.
        ScrollView::setScrollPosition(IntPoint(0, rowRect.y()));
    } else if (rowRect.bottom() > scrollY() + visibleHeight()) {
        // Row is below current scroll position, scroll down.
        ScrollView::setScrollPosition(IntPoint(0, rowRect.bottom() - visibleHeight()));
    }
}

bool PopupListBox::isSelectableItem(int index) {
    return m_items[index]->type == TYPE_OPTION &&
           m_popupClient->itemIsEnabled(index);
}

void PopupListBox::adjustSelectedIndex(int delta)
{
    int targetIndex = m_selectedIndex + delta;
    targetIndex = min(max(targetIndex, 0), numItems() - 1);
    if (!isSelectableItem(targetIndex)) {
        // We didn't land on an option.  Try to find one.
        // We try to select the closest index to target, prioritizing any in
        // the range [current, target].

        int dir = delta > 0 ? 1 : -1;
        int testIndex = m_selectedIndex;
        int bestIndex = m_selectedIndex;
        bool passedTarget = false;
        while (testIndex >= 0 && testIndex < numItems()) {
            if (isSelectableItem(testIndex))
                bestIndex = testIndex;
            if (testIndex == targetIndex)
                passedTarget = true;
            if (passedTarget && bestIndex != m_selectedIndex)
                break;

            testIndex += dir;
        }

        // Pick the best index, which may mean we don't change.
        targetIndex = bestIndex;
    }

    // Select the new index, and ensure its visible.  We do this regardless of
    // whether the selection changed to ensure keyboard events always bring the
    // selection into view.
    selectIndex(targetIndex);
    scrollToRevealSelection();
}

void PopupListBox::updateFromElement()
{
    // It happens when pressing a key to jump to an item, then use tab or
    // mouse to get away from the select box. In that case, updateFromElement
    // is called before abandon, which causes discarding of the select result.    
    if (m_acceptOnAbandon) {
        m_popupClient->valueChanged(m_selectedIndex);
        m_acceptOnAbandon = false;
    }

    clear();

    int size = m_popupClient->listSize();
    for (int i = 0; i < size; ++i) {
        ListItemType type;
        if (m_popupClient->itemIsSeparator(i))
            type = PopupListBox::TYPE_SEPARATOR;
        else if (m_popupClient->itemIsLabel(i))
            type = PopupListBox::TYPE_GROUP;
        else
            type = PopupListBox::TYPE_OPTION;
        m_items.append(new ListItem(m_popupClient->itemText(i), type));
    }

    m_selectedIndex = m_popupClient->selectedIndex();
    setOriginalIndex(m_selectedIndex);

    layout();
}

void PopupListBox::layout()
{
    // Size our child items.
    int baseWidth = 0;
    int paddingWidth = 0;
    int y = 0;
    for (int i = 0; i < numItems(); ++i) {
        Font itemFont = getRowFont(i);

        // Place the item vertically.
        m_items[i]->y = y;
        y += itemFont.height();

        // Ensure the popup is wide enough to fit this item.
        String text = m_popupClient->itemText(i);
        if (!text.isEmpty()) {
            int width = itemFont.width(TextRun(text));
            baseWidth = max(baseWidth, width);
        }
        // TODO(ojan): http://b/1210481 We should get the padding of individual option elements.
        paddingWidth = max(paddingWidth, 
            m_popupClient->clientPaddingLeft() + m_popupClient->clientPaddingRight());
    }

    int windowHeight = 0;
    m_visibleRows = min(numItems(), kMaxVisibleRows);
    for (int i = 0; i < m_visibleRows; ++i) {
        int rowHeight = getRowHeight(i);
        if (windowHeight + rowHeight > kMaxHeight) {
            m_visibleRows = i;
            break;
        }

        windowHeight += rowHeight;
    }

    if (windowHeight == 0)
        windowHeight = min(getRowHeight(-1), kMaxHeight);

    // Set our widget and scrollable contents sizes.
    int scrollbarWidth = 0;
    if (m_visibleRows < numItems())
        scrollbarWidth = ScrollbarTheme::nativeTheme()->scrollbarThickness();

    int windowWidth = baseWidth + scrollbarWidth + paddingWidth;
    int contentWidth = baseWidth;

    if (windowWidth < m_baseWidth) {
        windowWidth = m_baseWidth;
        contentWidth = m_baseWidth - scrollbarWidth - paddingWidth;
    } else {
        m_baseWidth = baseWidth;
    }

    resize(windowWidth, windowHeight);
    setContentsSize(IntSize(contentWidth, getRowBounds(numItems() - 1).bottom()));
    
    if (hostWindow())
        scrollToRevealSelection();

    invalidate();
}

void PopupListBox::clear()
{
    for (Vector<ListItem*>::iterator it = m_items.begin(); it != m_items.end(); ++it)
        delete *it;
    m_items.clear();
}

bool PopupListBox::isPointInBounds(const IntPoint& point)
{
    return numItems() != 0 && IntRect(0, 0, width(), height()).contains(point);
}


///////////////////////////////////////////////////////////////////////////////
// PopupMenu implementation
// 
// Note: you cannot add methods to this class, since it is defined above the 
//       portability layer. To access methods and properties on the
//       popup widgets, use |popupWindow| above. 

PopupMenu::PopupMenu(PopupMenuClient* client) 
    : m_popupClient(client)
{
}

PopupMenu::~PopupMenu()
{
    hide();
}

void PopupMenu::show(const IntRect& r, FrameView* v, int index) 
{
    p.m_popup = PopupContainer::create(client(), true);
    p.m_popup->show(r, v, index);
}

void PopupMenu::hide()
{
    if (p.m_popup) {
        p.m_popup->hidePopup();
        p.m_popup = 0;
    }
}

void PopupMenu::updateFromElement()
{
    p.m_popup->listBox()->updateFromElement();
}

bool PopupMenu::itemWritingDirectionIsNatural() 
{ 
    return false; 
}

} // namespace WebCore
