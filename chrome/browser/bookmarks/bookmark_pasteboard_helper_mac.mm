// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/bookmarks/bookmark_pasteboard_helper_mac.h"

#import <Cocoa/Cocoa.h>

#include "base/sys_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/cocoa/bookmark_drag_source.h"
#include "chrome/browser/tab_contents/tab_contents_view_mac.h"

namespace {

// Pasteboard type used to store profile path to determine which profile
// a set of bookmarks came from.
NSString* const ChromiumProfilePathPboardType =
    @"ChromiumProfilePathPboardType";

// Internal bookmark ID for a bookmark node.  Used only when moving inside
// of one profile.
NSString* const kChromiumBookmarkId =
    @"ChromiumBookmarkId";

// Mac WebKit uses this type, declared in
// WebKit/mac/History/WebURLsWithTitles.h
NSString* const WebURLsWithTitlesPboardType =
    @"WebURLsWithTitlesPboardType";

NSString* const BookmarkDictionaryListPboardType =
    @"BookmarkDictionaryListPboardType";

// Keys for the type of node in BookmarkDictionaryListPboardType
NSString* const WebBookmarkType =
    @"WebBookmarkType";

NSString* const WebBookmarkTypeList =
    @"WebBookmarkTypeList";

NSString* const WebBookmarkTypeLeaf =
    @"WebBookmarkTypeLeaf";

void ConvertPlistToElements(NSArray* input,
                            std::vector<BookmarkDragData::Element>& elements) {
  NSUInteger len = [input count];
  for (NSUInteger i = 0; i < len; ++i) {
    NSDictionary* pboardBookmark = [input objectAtIndex:i];
    scoped_ptr<BookmarkNode> new_node(new BookmarkNode(0, GURL()));
    int64 node_id =
        [[pboardBookmark objectForKey:kChromiumBookmarkId] longLongValue];
    new_node->set_id(node_id);
    BOOL is_folder = [[pboardBookmark objectForKey:WebBookmarkType]
        isEqualToString:WebBookmarkTypeList];
    if (is_folder) {
      new_node->set_type(BookmarkNode::FOLDER);
      NSString* title = [pboardBookmark objectForKey:@"Title"];
      new_node->SetTitle(base::SysNSStringToUTF16(title));
    } else {
      new_node->set_type(BookmarkNode::URL);
      NSDictionary* uriDictionary =
          [pboardBookmark objectForKey:@"URIDictionary"];
      NSString* title = [uriDictionary objectForKey:@"title"];
      NSString* urlString = [pboardBookmark objectForKey:@"URLString"];
      new_node->SetTitle(base::SysNSStringToUTF16(title));
      new_node->SetURL(GURL(base::SysNSStringToUTF8(urlString)));
    }
    BookmarkDragData::Element e = BookmarkDragData::Element(new_node.get());
    if(is_folder)
      ConvertPlistToElements([pboardBookmark objectForKey:@"Children"],
                             e.children);
    elements.push_back(e);
  }
}

bool ReadBookmarkDictionaryListPboardType(NSPasteboard* pb,
      std::vector<BookmarkDragData::Element>& elements) {
  NSArray* bookmarks = [pb propertyListForType:
      BookmarkDictionaryListPboardType];
  if (!bookmarks) return false;
  ConvertPlistToElements(bookmarks, elements);
  return true;
}

bool ReadWebURLsWithTitlesPboardType(NSPasteboard* pb,
      std::vector<BookmarkDragData::Element>& elements) {
  NSArray* bookmarkPairs =
      [pb propertyListForType:WebURLsWithTitlesPboardType];
  if (![bookmarkPairs isKindOfClass:[NSArray class]]) {
    return false;
  }
  NSArray* urlsArr = [bookmarkPairs objectAtIndex:0];
  NSArray* titlesArr = [bookmarkPairs objectAtIndex:1];
  if ([urlsArr count] < 1) {
    return false;
  }
  if ([urlsArr count] != [titlesArr count]) {
    return false;
  }

  int len = [titlesArr count];
  for (int i = 0; i < len; ++i) {
    string16 title = base::SysNSStringToUTF16([titlesArr objectAtIndex:i]);
    std::string url = base::SysNSStringToUTF8([urlsArr objectAtIndex:i]);
    if (!url.empty()) {
      BookmarkDragData::Element element;
      element.is_url = true;
      element.url = GURL(url);
      element.title = title;
      elements.push_back(element);
    }
  }
  return true;
}

bool ReadNSURLPboardType(NSPasteboard* pb,
                         std::vector<BookmarkDragData::Element>& elements) {
  NSURL* url = [NSURL URLFromPasteboard:pb];
  if (url == nil) {
    return false;
  }
  std::string urlString = base::SysNSStringToUTF8([url absoluteString]);
  NSString* title = [pb stringForType:@"public.url-name"];
  if (!title)
    title = [pb stringForType:NSStringPboardType];

  BookmarkDragData::Element element;
  element.is_url = true;
  element.url = GURL(urlString);
  element.title = base::SysNSStringToUTF16(title);
  elements.push_back(element);
  return true;
}

NSArray* GetPlistForBookmarkList(
    const std::vector<BookmarkDragData::Element>& elements) {
  NSMutableArray* plist = [NSMutableArray array];
  for (size_t i = 0; i < elements.size(); ++i) {
    BookmarkDragData::Element element = elements[i];
    if (element.is_url) {
      NSString* title = base::SysUTF16ToNSString(element.title);
      NSString* url = base::SysUTF8ToNSString(element.url.spec());
      int64 elementId = element.get_id();
      NSNumber* idNum = [NSNumber numberWithLongLong:elementId];
      NSDictionary* uriDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
              title, @"title", nil];
      NSDictionary* object = [NSDictionary dictionaryWithObjectsAndKeys:
          uriDictionary, @"URIDictionary",
          url, @"URLString",
          WebBookmarkTypeLeaf, WebBookmarkType,
          idNum, kChromiumBookmarkId,
          nil];
      [plist addObject:object];
    } else {
      NSString* title = base::SysUTF16ToNSString(element.title);
      NSArray* children = GetPlistForBookmarkList(element.children);
      int64 elementId = element.get_id();
      NSNumber* idNum = [NSNumber numberWithLongLong:elementId];
      NSDictionary* object = [NSDictionary dictionaryWithObjectsAndKeys:
          title, @"Title",
          children, @"Children",
          WebBookmarkTypeList, WebBookmarkType,
          idNum, kChromiumBookmarkId,
          nil];
      [plist addObject:object];
    }
  }
  return plist;
}

void WriteBookmarkDictionaryListPboardType(NSPasteboard* pb,
    const std::vector<BookmarkDragData::Element>& elements) {
  NSArray* plist = GetPlistForBookmarkList(elements);
  [pb setPropertyList:plist forType:BookmarkDictionaryListPboardType];
}

void FillFlattenedArraysForBookmarks(
    const std::vector<BookmarkDragData::Element>& elements,
    NSMutableArray* titles, NSMutableArray* urls) {
  for (size_t i = 0; i < elements.size(); ++i) {
    BookmarkDragData::Element element = elements[i];
    if (element.is_url) {
      NSString* title = base::SysUTF16ToNSString(element.title);
      NSString* url = base::SysUTF8ToNSString(element.url.spec());
      [titles addObject:title];
      [urls addObject:url];
    } else {
      FillFlattenedArraysForBookmarks(element.children, titles, urls);
    }
  }
}

void WriteSimplifiedBookmarkTypes(NSPasteboard* pb,
    const std::vector<BookmarkDragData::Element>& elements) {
  NSMutableArray* titles = [NSMutableArray array];
  NSMutableArray* urls = [NSMutableArray array];
  FillFlattenedArraysForBookmarks(elements, titles, urls);

  //Write WebURLsWithTitlesPboardType
  [pb setPropertyList:[NSArray arrayWithObjects:urls, titles, nil]
              forType:WebURLsWithTitlesPboardType];

  //Write NSStringPboardType
  [pb setString:[urls componentsJoinedByString:@"\n"]
      forType:NSStringPboardType];

  // Write NSURLPboardType
  NSURL* url = [NSURL URLWithString:[urls objectAtIndex:0]];
  [url writeToPasteboard:pb];
  NSString* titleString = [titles objectAtIndex:0];
  [pb setString:titleString forType:@"public.url-name"];
}

void WriteToClipboardPrivate(
    const std::vector<BookmarkDragData::Element>& elements,
    NSPasteboard* pb,
    FilePath::StringType profile_path) {
  if (elements.size() == 0) {
    return;
  }
  NSArray* types = [NSArray arrayWithObjects:BookmarkDictionaryListPboardType,
                                             WebURLsWithTitlesPboardType,
                                             NSStringPboardType,
                                             NSURLPboardType,
                                             @"public.url-name",
                                             ChromiumProfilePathPboardType,
                                             nil];
  [pb declareTypes:types owner:nil];
  [pb setString:base::SysUTF8ToNSString(profile_path)
        forType:ChromiumProfilePathPboardType];
  WriteBookmarkDictionaryListPboardType(pb, elements);
  WriteSimplifiedBookmarkTypes(pb, elements);
}

bool ReadFromClipboardPrivate(
    std::vector<BookmarkDragData::Element>& elements,
    NSPasteboard* pb,
    FilePath::StringType* profile_path) {
  elements.clear();
  NSString* profile = [pb stringForType:ChromiumProfilePathPboardType];
  profile_path->assign(base::SysNSStringToUTF8(profile));
  return (ReadBookmarkDictionaryListPboardType(pb, elements) ||
         ReadWebURLsWithTitlesPboardType(pb, elements) ||
         ReadNSURLPboardType(pb, elements));
}

bool ClipboardContainsBookmarksPrivate(NSPasteboard* pb) {
  NSArray* availableTypes = [NSArray arrayWithObjects:
                                BookmarkDictionaryListPboardType,
                                WebURLsWithTitlesPboardType,
                                NSURLPboardType,
                                nil];
  return [pb availableTypeFromArray:availableTypes] != nil;
}

}  // anonymous namespace

namespace bookmark_pasteboard_helper_mac {

void WriteToClipboard(const std::vector<BookmarkDragData::Element>& elements,
                      FilePath::StringType profile_path) {
  NSPasteboard* pb = [NSPasteboard generalPasteboard];
  WriteToClipboardPrivate(elements, pb, profile_path);
}

void WriteToDragClipboard(
    const std::vector<BookmarkDragData::Element>& elements,
    FilePath::StringType profile_path) {
  NSPasteboard* pb = [NSPasteboard pasteboardWithName:NSDragPboard];
  WriteToClipboardPrivate(elements, pb, profile_path);
}

bool ReadFromClipboard(std::vector<BookmarkDragData::Element>& elements,
                       FilePath::StringType* profile_path) {
  NSPasteboard* pb = [NSPasteboard generalPasteboard];
  return ReadFromClipboardPrivate(elements, pb, profile_path);
}

bool ReadFromDragClipboard(std::vector<BookmarkDragData::Element>& elements,
                           FilePath::StringType* profile_path) {
  NSPasteboard* pb = [NSPasteboard pasteboardWithName:NSDragPboard];
  return ReadFromClipboardPrivate(elements, pb, profile_path);
}


bool ClipboardContainsBookmarks() {
  NSPasteboard* pb = [NSPasteboard generalPasteboard];
  return ClipboardContainsBookmarksPrivate(pb);
}

bool DragClipboardContainsBookmarks() {
  NSPasteboard* pb = [NSPasteboard pasteboardWithName:NSDragPboard];
  return ClipboardContainsBookmarksPrivate(pb);
}

void StartDrag(Profile* profile, const std::vector<const BookmarkNode*>& nodes,
    gfx::NativeView view) {
  DCHECK([view isKindOfClass:[TabContentsViewCocoa class]]);
  TabContentsViewCocoa* tabView = static_cast<TabContentsViewCocoa*>(view);
  std::vector<BookmarkDragData::Element> elements;
  for (std::vector<const BookmarkNode*>::const_iterator it = nodes.begin();
       it != nodes.end(); ++it) {
    elements.push_back(BookmarkDragData::Element(*it));
  }
  NSPasteboard* pb = [NSPasteboard pasteboardWithName:NSDragPboard];
  scoped_nsobject<BookmarkDragSource> source([[BookmarkDragSource alloc]
      initWithContentsView:tabView
                  dropData:elements
                   profile:profile
                pasteboard:pb
         dragOperationMask:NSDragOperationEvery]);
  [source startDrag];
}

}  // namespace bookmark_pasteboard_helper_mac
