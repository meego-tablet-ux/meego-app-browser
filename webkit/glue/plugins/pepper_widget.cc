// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/plugins/pepper_widget.h"

#include "base/logging.h"
#include "ppapi/c/dev/ppb_widget_dev.h"
#include "ppapi/c/dev/ppp_widget_dev.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_errors.h"
#include "webkit/glue/plugins/pepper_common.h"
#include "webkit/glue/plugins/pepper_image_data.h"
#include "webkit/glue/plugins/pepper_plugin_instance.h"
#include "webkit/glue/plugins/pepper_plugin_module.h"

namespace pepper {

namespace {

PP_Bool IsWidget(PP_Resource resource) {
  return BoolToPPBool(!!Resource::GetAs<Widget>(resource));
}

PP_Bool Paint(PP_Resource resource, const PP_Rect* rect, PP_Resource image_id) {
  scoped_refptr<Widget> widget(Resource::GetAs<Widget>(resource));
  if (!widget)
    return PP_FALSE;

  scoped_refptr<ImageData> image(Resource::GetAs<ImageData>(image_id));
  if (!image)
    return PP_FALSE;

  return BoolToPPBool(widget->Paint(rect, image));
}

PP_Bool HandleEvent(PP_Resource resource, const PP_InputEvent* event) {
  scoped_refptr<Widget> widget(Resource::GetAs<Widget>(resource));
  return BoolToPPBool(widget && widget->HandleEvent(event));
}

PP_Bool GetLocation(PP_Resource resource, PP_Rect* location) {
  scoped_refptr<Widget> widget(Resource::GetAs<Widget>(resource));
  return BoolToPPBool(widget && widget->GetLocation(location));
}

void SetLocation(PP_Resource resource, const PP_Rect* location) {
  scoped_refptr<Widget> widget(Resource::GetAs<Widget>(resource));
  if (widget)
    widget->SetLocation(location);
}

const PPB_Widget_Dev ppb_widget = {
  &IsWidget,
  &Paint,
  &HandleEvent,
  &GetLocation,
  &SetLocation,
};

}  // namespace

Widget::Widget(PluginInstance* instance)
    : Resource(instance->module()),
      instance_(instance) {
}

Widget::~Widget() {
}

// static
const PPB_Widget_Dev* Widget::GetInterface() {
  return &ppb_widget;
}

bool Widget::GetLocation(PP_Rect* location) {
  *location = location_;
  return true;
}

void Widget::SetLocation(const PP_Rect* location) {
  location_ = *location;
  SetLocationInternal(location);
}

void Widget::Invalidate(const PP_Rect* dirty) {
  const PPP_Widget_Dev* widget = static_cast<const PPP_Widget_Dev*>(
      module()->GetPluginInterface(PPP_WIDGET_DEV_INTERFACE));
  if (!widget)
    return;
  ScopedResourceId resource(this);
  widget->Invalidate(instance_->pp_instance(), resource.id, dirty);
}

}  // namespace pepper
