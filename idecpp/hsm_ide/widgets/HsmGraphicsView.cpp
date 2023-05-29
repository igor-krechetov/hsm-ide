#include "HsmGraphicsView.hpp"
#include "elements/HsmElementsFactory.hpp"
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QMimeData>

HsmGraphicsView::HsmGraphicsView(QWidget* parent)
    : QGraphicsView(parent)
{
    setAcceptDrops(true);
}

void HsmGraphicsView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("hsm/element")) {
        qDebug() << "dragEnterEvent:" << event->source() << "," << event->mimeData()->formats();
        qDebug() << event->mimeData()->data("hsm/element");
        event->setDropAction(Qt::CopyAction);
        event->accept();
    }
}

void HsmGraphicsView::dragMoveEvent(QDragMoveEvent* event)
{
    event->setDropAction(Qt::CopyAction);
    event->accept();
}

void HsmGraphicsView::dropEvent(QDropEvent* event)
{
    qDebug() << "dropEvent:" << event->position();
    event->setDropAction(Qt::CopyAction);
    event->accept();

    // TODO: remove local var scenePos and check mapping
    // QPointF scenePos = mapToScene(event->position());
    // QPointF scenePos = mapToScene(event->position());
    QPointF scenePos = event->position();
    HsmElement* e1 = HsmElementsFactory::createElement(event->mimeData()->data("hsm/element").data());

    e1->setPos(scenePos);
    scene()->addItem(e1);
}

// void HsmGraphicsView::mousePressEvent(QMouseEvent* event)
// {
//     if (event->buttons() == Qt::RightButton) {
//         // Right button pressed, perform custom action
//         // ...
//     }
//     else {
//         QGraphicsView::mousePressEvent(event);
//     }
// }
