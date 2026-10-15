#ifndef DROPOVERLAYWIDGET_HPP
#define DROPOVERLAYWIDGET_HPP

#include <QWidget>

class QPaintEvent;

namespace view {

// Translucent, frameless, mouse-transparent overlay that paints a centered
// "Drop to open" hint over its parent. Shown while a file drag carrying at
// least one .scxml file is over the central area; mouse/drag transparent so it
// never intercepts the drop (Req 2.1, 2.8).
class DropOverlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit DropOverlayWidget(QWidget* parent);

protected:
    void paintEvent(QPaintEvent* event) override;
};

}  // namespace view

#endif  // DROPOVERLAYWIDGET_HPP
