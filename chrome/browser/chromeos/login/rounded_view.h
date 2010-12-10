#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_VIEW_FILTER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_VIEW_FILTER_H_

#include "gfx/canvas.h"
#include "gfx/canvas_skia.h"
#include "gfx/rect.h"

namespace chromeos {

namespace rounded_view {

// Corner radius of the RoundedView.
const SkScalar kCornerRadius = SkIntToScalar(5);

// Stroke width to be used by the RoundedView.
const SkScalar kStrokeWidth = SkIntToScalar(1);

// Color of the inner frame of the RoundedView.
const SkColor kInnerFrameColor = SK_ColorWHITE;

// Color of the outer frame of the RoundedView.
const SkColor kOuterFrameColor = 0xFF555555;

}  // namespace rounded_view

// Class that sets up the round rectangle as a clip region of the view.
// |C| - class inherited from |views::View|.
template<typename C>
class RoundedView: public C {
 public:
  // Empty constructor.
  RoundedView() {}

  // One argument constructor.
  template<typename D>
  explicit RoundedView(const D &value) : C(value) {}

  // Overrides views::View.
  virtual void ProcessPaint(gfx::Canvas* canvas);

 protected:
  // Returns the path that will be used for a clip.
  virtual SkPath GetClipPath() const;

  // Returns maximal rectangle in the view.
  virtual SkRect GetViewRect() const;

  // Draws custom frame for the view.
  virtual void DrawFrame(gfx::Canvas* canvas);
};

// RoundedView implementation.

template <typename C>
void RoundedView<C>::ProcessPaint(gfx::Canvas* canvas) {
  // Setup clip region.
  canvas->Save();
  canvas->AsCanvasSkia()->clipPath(GetClipPath());
  // Do original painting.
  C::ProcessPaint(canvas);
  canvas->Restore();
  // Add frame.
  DrawFrame(canvas);
}

template <typename C>
SkPath RoundedView<C>::GetClipPath() const {
  SkPath round_view;
  SkRect view_rect = GetViewRect();
  view_rect.inset(2 * rounded_view::kStrokeWidth,
                  2 * rounded_view::kStrokeWidth);
  // 3 is used instead of 2 to avoid empty points between the clip and
  // the frame.
  round_view.addRoundRect(
      view_rect,
      rounded_view::kCornerRadius - 3 * rounded_view::kStrokeWidth,
      rounded_view::kCornerRadius - 3 * rounded_view::kStrokeWidth);

  return round_view;
}

template <typename C>
SkRect RoundedView<C>::GetViewRect() const {
  gfx::Rect bounds = RoundedView<C>::GetLocalBounds(false);
  SkRect view_rect;
  view_rect.iset(bounds.x(), bounds.y(),
                 bounds.x() + bounds.width(),
                 bounds.y() + bounds.height());
  return view_rect;
}

template <typename C>
void RoundedView<C>::DrawFrame(gfx::Canvas* canvas) {
  SkPaint paint;
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeWidth(rounded_view::kStrokeWidth);
  paint.setAntiAlias(true);
  SkRect view_rect = GetViewRect();

  // Used to make nested rounded rects look better.
  const SkScalar kOriginShift = 1.0;
  const SkScalar kDelta = 0.3;

  // Draw inner frame.
  view_rect.fLeft -= kOriginShift;
  view_rect.fTop -= kOriginShift;
  view_rect.inset(rounded_view::kStrokeWidth, rounded_view::kStrokeWidth);
  paint.setColor(rounded_view::kInnerFrameColor);
  canvas->AsCanvasSkia()->
      drawRoundRect(view_rect,
                    rounded_view::kCornerRadius - rounded_view::kStrokeWidth,
                    rounded_view::kCornerRadius - rounded_view::kStrokeWidth,
                    paint);

  // Draw outer frame.
  view_rect.fLeft -= kDelta;
  view_rect.fTop -= kDelta;
  view_rect.offset(rounded_view::kStrokeWidth - kDelta,
                   rounded_view::kStrokeWidth - kDelta);
  paint.setColor(rounded_view::kOuterFrameColor);
  canvas->AsCanvasSkia()->drawRoundRect(view_rect, rounded_view::kCornerRadius,
                                        rounded_view::kCornerRadius, paint);
}

}

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_VIEW_FILTER_H_
