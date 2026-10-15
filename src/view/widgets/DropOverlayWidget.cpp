#include "DropOverlayWidget.hpp"

#include <QColor>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QRect>

#include "view/theme/ThemeManager.hpp"

namespace view {

DropOverlayWidget::DropOverlayWidget(QWidget* parent)
    : QWidget(parent) {
    // Pointer and drag events pass through to the widgets beneath, so the overlay
    // never intercepts the drag or the drop (Req 2.8). This also makes it ineligible
    // as a drop target, which is exactly why it is safe as pure visual feedback.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    // Draw a translucent fill ourselves so the underlying content shows through.
    setAttribute(Qt::WA_NoSystemBackground, true);
    // Plain child widget parented to the central area (no window flags); starts hidden.
}

void DropOverlayWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    // Paint the "Drop to open" overlay in widget pixels so it always covers the whole
    // overlay area. Colors derive from the theme's highlight so the overlay stays
    // theme-aware, with sane fallbacks (Req 2.1).
    const QRect overlayRect = rect();
    const auto& nodeTheme = ThemeManager::instance().theme().node;
    QColor accentColor = nodeTheme.highlightBorderPen.color();

    if (false == accentColor.isValid()) {
        accentColor = QColor(66, 133, 244);
    }

    QColor fillColor = accentColor;
    fillColor.setAlpha(80);

    QColor borderColor = accentColor;
    borderColor.setAlpha(220);

    QColor textColor = nodeTheme.textColor.isValid() ? nodeTheme.textColor : QColor(Qt::white);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.fillRect(overlayRect, fillColor);

    QPen borderPen(borderColor);
    borderPen.setWidth(3);
    painter.setPen(borderPen);
    painter.drawRect(overlayRect.adjusted(1, 1, -2, -2));

    QFont overlayFont = painter.font();
    overlayFont.setBold(true);
    overlayFont.setPointSize(overlayFont.pointSize() + 6);
    painter.setFont(overlayFont);
    painter.setPen(textColor);
    painter.drawText(overlayRect, Qt::AlignCenter, QStringLiteral("Drop to open"));
}

}  // namespace view
