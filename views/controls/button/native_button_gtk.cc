// Copyright (c) 2009 The Chromium Authors. All rights reserved. Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#include <gtk/gtk.h>

#include "views/controls/button/native_button_gtk.h"

#include "base/logging.h"
#include "base/string_util.h"
#include "views/controls/button/checkbox.h"
#include "views/controls/button/native_button.h"
#include "views/controls/button/radio_button.h"
#include "views/controls/native/native_view_host_gtk.h"
#include "views/widget/widget.h"

namespace views {

NativeButtonGtk::NativeButtonGtk(NativeButton* native_button)
    : NativeControlGtk(),
      native_button_(native_button) {
  // Associates the actual GtkWidget with the native_button so the native_button
  // is the one considered as having the focus (not the wrapper) when the
  // GtkWidget is focused directly (with a click for example).
  set_focus_view(native_button);
}

NativeButtonGtk::~NativeButtonGtk() {
}

void NativeButtonGtk::UpdateLabel() {
  if (!native_view())
    return;

  gtk_button_set_label(GTK_BUTTON(native_view()),
                       WideToUTF8(native_button_->label()).c_str());
  preferred_size_ = gfx::Size();
}

void NativeButtonGtk::UpdateFont() {
  if (!native_view())
    return;

  NOTIMPLEMENTED();
  preferred_size_ = gfx::Size();
  // SendMessage(GetHWND(), WM_SETFONT,
  // reinterpret_cast<WPARAM>(native_button_->font().hfont()),
  // FALSE);
}

void NativeButtonGtk::UpdateEnabled() {
  if (!native_view())
    return;
  SetEnabled(native_button_->IsEnabled());
}

void NativeButtonGtk::UpdateDefault() {
  if (!native_view())
    return;
  NOTIMPLEMENTED();
}

View* NativeButtonGtk::GetView() {
  return this;
}

void NativeButtonGtk::SetFocus() {
  // Focus the associated widget.
  Focus();
}

bool NativeButtonGtk::UsesNativeLabel() const {
  return true;
}

bool NativeButtonGtk::UsesNativeRadioButtonGroup() const {
  return true;
}

gfx::NativeView NativeButtonGtk::GetTestingHandle() const {
  return native_view();
}

gfx::Size NativeButtonGtk::GetPreferredSize() {
  if (!native_view())
    return gfx::Size();

  if (preferred_size_.IsEmpty()) {
    GtkRequisition size_request = { 0, 0 };
    gtk_widget_size_request(native_view(), &size_request);
    preferred_size_.SetSize(size_request.width,
                            std::max(size_request.height, 29));
  }
  return preferred_size_;
}

void NativeButtonGtk::CreateNativeControl() {
  GtkWidget* widget = gtk_button_new();
  g_signal_connect(G_OBJECT(widget), "clicked",
                   G_CALLBACK(CallClicked), this);
  NativeControlCreated(widget);
}

void NativeButtonGtk::NativeControlCreated(GtkWidget* widget) {
  NativeControlGtk::NativeControlCreated(widget);

  UpdateFont();
  UpdateLabel();
  UpdateDefault();
}

// static
void NativeButtonGtk::CallClicked(GtkButton* widget, NativeButtonGtk* button) {
  button->OnClicked();
}

void NativeButtonGtk::OnClicked() {
  native_button_->ButtonPressed();
}

////////////////////////////////////////////////////////////////////////////////
// NativeCheckboxGtk
NativeCheckboxGtk::NativeCheckboxGtk(Checkbox* checkbox)
    : NativeButtonGtk(checkbox),
      deliver_click_event_(true) {
}

void NativeCheckboxGtk::SyncCheckState() {
  checkbox()->SetChecked(
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(native_view())));
}

Checkbox* NativeCheckboxGtk::checkbox() {
  return static_cast<Checkbox*>(native_button_);
}

void NativeCheckboxGtk::CreateNativeControl() {
  GtkWidget* widget = gtk_check_button_new();
  g_signal_connect(G_OBJECT(widget), "clicked",
                   G_CALLBACK(CallClicked), this);
  NativeControlCreated(widget);
}

void NativeCheckboxGtk::OnClicked() {
  // ignore event if the event is generated by
  // gtk_toggle_button_set_active below.
  if (deliver_click_event_) {
    SyncCheckState();
    NativeButtonGtk::OnClicked();
  }
}

void NativeCheckboxGtk::UpdateDefault() {
  if (!native_view())
    return;
  UpdateChecked();
}

void NativeCheckboxGtk::UpdateChecked() {
  if (!native_view())
    return;
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(native_view()))
      != checkbox()->checked()) {
    // gtk_toggle_button_set_active emites "clicked" signal, which
    // invokes OnClicked method above. deliver_click_event_ flag is used
    // to prevent such signal to invoke OnClicked callback.
    deliver_click_event_ = false;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(native_view()),
                                 checkbox()->checked());
    deliver_click_event_ = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
NativeRadioButtonGtk::NativeRadioButtonGtk(RadioButton* radio_button)
    : NativeCheckboxGtk(radio_button) {
}

NativeRadioButtonGtk::~NativeRadioButtonGtk() {
}

RadioButton* NativeRadioButtonGtk::radio_button() {
  return static_cast<RadioButton*>(native_button_);
}

////////////////////////////////////////////////////////////////////////////////
// NativeRadioButtonGtk, NativeCheckboxGtk overrides:

void NativeRadioButtonGtk::CreateNativeControl() {
  GtkWidget* widget = gtk_radio_button_new(NULL);
  g_signal_connect(G_OBJECT(widget), "clicked",
                   G_CALLBACK(CallClicked), this);
  g_signal_connect(G_OBJECT(widget), "toggled",
                   G_CALLBACK(CallToggled), this);
  NativeControlCreated(widget);
}

void NativeRadioButtonGtk::OnToggled() {
  SyncCheckState();
}

// static
void NativeRadioButtonGtk::CallToggled(GtkButton* widget,
                                       NativeRadioButtonGtk* button) {
  button->OnToggled();
}

////////////////////////////////////////////////////////////////////////////////
// NativeRadioButtonGtk, NativeButtonWrapper overrides:
void NativeRadioButtonGtk::SetGroupFrom(NativeButtonWrapper* wrapper) {
  NativeRadioButtonGtk* peer = static_cast<NativeRadioButtonGtk*>(wrapper);
  GSList* group =
      gtk_radio_button_get_group(GTK_RADIO_BUTTON(peer->native_view()));
  // A group object is managed by gtk framework. It's updated as a radio
  // button is added to, or removed.
  DCHECK(group);
  GtkRadioButton* this_radio_button = GTK_RADIO_BUTTON(native_view());
  if (!g_slist_find(group, this_radio_button))
    gtk_radio_button_set_group(this_radio_button, group);
}

////////////////////////////////////////////////////////////////////////////////
// NativeRadioButtonGtk, NativeControlGtk overrides:
void NativeRadioButtonGtk::ViewHierarchyChanged(bool is_add,
                                                View *parent, View *child) {
  NativeControlGtk::ViewHierarchyChanged(is_add, parent, child);

  // look for the same group and update
  if (is_add && child == this) {
    View* container = GetParent();
    while (container && container->GetParent())
      container = container->GetParent();
    if (container) {
      std::vector<View*> other;
      container->GetViewsWithGroup(native_button_->GetGroup(), &other);
      for (std::vector<View*>::iterator i = other.begin();
           i != other.end();
           ++i) {
        if (*i != native_button_) {
          if ((*i)->GetClassName() != RadioButton::kViewClassName) {
            NOTREACHED() << "radio-button has same group as other non "
                "radio-button views.";
            continue;
          }
          // Join the group
          NativeButtonWrapper* wrapper =
              static_cast<RadioButton*>(*i)->native_wrapper();
          SetGroupFrom(wrapper);
          break;
        }
      }
    }

    // Sync the state after setting the group because single radio button
    // is always active.
    SyncCheckState();
  }
}

////////////////////////////////////////////////////////////////////////////////
// NativeButtonWrapper

// static
int NativeButtonWrapper::GetFixedWidth() {
  // TODO(brettw) implement this properly.
  return 10;
}

// static
NativeButtonWrapper* NativeButtonWrapper::CreateNativeButtonWrapper(
    NativeButton* native_button) {
  return new NativeButtonGtk(native_button);
}

// static
NativeButtonWrapper* NativeButtonWrapper::CreateCheckboxWrapper(
    Checkbox* checkbox) {
  return new NativeCheckboxGtk(checkbox);
}

// static
NativeButtonWrapper* NativeButtonWrapper::CreateRadioButtonWrapper(
    RadioButton* radio_button) {
  return new NativeRadioButtonGtk(radio_button);
}

}  // namespace views
