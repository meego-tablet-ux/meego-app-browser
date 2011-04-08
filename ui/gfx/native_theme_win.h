// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// A wrapper class for working with custom XP/Vista themes provided in
// uxtheme.dll.  This is a singleton class that can be grabbed using
// NativeThemeWin::instance().
// For more information on visual style parts and states, see:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/shellcc/platform/commctls/userex/topics/partsandstates.asp

#ifndef UI_GFX_NATIVE_THEME_WIN_H_
#define UI_GFX_NATIVE_THEME_WIN_H_
#pragma once

#include "ui/gfx/native_theme.h"
#include "ui/gfx/size.h"
#include <windows.h>
#include <uxtheme.h>
#include "base/basictypes.h"
#include "third_party/skia/include/core/SkColor.h"

class SkCanvas;

namespace gfx {

// Windows implementation of native theme class.
//
// At the moment, this class in in transition from an older API that consists
// of several PaintXXX methods to an API, inherited from the NativeTheme base
// class, that consists of a single Paint() method with a argument to indicate
// what kind of part to paint.
class NativeThemeWin : public NativeTheme {
 public:
  enum ThemeName {
    BUTTON,
    LIST,
    MENU,
    MENULIST,
    SCROLLBAR,
    STATUS,
    TAB,
    TEXTFIELD,
    TRACKBAR,
    WINDOW,
    PROGRESS,
    SPIN,
    LAST
  };

  bool IsThemingActive() const;

  HRESULT GetThemePartSize(ThemeName themeName,
                           HDC hdc,
                           int part_id,
                           int state_id,
                           RECT* rect,
                           int ts,
                           SIZE* size) const;

  HRESULT GetThemeColor(ThemeName theme,
                        int part_id,
                        int state_id,
                        int prop_id,
                        SkColor* color) const;

  // Get the theme color if theming is enabled.  If theming is unsupported
  // for this part, use Win32's GetSysColor to find the color specified
  // by default_sys_color.
  SkColor GetThemeColorWithDefault(ThemeName theme,
                                   int part_id,
                                   int state_id,
                                   int prop_id,
                                   int default_sys_color) const;

  HRESULT GetThemeInt(ThemeName theme,
                      int part_id,
                      int state_id,
                      int prop_id,
                      int *result) const;

  // Get the thickness of the border associated with the specified theme,
  // defaulting to GetSystemMetrics edge size if themes are disabled.
  // In Classic Windows, borders are typically 2px; on XP+, they are 1px.
  Size GetThemeBorderSize(ThemeName theme) const;

  // Disables all theming for top-level windows in the entire process, from
  // when this method is called until the process exits.  All the other
  // methods in this class will continue to work, but their output will ignore
  // the user's theme. This is meant for use when running tests that require
  // consistent visual results.
  void DisableTheming() const;

  // Closes cached theme handles so we can unload the DLL or update our UI
  // for a theme change.
  void CloseHandles() const;

  // Returns true if classic theme is in use.
  bool IsClassicTheme(ThemeName name) const;

  // Gets our singleton instance.
  static const NativeThemeWin* instance();

  // The PaintXXX methods below this point should be private or be deleted,
  // but remain public while NativeThemeWin is transitioned over to use the
  // single Paint() entry point.  Do not make new calls to these methods.

  // This enumeration is used within PaintMenuArrow in order to indicate the
  // direction the menu arrow should point to.
  enum MenuArrowDirection {
    LEFT_POINTING_ARROW,
    RIGHT_POINTING_ARROW
  };

  enum ControlState {
    CONTROL_NORMAL,
    CONTROL_HIGHLIGHTED,
    CONTROL_DISABLED
  };

  typedef HRESULT (WINAPI* DrawThemeBackgroundPtr)(HANDLE theme,
                                                   HDC hdc,
                                                   int part_id,
                                                   int state_id,
                                                   const RECT* rect,
                                                   const RECT* clip_rect);
  typedef HRESULT (WINAPI* DrawThemeBackgroundExPtr)(HANDLE theme,
                                                     HDC hdc,
                                                     int part_id,
                                                     int state_id,
                                                     const RECT* rect,
                                                     const DTBGOPTS* opts);
  typedef HRESULT (WINAPI* GetThemeColorPtr)(HANDLE hTheme,
                                             int part_id,
                                             int state_id,
                                             int prop_id,
                                             COLORREF* color);
  typedef HRESULT (WINAPI* GetThemeContentRectPtr)(HANDLE hTheme,
                                                   HDC hdc,
                                                   int part_id,
                                                   int state_id,
                                                   const RECT* rect,
                                                   RECT* content_rect);
  typedef HRESULT (WINAPI* GetThemePartSizePtr)(HANDLE hTheme,
                                                HDC hdc,
                                                int part_id,
                                                int state_id,
                                                RECT* rect,
                                                int ts,
                                                SIZE* size);
  typedef HANDLE (WINAPI* OpenThemeDataPtr)(HWND window,
                                            LPCWSTR class_list);
  typedef HRESULT (WINAPI* CloseThemeDataPtr)(HANDLE theme);

  typedef void (WINAPI* SetThemeAppPropertiesPtr) (DWORD flags);
  typedef BOOL (WINAPI* IsThemeActivePtr)();
  typedef HRESULT (WINAPI* GetThemeIntPtr)(HANDLE hTheme,
                                           int part_id,
                                           int state_id,
                                           int prop_id,
                                           int *value);

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintButton(HDC hdc,
                      int part_id,
                      int state_id,
                      int classic_state,
                      RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintDialogBackground(HDC dc, bool active, RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintListBackground(HDC dc, bool enabled, RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  // |arrow_direction| determines whether the arrow is pointing to the left or
  // to the right. In RTL locales, sub-menus open from right to left and
  // therefore the menu arrow should point to the left and not to the right.
  HRESULT PaintMenuArrow(ThemeName theme,
                         HDC hdc,
                         int part_id,
                         int state_id,
                         RECT* rect,
                         MenuArrowDirection arrow_direction,
                         ControlState state) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintMenuBackground(ThemeName theme,
                              HDC hdc,
                              int part_id,
                              int state_id,
                              RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintMenuCheck(ThemeName theme,
                         HDC hdc,
                         int part_id,
                         int state_id,
                         RECT* rect,
                         ControlState state) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintMenuCheckBackground(ThemeName theme,
                                   HDC hdc,
                                   int part_id,
                                   int state_id,
                                   RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintMenuGutter(HDC hdc,
                          int part_id,
                          int state_id,
                          RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintMenuItemBackground(ThemeName theme,
                                  HDC hdc,
                                  int part_id,
                                  int state_id,
                                  bool selected,
                                  RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintMenuList(HDC hdc,
                        int part_id,
                        int state_id,
                        int classic_state,
                        RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintMenuSeparator(HDC hdc,
                             int part_id,
                             int state_id,
                             RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  // Paints a scrollbar arrow.  |classic_state| should have the appropriate
  // classic part number ORed in already.
  HRESULT PaintScrollbarArrow(HDC hdc,
                              int state_id,
                              int classic_state,
                              RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  // Paints a scrollbar track section.  |align_rect| is only used in classic
  // mode, and makes sure the checkerboard pattern in |target_rect| is aligned
  // with one presumed to be in |align_rect|.
  HRESULT PaintScrollbarTrack(HDC hdc,
                              int part_id,
                              int state_id,
                              int classic_state,
                              RECT* target_rect,
                              RECT* align_rect,
                              SkCanvas* canvas) const;

  // This method is deprecated and will be removed in the near future.
  // Paints a scrollbar thumb or gripper.
  HRESULT PaintScrollbarThumb(HDC hdc,
                              int part_id,
                              int state_id,
                              int classic_state,
                              RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintSpinButton(HDC hdc,
                          int part_id,
                          int state_id,
                          int classic_state,
                          RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintStatusGripper(HDC hdc,
                             int part_id,
                             int state_id,
                             int classic_state,
                             RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintTabPanelBackground(HDC dc, RECT* rect) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintTextField(HDC hdc,
                         int part_id,
                         int state_id,
                         int classic_state,
                         RECT* rect,
                         COLORREF color,
                         bool fill_content_area,
                         bool draw_edges) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintTrackbar(HDC hdc,
                        int part_id,
                        int state_id,
                        int classic_state,
                        RECT* rect,
                        SkCanvas* canvas) const;

  // This method is deprecated and will be removed in the near future.
  HRESULT PaintProgressBar(HDC hdc,
                           RECT* bar_rect,
                           RECT* value_rect,
                           bool determinate,
                           double animated_seconds,
                           SkCanvas* canvas) const;

 private:
  NativeThemeWin();
  ~NativeThemeWin();

  // NativeTheme Implementation:
  virtual gfx::Size GetPartSize(Part part) const;
  virtual void Paint(skia::PlatformCanvas* canvas,
                     Part part,
                     State state,
                     const gfx::Rect& rect,
                     const ExtraParams& extra) const;

  // Paints a scrollbar arrow.  |classic_state| should have the appropriate
  // classic part number ORed in already.
  HRESULT PaintScrollbarArrow(HDC hdc,
                              Part direction,
                              State state,
                              const gfx::Rect& rect) const;

  HRESULT PaintScrollbarThumb(HDC hdc,
                              Part direction,
                              State state,
                              const gfx::Rect& rect) const;

  HRESULT PaintPushButton(HDC hdc,
                          Part part,
                          State state,
                          const gfx::Rect& rect,
                          const ButtonExtraParams& extra) const;

  HRESULT PaintRadioButton(HDC hdc,
                           Part part,
                           State state,
                           const gfx::Rect& rect,
                           const ButtonExtraParams& extra) const;

  HRESULT PaintCheckbox(HDC hdc,
                        Part part,
                        State state,
                        const gfx::Rect& rect,
                        const ButtonExtraParams& extra) const;

  // Get the windows theme name that goes with the part.
  static ThemeName GetThemeName(Part part);

  // Get the windows theme part id that goes with the part.
  static int GetWindowsPart(Part part);

  HRESULT PaintFrameControl(HDC hdc,
                            RECT* rect,
                            UINT type,
                            UINT state,
                            ControlState control_state) const;

  // Returns a handle to the theme data.
  HANDLE GetThemeHandle(ThemeName theme_name) const;

  // Function pointers into uxtheme.dll.
  DrawThemeBackgroundPtr draw_theme_;
  DrawThemeBackgroundExPtr draw_theme_ex_;
  GetThemeColorPtr get_theme_color_;
  GetThemeContentRectPtr get_theme_content_rect_;
  GetThemePartSizePtr get_theme_part_size_;
  OpenThemeDataPtr open_theme_;
  CloseThemeDataPtr close_theme_;
  SetThemeAppPropertiesPtr set_theme_properties_;
  IsThemeActivePtr is_theme_active_;
  GetThemeIntPtr get_theme_int_;

  // Handle to uxtheme.dll.
  HMODULE theme_dll_;

  // A cache of open theme handles.
  mutable HANDLE theme_handles_[LAST];

  DISALLOW_COPY_AND_ASSIGN(NativeThemeWin);
};

}  // namespace gfx

#endif  // UI_GFX_NATIVE_THEME_WIN_H_
