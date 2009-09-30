// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/tools/test_shell/text_input_controller.h"

#include "base/string_util.h"
#include "webkit/api/public/WebFrame.h"
#include "webkit/api/public/WebRange.h"
#include "webkit/api/public/WebString.h"
#include "webkit/glue/webview.h"
#include "webkit/tools/test_shell/test_shell.h"

using WebKit::WebFrame;
using WebKit::WebRange;
using WebKit::WebString;

TestShell* TextInputController::shell_ = NULL;

TextInputController::TextInputController(TestShell* shell) {
  // Set static shell_ variable. Be careful not to assign shell_ to new
  // windows which are temporary.
  if (NULL == shell_)
    shell_ = shell;

  BindMethod("insertText", &TextInputController::insertText);
  BindMethod("doCommand", &TextInputController::doCommand);
  BindMethod("setMarkedText", &TextInputController::setMarkedText);
  BindMethod("unmarkText", &TextInputController::unmarkText);
  BindMethod("hasMarkedText", &TextInputController::hasMarkedText);
  BindMethod("conversationIdentifier", &TextInputController::conversationIdentifier);
  BindMethod("substringFromRange", &TextInputController::substringFromRange);
  BindMethod("attributedSubstringFromRange", &TextInputController::attributedSubstringFromRange);
  BindMethod("markedRange", &TextInputController::markedRange);
  BindMethod("selectedRange", &TextInputController::selectedRange);
  BindMethod("firstRectForCharacterRange", &TextInputController::firstRectForCharacterRange);
  BindMethod("characterIndexForPoint", &TextInputController::characterIndexForPoint);
  BindMethod("validAttributesForMarkedText", &TextInputController::validAttributesForMarkedText);
  BindMethod("makeAttributedString", &TextInputController::makeAttributedString);
}

// static
WebFrame* TextInputController::GetMainFrame() {
  return shell_->webView()->mainFrame();
}

void TextInputController::insertText(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  if (args.size() >= 1 && args[0].isString()) {
    if (main_frame->hasMarkedText()) {
      main_frame->unmarkText();
      main_frame->replaceSelection(WebString());
    }
    main_frame->insertText(WebString::fromUTF8(args[0].ToString()));
  }
}

void TextInputController::doCommand(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  if (args.size() >= 1 && args[0].isString())
    main_frame->executeCommand(WebString::fromUTF8(args[0].ToString()));
}

void TextInputController::setMarkedText(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  if (args.size() >= 3 && args[0].isString()
      && args[1].isNumber() && args[2].isNumber()) {
    main_frame->setMarkedText(WebString::fromUTF8(args[0].ToString()),
                              args[1].ToInt32(),
                              args[2].ToInt32());
  }
}

void TextInputController::unmarkText(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  main_frame->unmarkText();
}

void TextInputController::hasMarkedText(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  result->Set(main_frame->hasMarkedText());
}

void TextInputController::conversationIdentifier(
    const CppArgumentList& args, CppVariant* result) {
  NOTIMPLEMENTED();
  result->SetNull();
}

void TextInputController::substringFromRange(
    const CppArgumentList& args, CppVariant* result) {
  NOTIMPLEMENTED();
  result->SetNull();
}

void TextInputController::attributedSubstringFromRange(
    const CppArgumentList& args, CppVariant* result) {
  NOTIMPLEMENTED();
  result->SetNull();
}

void TextInputController::markedRange(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  WebRange range = main_frame->markedRange();

  std::string range_str;
  SStringPrintf(&range_str, "%d,%d", range.startOffset(), range.endOffset());
  result->Set(range_str);
}

void TextInputController::selectedRange(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  WebRange range = main_frame->selectionRange();

  std::string range_str;
  SStringPrintf(&range_str, "%d,%d", range.startOffset(), range.endOffset());
  result->Set(range_str);
}

void TextInputController::firstRectForCharacterRange(
    const CppArgumentList& args, CppVariant* result) {
  NOTIMPLEMENTED();
  result->SetNull();
}

void TextInputController::characterIndexForPoint(
    const CppArgumentList& args, CppVariant* result) {
  NOTIMPLEMENTED();
  result->SetNull();
}

void TextInputController::validAttributesForMarkedText(
    const CppArgumentList& args, CppVariant* result) {
  result->SetNull();

  WebFrame* main_frame = GetMainFrame();
  if (!main_frame)
    return;

  result->Set("NSUnderline,NSUnderlineColor,NSMarkedClauseSegment,"
              "NSTextInputReplacementRangeAttributeName");
}

void TextInputController::makeAttributedString(
    const CppArgumentList& args, CppVariant* result) {
  NOTIMPLEMENTED();
  result->SetNull();
}
