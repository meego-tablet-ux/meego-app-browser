// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/cookies_window_controller.h"

#include <queue>
#include <vector>

#include "app/l10n_util_mac.h"
#include "app/resource_bundle.h"
#import "base/i18n/time_formatting.h"
#import "base/mac_util.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/browsing_data_remover.h"
#include "chrome/browser/cocoa/clear_browsing_data_controller.h"
#include "chrome/browser/profile.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/apple/ImageAndTextCell.h"
#include "third_party/skia/include/core/SkBitmap.h"

// Key path used for notifying KVO.
static NSString* const kCocoaTreeModel = @"cocoaTreeModel";

CookiesTreeModelObserverBridge::CookiesTreeModelObserverBridge(
    CookiesWindowController* controller)
    : window_controller_(controller),
      batch_update_(false) {
}

// Notification that nodes were added to the specified parent.
void CookiesTreeModelObserverBridge::TreeNodesAdded(TreeModel* model,
                                                    TreeModelNode* parent,
                                                    int start,
                                                    int count) {
  // We're in for a major rebuild. Ignore this request.
  if (batch_update_ || !HasCocoaModel())
    return;

  CocoaCookieTreeNode* cocoa_parent = FindCocoaNode(parent, nil);
  NSMutableArray* cocoa_children = [cocoa_parent mutableChildren];

  [window_controller_ willChangeValueForKey:kCocoaTreeModel];
  CookieTreeNode* cookie_parent = static_cast<CookieTreeNode*>(parent);
  for (int i = 0; i < count; ++i) {
    CookieTreeNode* cookie_child = cookie_parent->GetChild(start + i);
    CocoaCookieTreeNode* new_child = CocoaNodeFromTreeNode(cookie_child);
    [cocoa_children addObject:new_child];
  }
  [window_controller_ didChangeValueForKey:kCocoaTreeModel];
}

// Notification that nodes were removed from the specified parent.
void CookiesTreeModelObserverBridge::TreeNodesRemoved(TreeModel* model,
                                                      TreeModelNode* parent,
                                                      int start,
                                                      int count) {
  // We're in for a major rebuild. Ignore this request.
  if (batch_update_ || !HasCocoaModel())
    return;

  CocoaCookieTreeNode* cocoa_parent = FindCocoaNode(parent, nil);
  [window_controller_ willChangeValueForKey:kCocoaTreeModel];
  NSMutableArray* cocoa_children = [cocoa_parent mutableChildren];
  for (int i = start + count - 1; i >= start; --i) {
    [cocoa_children removeObjectAtIndex:i];
  }
  [window_controller_ didChangeValueForKey:kCocoaTreeModel];
}

// Notification the children of |parent| have been reordered. Note, only
// the direct children of |parent| have been reordered, not descendants.
void CookiesTreeModelObserverBridge::TreeNodeChildrenReordered(TreeModel* model,
    TreeModelNode* parent) {
  // We're in for a major rebuild. Ignore this request.
  if (batch_update_ || !HasCocoaModel())
    return;

  CocoaCookieTreeNode* cocoa_parent = FindCocoaNode(parent, nil);
  NSMutableArray* cocoa_children = [cocoa_parent mutableChildren];

  CookieTreeNode* cookie_parent = static_cast<CookieTreeNode*>(parent);
  const int child_count = cookie_parent->GetChildCount();

  [window_controller_ willChangeValueForKey:kCocoaTreeModel];
  for (int i = 0; i < child_count; ++i) {
    CookieTreeNode* swap_in = cookie_parent->GetChild(i);
    for (int j = i; j < child_count; ++j) {
      CocoaCookieTreeNode* child = [cocoa_children objectAtIndex:j];
      TreeModelNode* swap_out = [child treeNode];
      if (swap_in == swap_out) {
        [cocoa_children exchangeObjectAtIndex:j withObjectAtIndex:i];
        break;
      }
    }
  }
  [window_controller_ didChangeValueForKey:kCocoaTreeModel];
}

// Notification that the contents of a node has changed.
void CookiesTreeModelObserverBridge::TreeNodeChanged(TreeModel* model,
                                                     TreeModelNode* node) {
  // If we don't have a Cocoa model, only let the root node change.
  if (batch_update_ || (!HasCocoaModel() && model->GetRoot() != node))
    return;

  if (HasCocoaModel()) {
    // We still have a Cocoa model, so just rebuild the node.
    [window_controller_ willChangeValueForKey:kCocoaTreeModel];
    CocoaCookieTreeNode* changed_node = FindCocoaNode(node, nil);
    [changed_node rebuild];
    [window_controller_ didChangeValueForKey:kCocoaTreeModel];
  } else {
    // Full rebuild.
    [window_controller_ setCocoaTreeModel:CocoaNodeFromTreeNode(node)];
  }
}

void CookiesTreeModelObserverBridge::TreeModelBeginBatch(
    CookiesTreeModel* model) {
  batch_update_ = true;
}

void CookiesTreeModelObserverBridge::TreeModelEndBatch(
    CookiesTreeModel* model) {
  DCHECK(batch_update_);
  CocoaCookieTreeNode* root = CocoaNodeFromTreeNode(model->GetRoot());
  [window_controller_ setCocoaTreeModel:root];
  batch_update_ = false;
}

void CookiesTreeModelObserverBridge::InvalidateCocoaModel() {
  [[[window_controller_ cocoaTreeModel] mutableChildren] removeAllObjects];
}

CocoaCookieTreeNode* CookiesTreeModelObserverBridge::CocoaNodeFromTreeNode(
    TreeModelNode* node) {
  CookieTreeNode* cookie_node = static_cast<CookieTreeNode*>(node);
  return [[[CocoaCookieTreeNode alloc] initWithNode:cookie_node] autorelease];
}

// Does breadth-first search on the tree to find |node|. This method is most
// commonly used to find origin/folder nodes, which are at the first level off
// the root (hence breadth-first search).
CocoaCookieTreeNode* CookiesTreeModelObserverBridge::FindCocoaNode(
    TreeModelNode* target, CocoaCookieTreeNode* start) {
  if (!start) {
    start = [window_controller_ cocoaTreeModel];
  }
  if ([start treeNode] == target) {
    return start;
  }

  // Enqueue the root node of the search (sub-)tree.
  std::queue<CocoaCookieTreeNode*> horizon;
  horizon.push(start);

  // Loop until we've looked at every node or we found the target.
  while (!horizon.empty()) {
    // Dequeue the item at the front.
    CocoaCookieTreeNode* node = horizon.front();
    horizon.pop();

    // If this is the droid we're looking for, report it.
    if ([node treeNode] == target)
      return node;

    // "Move along, move along." by adding all child nodes to the queue.
    if (![node isLeaf]) {
      NSArray* children = [node children];
      for (CocoaCookieTreeNode* child in children) {
        horizon.push(child);
      }
    }
  }

  return nil;  // We couldn't find the node.
}

// Returns whether or not the Cocoa tree model is built.
bool CookiesTreeModelObserverBridge::HasCocoaModel() {
  return ([[[window_controller_ cocoaTreeModel] children] count] > 0U);
}

#pragma mark Window Controller

@implementation CookiesWindowController

@synthesize removeButtonEnabled = removeButtonEnabled_;
@synthesize treeController = treeController_;

- (id)initWithProfile:(Profile*)profile
       databaseHelper:(BrowsingDataDatabaseHelper*)databaseHelper
        storageHelper:(BrowsingDataLocalStorageHelper*)storageHelper {
  DCHECK(profile);
  NSString* nibpath = [mac_util::MainAppBundle() pathForResource:@"Cookies"
                                                          ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibpath owner:self])) {
    profile_ = profile;
    databaseHelper_ = databaseHelper;
    storageHelper_ = storageHelper;

    [self loadTreeModelFromProfile];

    // Register for Clear Browsing Data controller so we update appropriately.
    ClearBrowsingDataController* clearingController =
        [ClearBrowsingDataController controllerForProfile:profile_];
    if (clearingController) {
      NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
      [center addObserver:self
                 selector:@selector(clearBrowsingDataNotification:)
                     name:kClearBrowsingDataControllerDidDelete
                   object:clearingController];
    }
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)awakeFromNib {
  DCHECK([self window]);
  DCHECK_EQ(self, [[self window] delegate]);
}

- (void)windowWillClose:(NSNotification*)notif {
  [searchField_ setTarget:nil];
  [outlineView_ setDelegate:nil];
  [self autorelease];
}

- (void)attachSheetTo:(NSWindow*)window {
  [NSApp beginSheet:[self window]
     modalForWindow:window
      modalDelegate:self
     didEndSelector:@selector(sheetEndSheet:returnCode:contextInfo:)
        contextInfo:nil];
}

- (void)sheetEndSheet:(NSWindow*)sheet
          returnCode:(NSInteger)returnCode
         contextInfo:(void*)context {
  [sheet close];
  [sheet orderOut:self];
}

- (IBAction)updateFilter:(id)sender {
  DCHECK([sender isKindOfClass:[NSSearchField class]]);
  NSString* string = [sender stringValue];
  // Invalidate the model here because all the nodes are going to be removed
  // in UpdateSearchResults(). This could lead to there temporarily being
  // invalid pointers in the Cocoa model.
  modelObserver_->InvalidateCocoaModel();
  treeModel_->UpdateSearchResults(base::SysNSStringToWide(string));
}

- (IBAction)deleteCookie:(id)sender {
  NSIndexPath* selectionPath = [treeController_ selectionIndexPath];
  // N.B.: I suspect that |-selectedObjects| does not retain/autorelease the
  // return value, which may result in the pointer going to garbage before it
  // even goes out of scope. Retaining it immediately will fix this.
  NSArray* selection = [treeController_ selectedObjects];
  if (selectionPath) {
    DCHECK_EQ([selection count], 1U);
    CocoaCookieTreeNode* node = [selection lastObject];
    CookieTreeNode* cookie = static_cast<CookieTreeNode*>([node treeNode]);
    treeModel_->DeleteCookieNode(cookie);
    // If there is a next cookie, this will select it because items will slide
    // up. If there is no next cookie, this is a no-op.
    [treeController_ setSelectionIndexPath:selectionPath];
  }
}

- (IBAction)deleteAllCookies:(id)sender {
  // Preemptively delete all cookies in the Cocoa model.
  modelObserver_->InvalidateCocoaModel();
  treeModel_->DeleteAllStoredObjects();
}

- (IBAction)closeSheet:(id)sender {
  [NSApp endSheet:[self window]];
}

- (void)clearBrowsingDataNotification:(NSNotification*)notif {
  NSNumber* removeMask =
      [[notif userInfo] objectForKey:kClearBrowsingDataControllerRemoveMask];
  if ([removeMask intValue] & BrowsingDataRemover::REMOVE_COOKIES) {
    [self loadTreeModelFromProfile];
  }
}

#pragma mark Getters and Setters

- (CocoaCookieTreeNode*)cocoaTreeModel {
  return cocoaTreeModel_.get();
}
- (void)setCocoaTreeModel:(CocoaCookieTreeNode*)model {
  cocoaTreeModel_.reset([model retain]);
}

- (CookiesTreeModel*)treeModel {
  return treeModel_.get();
}

#pragma mark Outline View Delegate

- (void)outlineView:(NSOutlineView*)outlineView
    willDisplayCell:(id)cell
     forTableColumn:(NSTableColumn*)tableColumn
               item:(id)item {
  CocoaCookieTreeNode* node = [item representedObject];
  int index = treeModel_->GetIconIndex([node treeNode]);
  NSImage* icon = nil;
  if (index >= 0)
    icon = [icons_ objectAtIndex:index];
  else
    icon = [icons_ lastObject];
  [(ImageAndTextCell*)cell setImage:icon];
}

- (void)outlineViewItemDidExpand:(NSNotification*)notif {
  NSTreeNode* item = [[notif userInfo] objectForKey:@"NSObject"];
  CocoaCookieTreeNode* node = [item representedObject];
  NSArray* children = [node children];
  if ([children count] == 1U) {
    // The node that will expand has one child. Do the user a favor and expand
    // that node (saving her a click) if it is non-leaf.
    CocoaCookieTreeNode* child = [children lastObject];
    if (![child isLeaf]) {
      NSOutlineView* outlineView = [notif object];
      // Tell the OutlineView to expand the NSTreeNode, not the model object.
      children = [item childNodes];
      DCHECK_EQ([children count], 1U);
      [outlineView expandItem:[children lastObject]];
      // Select the first node in that child set.
      NSTreeNode* folderChild = [children lastObject];
      if ([[folderChild childNodes] count] > 0) {
        NSTreeNode* firstCookieChild =
            [[folderChild childNodes] objectAtIndex:0];
        [treeController_ setSelectionIndexPath:[firstCookieChild indexPath]];
      }
    }
  }
}

- (void)outlineViewSelectionDidChange:(NSNotification*)notif {
  // Multi-selection should be disabled in the UI, but for sanity, double-check
  // that they can't do it here.
  NSArray* selectedObjects = [treeController_ selectedObjects];
  NSUInteger count = [selectedObjects count];
  if (count != 1U) {
    DCHECK_LT(count, 1U) << "User was able to select more than 1 cookie node!";
    [self setRemoveButtonEnabled:NO];

    // Make sure that the cookie info pane is shown when there is no selection.
    // That's what windows does.
    [cookieInfo_ setHidden:NO];
    [localStorageInfo_ setHidden:YES];
    [databaseInfo_ setHidden:YES];
    return;
  }

  // Go through the selection's indexPath and make sure that the node that is
  // being referenced actually exists in the Cocoa model.
  NSIndexPath* selection = [treeController_ selectionIndexPath];
  NSUInteger length = [selection length];
  CocoaCookieTreeNode* node = [self cocoaTreeModel];
  for (NSUInteger i = 0; i < length; ++i) {
    NSUInteger childIndex = [selection indexAtPosition:i];
    if (childIndex >= [[node children] count]) {
      [self setRemoveButtonEnabled:NO];
      return;
    }
    node = [[node children] objectAtIndex:childIndex];
  }

  [self setRemoveButtonEnabled:YES];
  CocoaCookieTreeNodeType nodeType = [[selectedObjects lastObject] nodeType];
  bool hideCookieInfoView = nodeType != kCocoaCookieTreeNodeTypeCookie &&
      nodeType != kCocoaCookieTreeNodeTypeFolder;
  bool hideLocaStorageInfoView =
      nodeType != kCocoaCookieTreeNodeTypeLocalStorage;
  bool hideDatabaseInfoView =
      nodeType != kCocoaCookieTreeNodeTypeDatabaseStorage;
  [cookieInfo_ setHidden:hideCookieInfoView];
  [localStorageInfo_ setHidden:hideLocaStorageInfoView];
  [databaseInfo_ setHidden:hideDatabaseInfoView];
}

#pragma mark Unit Testing

- (CookiesTreeModelObserverBridge*)modelObserver {
  return modelObserver_.get();
}

- (NSArray*)icons {
  return icons_.get();
}

- (NSView*)cookieInfoView {
  return cookieInfo_;
}

- (NSView*)localStorageInfoView {
  return localStorageInfo_;
}

- (NSView*)databaseInfoView {
  return databaseInfo_;
}

// Re-initializes the |treeModel_|, creates a new observer for it, and re-
// builds the |cocoaTreeModel_|. We use this to initialize the controller and
// to rebuild after the user clears browsing data. Because the models get
// clobbered, we rebuild the icon cache for safety (though they do not change).
- (void)loadTreeModelFromProfile {
  treeModel_.reset(new CookiesTreeModel(profile_, databaseHelper_,
                   storageHelper_, nil));
  modelObserver_.reset(new CookiesTreeModelObserverBridge(self));
  treeModel_->AddObserver(modelObserver_.get());

  // Convert the model's icons from Skia to Cocoa.
  std::vector<SkBitmap> skiaIcons;
  treeModel_->GetIcons(&skiaIcons);
  icons_.reset([[NSMutableArray alloc] init]);
  for (std::vector<SkBitmap>::iterator it = skiaIcons.begin();
       it != skiaIcons.end(); ++it) {
    [icons_ addObject:gfx::SkBitmapToNSImage(*it)];
  }

  // Default icon will be the last item in the array.
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  // TODO(rsesek): Rename this resource now that it's in multiple places.
  [icons_ addObject:rb.GetNSImageNamed(IDR_BOOKMARK_BAR_FOLDER)];

  // Create the Cocoa model.
  CookieTreeNode* root = static_cast<CookieTreeNode*>(treeModel_->GetRoot());
  scoped_nsobject<CocoaCookieTreeNode> model(
      [[CocoaCookieTreeNode alloc] initWithNode:root]);
  [self setCocoaTreeModel:model.get()];  // Takes ownership.
}

@end
