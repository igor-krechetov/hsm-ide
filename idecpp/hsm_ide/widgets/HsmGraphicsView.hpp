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
    void wheelEvent(QWheelEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void setPanningMode(const bool enable);

private:
    QPoint mLastPanPoint;
    bool mPanning = false;
    bool mSpacePressed = false;
};

#endif // HSMGRAPHICSVIEW_HPP
