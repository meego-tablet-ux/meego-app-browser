// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/autofill/autofill_dialog_controller_mac.h"
#include "app/l10n_util.h"
#include "app/resource_bundle.h"
#include "base/mac_util.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#import "chrome/browser/autofill/autofill_address_model_mac.h"
#import "chrome/browser/autofill/autofill_address_sheet_controller_mac.h"
#import "chrome/browser/autofill/autofill_credit_card_model_mac.h"
#import "chrome/browser/autofill/autofill_credit_card_sheet_controller_mac.h"
#import "chrome/browser/autofill/personal_data_manager.h"
#include "chrome/browser/browser_process.h"
#import "chrome/browser/cocoa/window_size_autosaver.h"
#include "chrome/browser/pref_service.h"
#include "chrome/browser/profile.h"
#include "chrome/common/pref_names.h"
#include "grit/generated_resources.h"
#include "grit/app_resources.h"
#include "grit/theme_resources.h"

// Delegate protocol that needs to be in place for the AutoFillTableView's
// handling of delete and backspace keys.
@protocol DeleteKeyDelegate
- (IBAction)deleteSelection:(id)sender;
@end

// A subclass of NSTableView that allows for deleting selected elements using
// the delete or backspace keys.
@interface AutoFillTableView : NSTableView {
}
@end

@implementation AutoFillTableView

// We override the keyDown method to dispatch the |deleteSelection:| action
// when the user presses the delete or backspace keys.  Note a delegate must
// be present that conforms to the DeleteKeyDelegate protocol.
- (void)keyDown:(NSEvent *)event {
  id object = [self delegate];
  unichar c = [[event characters] characterAtIndex: 0];

  // If the user pressed delete and the delegate supports deleteSelection:
  if ((c == NSDeleteFunctionKey ||
       c == NSDeleteCharFunctionKey ||
       c == NSDeleteCharacter) &&
      [object respondsToSelector:@selector(deleteSelection:)]) {
    id <DeleteKeyDelegate> delegate = (id <DeleteKeyDelegate>) object;

    [delegate deleteSelection:self];
  } else {
    [super keyDown:event];
  }
}

@end

// Private interface.
@interface AutoFillDialogController (PrivateMethods)
// Asyncronous handler for when PersonalDataManager data loads.  The
// personal data manager notifies the dialog with this method when the
// data loading is complete and ready to be used.
- (void)onPersonalDataLoaded:(const std::vector<AutoFillProfile*>&)profiles
                 creditCards:(const std::vector<CreditCard*>&)creditCards;

// Returns true if |row| is an index to a valid profile in |tableView_|, and
// false otherwise.
- (BOOL)isProfileRow:(NSInteger)row;

// Returns true if |row| is an index to the profile group row in |tableView_|,
// and false otherwise.
- (BOOL)isProfileGroupRow:(NSInteger)row;

// Returns true if |row| is an index to a valid credit card in |tableView_|, and
// false otherwise.
- (BOOL)isCreditCardRow:(NSInteger)row;

// Returns true if |row| is the index to the credit card group row in
// |tableView_|, and false otherwise.
- (BOOL)isCreditCardGroupRow:(NSInteger)row;

// Returns the index to |profiles_| of the corresponding |row| in |tableView_|.
- (size_t)profileIndexFromRow:(NSInteger)row;

// Returns the index to |creditCards_| of the corresponding |row| in
// |tableView_|.
- (size_t)creditCardIndexFromRow:(NSInteger)row;

// Returns the  |row| in |tableView_| that corresponds to the index |i| into
// |profiles_|.
- (NSInteger)rowFromProfileIndex:(size_t)i;

// Returns the  |row| in |tableView_| that corresponds to the index |i| into
// |creditCards_|.
- (NSInteger)rowFromCreditCardIndex:(size_t)row;

// Invokes the modal dialog.
- (void)runModalDialog;

@end

namespace AutoFillDialogControllerInternal {

// PersonalDataManagerObserver facilitates asynchronous loading of
// PersonalDataManager data before showing the AutoFill settings data to the
// user.  It acts as a C++-based delegate for the |AutoFillDialogController|.
class PersonalDataManagerObserver : public PersonalDataManager::Observer {
 public:
  explicit PersonalDataManagerObserver(
      AutoFillDialogController* controller,
      PersonalDataManager* personal_data_manager,
      Profile* profile)
      : controller_(controller),
        personal_data_manager_(personal_data_manager),
        profile_(profile) {
  }

  virtual ~PersonalDataManagerObserver();

  // Notifies the observer that the PersonalDataManager has finished loading.
  virtual void OnPersonalDataLoaded();

 private:
  // Utility method to remove |this| from |personal_data_manager_| as an
  // observer.
  void RemoveObserver();

  // The dialog controller to be notified when the data loading completes.
  // Weak reference.
  AutoFillDialogController* controller_;

  // The object in which we are registered as an observer.  We hold on to
  // it to facilitate un-registering ourself in the destructor and in the
  // |OnPersonalDataLoaded| method.  This may be NULL.
  // Weak reference.
  PersonalDataManager* personal_data_manager_;

  // Profile of caller.  Held as weak reference.  May not be NULL.
  Profile* profile_;

 private:
  DISALLOW_COPY_AND_ASSIGN(PersonalDataManagerObserver);
};

// During destruction ensure that we are removed from the
// |personal_data_manager_| as an observer.
PersonalDataManagerObserver::~PersonalDataManagerObserver() {
  RemoveObserver();
}

void PersonalDataManagerObserver::RemoveObserver() {
  if (personal_data_manager_) {
    personal_data_manager_->RemoveObserver(this);
  }
}

// The data is ready so display our data.  Notify the dialog controller that
// the data is ready.  Once done we clear the observer.
void PersonalDataManagerObserver::OnPersonalDataLoaded() {
  RemoveObserver();
  [controller_ onPersonalDataLoaded:personal_data_manager_->web_profiles()
                        creditCards:personal_data_manager_->credit_cards()];
}

}  // namespace AutoFillDialogControllerInternal

@implementation AutoFillDialogController

@synthesize autoFillEnabled = autoFillEnabled_;
@synthesize auxiliaryEnabled = auxiliaryEnabled_;
@synthesize itemIsSelected = itemIsSelected_;

+ (void)showAutoFillDialogWithObserver:(AutoFillDialogObserver*)observer
               profile:(Profile*)profile
       importedProfile:(AutoFillProfile*) importedProfile
    importedCreditCard:(CreditCard*) importedCreditCard {
  AutoFillDialogController* controller =
      [AutoFillDialogController controllerWithObserver:observer
                                         profile:profile
                                 importedProfile:importedProfile
                              importedCreditCard:importedCreditCard];

  // Only run modal dialog if it is not already being shown.
  if (![controller isWindowLoaded]) {
    [controller runModalDialog];
  }
}

- (void)awakeFromNib {
  PersonalDataManager* personal_data_manager =
      profile_->GetPersonalDataManager();
  DCHECK(personal_data_manager);

  if (personal_data_manager->IsDataLoaded()) {
    // |personalDataManager| data is loaded, we can proceed with the contents.
    [self onPersonalDataLoaded:personal_data_manager->web_profiles()
                   creditCards:personal_data_manager->credit_cards()];
  } else {
    // |personalDataManager| data is NOT loaded, so we load it here, installing
    // our observer.
    personalDataManagerObserver_.reset(
        new AutoFillDialogControllerInternal::PersonalDataManagerObserver(
            self, personal_data_manager, profile_));
    personal_data_manager->SetObserver(personalDataManagerObserver_.get());
  }

  // Explicitly load the data in the table before window displays to avoid
  // nasty flicker as tables update.
  [tableView_ reloadData];

  // Set up edit when double-clicking on a table row.
  [tableView_ setDoubleAction:@selector(editSelection:)];
}

// NSWindow Delegate callback.  When the window closes the controller can
// be released.
- (void)windowWillClose:(NSNotification *)notification {
  [tableView_ setDataSource:nil];
  [tableView_ setDelegate:nil];
  [self autorelease];
}

// Called when the user clicks the save button.
- (IBAction)save:(id)sender {
  // If we have an |observer_| then communicate the changes back.
  if (observer_) {
    profile_->GetPrefs()->SetBoolean(prefs::kAutoFillEnabled, autoFillEnabled_);
    profile_->GetPrefs()->SetBoolean(prefs::kAutoFillAuxiliaryProfilesEnabled,
                                     auxiliaryEnabled_);
    observer_->OnAutoFillDialogApply(&profiles_, &creditCards_);
  }
  [self closeDialog];
}

// Called when the user clicks the cancel button. All we need to do is stop
// the modal session.
- (IBAction)cancel:(id)sender {
  [self closeDialog];
}

// Invokes the "Add" sheet for address information.  If user saves then the new
// information is added to |profiles_| in |addressAddDidEnd:| method.
- (IBAction)addNewAddress:(id)sender {
  DCHECK(!addressSheetController.get());

  // Create a new default address.
  string16 newName = l10n_util::GetStringUTF16(IDS_AUTOFILL_NEW_ADDRESS);
  AutoFillProfile newAddress(newName, 0);

  // Create a new address sheet controller in "Add" mode.
  addressSheetController.reset(
      [[AutoFillAddressSheetController alloc]
          initWithProfile:newAddress
                     mode:kAutoFillAddressAddMode]);

  // Show the sheet.
  [NSApp beginSheet:[addressSheetController window]
     modalForWindow:[self window]
      modalDelegate:self
     didEndSelector:@selector(addressAddDidEnd:returnCode:contextInfo:)
        contextInfo:NULL];
}

// Invokes the "Add" sheet for credit card information.  If user saves then the
// new information is added to |creditCards_| in |creditCardAddDidEnd:| method.
- (IBAction)addNewCreditCard:(id)sender {
  DCHECK(!creditCardSheetController.get());

  // Create a new default credit card.
  string16 newName = l10n_util::GetStringUTF16(IDS_AUTOFILL_NEW_CREDITCARD);
  CreditCard newCreditCard(newName, 0);

  // Create a new address sheet controller in "Add" mode.
  creditCardSheetController.reset(
      [[AutoFillCreditCardSheetController alloc]
          initWithCreditCard:newCreditCard
                        mode:kAutoFillCreditCardAddMode
                  controller:self]);

  // Show the sheet.
  [NSApp beginSheet:[creditCardSheetController window]
     modalForWindow:[self window]
      modalDelegate:self
     didEndSelector:@selector(creditCardAddDidEnd:returnCode:contextInfo:)
        contextInfo:NULL];
}

// Add address sheet was dismissed.  Non-zero |returnCode| indicates a save.
- (void)addressAddDidEnd:(NSWindow*)sheet
              returnCode:(int)returnCode
             contextInfo:(void*)contextInfo {
  DCHECK(contextInfo == NULL);

  if (returnCode) {
    // Create a new address and save it to the |profiles_| list.
    AutoFillProfile newAddress(string16(), 0);
    [addressSheetController copyModelToProfile:&newAddress];
    profiles_.push_back(newAddress);

    // Refresh the view based on new data.
    [tableView_ reloadData];

    // Update the selection to the newly added item.
    NSInteger row = [self rowFromProfileIndex:profiles_.size() - 1];
    [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:row]
            byExtendingSelection:NO];
  }
  [sheet orderOut:self];
  addressSheetController.reset(nil);
}

// Add credit card sheet was dismissed.  Non-zero |returnCode| indicates a save.
- (void)creditCardAddDidEnd:(NSWindow *)sheet
                 returnCode:(int)returnCode
                contextInfo:(void *)contextInfo {
  DCHECK(contextInfo == NULL);

  if (returnCode) {
    // Create a new credit card and save it to the |creditCards_| list.
    CreditCard newCreditCard(string16(), 0);
    [creditCardSheetController copyModelToCreditCard:&newCreditCard];
    creditCards_.push_back(newCreditCard);

    // Refresh the view based on new data.
    [tableView_ reloadData];

    // Update the selection to the newly added item.
    NSInteger row = [self rowFromCreditCardIndex:creditCards_.size() - 1];
    [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:row]
            byExtendingSelection:NO];
  }
  [sheet orderOut:self];
  creditCardSheetController.reset(nil);
}

// Deletes selected item, either address or credit card depending on the item
// selected.
- (IBAction)deleteSelection:(id)sender {
  NSInteger selectedRow = [tableView_ selectedRow];
  if ([self isProfileRow:selectedRow]) {
    profiles_.erase(profiles_.begin() + [self profileIndexFromRow:selectedRow]);

    // Select the previous row if possible, else current row, else deselect all.
    if ([self tableView:tableView_ shouldSelectRow:selectedRow-1]) {
      [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow-1]
              byExtendingSelection:NO];
    } else if ([self tableView:tableView_ shouldSelectRow:selectedRow]) {
      [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow]
              byExtendingSelection:NO];
    } else {
      [tableView_ selectRowIndexes:[NSIndexSet indexSet]
              byExtendingSelection:NO];
    }
    [tableView_ reloadData];
  } else if ([self isCreditCardRow:selectedRow]) {
    creditCards_.erase(
        creditCards_.begin() + [self creditCardIndexFromRow:selectedRow]);

    // Select the previous row if possible, else current row, else deselect all.
    if ([self tableView:tableView_ shouldSelectRow:selectedRow-1]) {
      [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow-1]
              byExtendingSelection:NO];
    } else if ([self tableView:tableView_ shouldSelectRow:selectedRow]) {
      [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:selectedRow]
              byExtendingSelection:NO];
    } else {
      [tableView_ selectRowIndexes:[NSIndexSet indexSet]
              byExtendingSelection:NO];
    }
    [tableView_ reloadData];
  }
}

// Edits the selected item, either address or credit card depending on the item
// selected.
- (IBAction)editSelection:(id)sender {
  NSInteger selectedRow = [tableView_ selectedRow];
  if ([self isProfileRow:selectedRow]) {
    if (!addressSheetController.get()) {
      int i = [self profileIndexFromRow:selectedRow];

      // Create a new address sheet controller in "Edit" mode.
      addressSheetController.reset(
          [[AutoFillAddressSheetController alloc]
              initWithProfile:profiles_[i]
                         mode:kAutoFillAddressEditMode]);

      // Show the sheet.
      [NSApp beginSheet:[addressSheetController window]
         modalForWindow:[self window]
          modalDelegate:self
         didEndSelector:@selector(addressEditDidEnd:returnCode:contextInfo:)
            contextInfo:&profiles_[i]];
    }
  } else if ([self isCreditCardRow:selectedRow]) {
    if (!creditCardSheetController.get()) {
      int i = [self creditCardIndexFromRow:selectedRow];

      // Create a new credit card sheet controller in "Edit" mode.
      creditCardSheetController.reset(
          [[AutoFillCreditCardSheetController alloc]
              initWithCreditCard:creditCards_[i]
                            mode:kAutoFillCreditCardEditMode
                      controller:self]);

      // Show the sheet.
      [NSApp beginSheet:[creditCardSheetController window]
         modalForWindow:[self window]
          modalDelegate:self
         didEndSelector:@selector(creditCardEditDidEnd:returnCode:contextInfo:)
            contextInfo:&creditCards_[i]];
    }
  }
}

// Navigates to the AutoFill help url.
- (IBAction)openHelp:(id)sender {
  Browser* browser = BrowserList::GetLastActive();

  if (!browser || !browser->GetSelectedTabContents())
    browser = Browser::Create(profile_);
  browser->OpenAutoFillHelpTabAndActivate();
}

// Edit address sheet was dismissed.  Non-zero |returnCode| indicates a save.
- (void)addressEditDidEnd:(NSWindow *)sheet
               returnCode:(int)returnCode
              contextInfo:(void *)contextInfo {
  DCHECK(contextInfo != NULL);
  if (returnCode) {
    AutoFillProfile* profile = static_cast<AutoFillProfile*>(contextInfo);
    [addressSheetController copyModelToProfile:profile];
    [tableView_ reloadData];
  }
  [sheet orderOut:self];
  addressSheetController.reset(nil);
}

// Edit credit card sheet was dismissed.  Non-zero |returnCode| indicates a
// save.
- (void)creditCardEditDidEnd:(NSWindow *)sheet
                  returnCode:(int)returnCode
                 contextInfo:(void *)contextInfo {
  DCHECK(contextInfo != NULL);
  if (returnCode) {
    CreditCard* creditCard = static_cast<CreditCard*>(contextInfo);
    [creditCardSheetController copyModelToCreditCard:creditCard];
    [tableView_ reloadData];
  }
  [sheet orderOut:self];
  creditCardSheetController.reset(nil);
}

// NSTableView Delegate method.
- (BOOL)tableView:(NSTableView *)tableView isGroupRow:(NSInteger)row {
  if ([self isProfileGroupRow:row] || [self isCreditCardGroupRow:row])
    return YES;
  return NO;
}

// NSTableView Delegate method.
- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row {
  return ![self tableView:tableView isGroupRow:row];
}

// NSTableView Delegate method.
- (id)tableView:(NSTableView *)tableView
  objectValueForTableColumn:(NSTableColumn *)tableColumn
                        row:(NSInteger)row {
  if ([[tableColumn identifier] isEqualToString:@"Spacer"])
    return @"";

  // Check that we're initialized before supplying data.
  if (tableView == tableView_) {

    // Section label.
    if ([self isProfileGroupRow:row])
      if ([[tableColumn identifier] isEqualToString:@"Label"])
        return @"Addresses";
      else
        return @"";

    if (row < 0)
      return @"";

    // Data row.
    if ([self isProfileRow:row]) {
      if ([[tableColumn identifier] isEqualToString:@"Label"])
        return SysUTF16ToNSString(
            profiles_[[self profileIndexFromRow:row]].Label());

      if ([[tableColumn identifier] isEqualToString:@"Summary"])
        return SysUTF16ToNSString(
            profiles_[[self profileIndexFromRow:row]].PreviewSummary());

      return @"";
    }

    // Section label.
    if ([self isCreditCardGroupRow:row])
      if ([[tableColumn identifier] isEqualToString:@"Label"])
        return @"Credit Cards";
      else
        return @"";

    // Data row.
    if ([self isCreditCardRow:row]) {
      if ([[tableColumn identifier] isEqualToString:@"Label"])
        return SysUTF16ToNSString(
            creditCards_[[self creditCardIndexFromRow:row]].Label());

      if ([[tableColumn identifier] isEqualToString:@"Summary"])
        return SysUTF16ToNSString(
            creditCards_[
                [self creditCardIndexFromRow:row]].PreviewSummary());

      return @"";
    }
  }

  return @"";
}

// We implement this delegate method to update our |itemIsSelected| property.
// The "Edit..." and "Remove" buttons' enabled state depends on having a
// valid selection in the table.
- (void)tableViewSelectionDidChange:(NSNotification *)aNotification {
  if ([tableView_ selectedRow] >= 0)
    [self setItemIsSelected:YES];
  else
    [self setItemIsSelected:NO];
}

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
  if (tableView == tableView_) {
    // 1 section header, the profiles, 1 section header, the credit cards.
    return 1 + profiles_.size() + 1 + creditCards_.size();
  }

  return 0;
}

- (NSArray*)addressLabels {
  NSUInteger capacity = profiles_.size();
  NSMutableArray* array = [NSMutableArray arrayWithCapacity:capacity];

  std::vector<AutoFillProfile>::iterator i;
  for (i = profiles_.begin(); i != profiles_.end(); ++i) {
    [array addObject:SysUTF16ToNSString(i->Label())];
  }

  return array;
}

@end

@implementation AutoFillDialogController (ExposedForUnitTests)

+ (AutoFillDialogController*)controllerWithObserver:
        (AutoFillDialogObserver*)observer
               profile:(Profile*)profile
       importedProfile:(AutoFillProfile*)importedProfile
    importedCreditCard:(CreditCard*)importedCreditCard {

  // Deallocation is done upon window close.  See |windowWillClose:|.
  AutoFillDialogController* controller =
      [[self alloc] initWithObserver:observer
                             profile:profile
                     importedProfile:importedProfile
                  importedCreditCard:importedCreditCard];
  return controller;
}


// This is the designated initializer for this class.
// |profiles| are non-retained immutable list of autofill profiles.
// |creditCards| are non-retained immutable list of credit card info.
- (id)initWithObserver:(AutoFillDialogObserver*)observer
               profile:(Profile*)profile
       importedProfile:(AutoFillProfile*)importedProfile
    importedCreditCard:(CreditCard*)importedCreditCard {
  DCHECK(profile);
  // Use initWithWindowNibPath: instead of initWithWindowNibName: so we
  // can override it in a unit test.
  NSString* nibpath = [mac_util::MainAppBundle()
                       pathForResource:@"AutoFillDialog"
                       ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibpath owner:self])) {
    // Initialize member variables based on input.
    observer_ = observer;
    profile_ = profile;
    importedProfile_ = importedProfile;
    importedCreditCard_ = importedCreditCard;

    // Use property here to trigger KVO binding.
    [self setAutoFillEnabled:profile_->GetPrefs()->GetBoolean(
        prefs::kAutoFillEnabled)];

    // Use property here to trigger KVO binding.
    [self setAuxiliaryEnabled:profile_->GetPrefs()->GetBoolean(
        prefs::kAutoFillAuxiliaryProfilesEnabled)];

    // Do not use [NSMutableArray array] here; we need predictable destruction
    // which will be prevented by having a reference held by an autorelease
    // pool.
  }
  return self;
}

// Close the dialog.
- (void)closeDialog {
  [[self window] close];
  [NSApp stopModal];
}

- (AutoFillAddressSheetController*)addressSheetController {
  return addressSheetController.get();
}

- (AutoFillCreditCardSheetController*)creditCardSheetController {
  return creditCardSheetController.get();
}

- (void)selectAddressAtIndex:(size_t)i {
  [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:
                                   [self rowFromProfileIndex:i]]
          byExtendingSelection:NO];
}

- (void)selectCreditCardAtIndex:(size_t)i {
  [tableView_ selectRowIndexes:[NSIndexSet indexSetWithIndex:
                                   [self rowFromCreditCardIndex:i]]
          byExtendingSelection:NO];
}

@end

@implementation AutoFillDialogController (PrivateMethods)

// Run application modal.
- (void)runModalDialog {
  // Use stored window geometry if it exists.
  if (g_browser_process && g_browser_process->local_state()) {
    sizeSaver_.reset([[WindowSizeAutosaver alloc]
        initWithWindow:[self window]
           prefService:g_browser_process->local_state()
                  path:prefs::kAutoFillDialogPlacement
                 state:kSaveWindowPos]);
  }

  [NSApp runModalForWindow:[self window]];
}

- (void)onPersonalDataLoaded:(const std::vector<AutoFillProfile*>&)profiles
                 creditCards:(const std::vector<CreditCard*>&)creditCards {
  if (importedProfile_) {
      profiles_.push_back(*importedProfile_);
  }

  if (importedCreditCard_) {
      creditCards_.push_back(*importedCreditCard_);
  }

  // If we're not using imported data then use the data fetch from the web db.
  if (!importedProfile_ && !importedCreditCard_) {
    // Make local copy of |profiles|.
    for (std::vector<AutoFillProfile*>::const_iterator iter = profiles.begin();
         iter != profiles.end(); ++iter)
      profiles_.push_back(**iter);

    // Make local copy of |creditCards|.
    for (std::vector<CreditCard*>::const_iterator iter = creditCards.begin();
         iter != creditCards.end(); ++iter)
      creditCards_.push_back(**iter);
  }
}

- (BOOL)isProfileRow:(NSInteger)row {
  if (row > 0 && static_cast<size_t>(row) <= profiles_.size())
    return YES;
  return NO;
}

- (BOOL)isProfileGroupRow:(NSInteger)row {
  if (row == 0)
    return YES;
  return NO;
}

- (BOOL)isCreditCardRow:(NSInteger)row {
  if (row > 0 &&
      static_cast<size_t>(row) >= profiles_.size() + 2 &&
      static_cast<size_t>(row) <= profiles_.size() + creditCards_.size() + 1)
    return YES;
  return NO;
}

- (BOOL)isCreditCardGroupRow:(NSInteger)row {
  if (row > 0 && static_cast<size_t>(row) == profiles_.size() + 1)
    return YES;
  return NO;
}

- (size_t)profileIndexFromRow:(NSInteger)row {
  DCHECK([self isProfileRow:row]);
  return static_cast<size_t>(row) - 1;
}

- (size_t)creditCardIndexFromRow:(NSInteger)row {
  DCHECK([self isCreditCardRow:row]);
  return static_cast<size_t>(row) - (profiles_.size() + 2);
}

- (NSInteger)rowFromProfileIndex:(size_t)i {
  return 1 + i;
}

- (NSInteger)rowFromCreditCardIndex:(size_t)i {
  return 1 + profiles_.size() + 1 + i;
}

@end

// An NSValueTransformer subclass for use in validation of empty data entry
// fields.  Transforms a nil or empty string into a warning image.  This data
// transformer is used in the address and credit card sheets for empty label
// strings.
@interface MissingAlertTransformer : NSValueTransformer {
}
@end

@implementation MissingAlertTransformer
+ (Class)transformedValueClass {
  return [NSImage class];
}

+ (BOOL)allowsReverseTransformation {
  return NO;
}

- (id)transformedValue:(id)string {
  NSImage* image = nil;
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  if (string == nil || [string length] == 0) {
    image = rb.GetNSImageNamed(IDR_INPUT_ALERT);
    DCHECK(image);
    return image;
  }

  if (!image) {
    image = rb.GetNSImageNamed(IDR_INPUT_GOOD);
    DCHECK(image);
    return image;
  }

  return nil;
}

@end

// An NSValueTransformer subclass for use in validation of phone number
// fields.  Transforms an invalid phone number string into a warning image.
// This data transformer is used in the credit card sheet for invalid phone and
// fax numbers.
@interface InvalidPhoneTransformer : NSValueTransformer {
}
@end

@implementation InvalidPhoneTransformer
+ (Class)transformedValueClass {
  return [NSImage class];
}

+ (BOOL)allowsReverseTransformation {
  return NO;
}

- (id)transformedValue:(id)string {
  NSImage* image = nil;
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();

  // We display no validation icon when input has not yet been entered.
  if (string == nil || [string length] == 0)
    return nil;

  // If we have input then display alert icon if we have an invalid number.
  if (string != nil && [string length] != 0) {
    // TODO(dhollowa): Using SetInfo() call to validate phone number.  Should
    // have explicit validation method.  More robust validation is needed as
    // well eventually.
    AutoFillProfile profile(string16(), 0);
    profile.SetInfo(AutoFillType(PHONE_HOME_WHOLE_NUMBER),
                    base::SysNSStringToUTF16(string));
    if (profile.GetFieldText(AutoFillType(PHONE_HOME_WHOLE_NUMBER)).empty()) {
      image = rb.GetNSImageNamed(IDR_INPUT_ALERT);
      DCHECK(image);
      return image;
    }
  }

  // No alert icon, so must be valid input.
  if (!image) {
    image = rb.GetNSImageNamed(IDR_INPUT_GOOD);
    DCHECK(image);
    return image;
  }

  return nil;
}

@end
