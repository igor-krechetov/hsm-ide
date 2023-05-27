#ifndef HSMGRAPHICSVIEW_HPP
#define HSMGRAPHICSVIEW_HPP

#include <QGraphicsView>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

class HsmGraphicsView : public QGraphicsView {
public:
    HsmGraphicsView(QWidget* parent);
    virtual ~HsmGraphicsView() = default;

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    // void mousePressEvent(QMouseEvent* event) override;
};

#endif // HSMGRAPHICSVIEW_HPP
