#include "AutoGroupItem.hpp"

#include <QChildEvent>
#include <QGraphicsItem>
#include <QPainter>
#include <QTimer>

#include "ui/theme/ThemeManager.hpp"

AutoGroupItem::AutoGroupItem(QGraphicsItem* parent)
    : QGraphicsObject(parent) {
    setFlags(ItemIsMovable | ItemSendsGeometryChanges);
}

void AutoGroupItem::makeMovable(const bool enable) {
    setFlag(QGraphicsItem::ItemIsMovable, enable);
    setFlag(QGraphicsItem::ItemIsSelectable, enable);
}

QRectF AutoGroupItem::boundingRect() const {
    QRectF newRect = childrenBoundingRect();

    if (mLastRect != newRect) {
        mLastRect = childrenBoundingRect();
        QTimer::singleShot(0, this, SLOT(relayout()));
        emit geometryChanged();
    }

    return newRect;
}

void AutoGroupItem::paint(QPainter* p, const QStyleOptionGraphicsItem*, QWidget*) {
    // Optional: draw bounding box (debug)
#ifdef DEBUG_RENDERING
    const auto& theme = ThemeManager::instance().theme();
    p->setPen(theme.grid.majorLinePen);
    p->drawRect(childrenBoundingRect());
#endif
}

QVariant AutoGroupItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemChildAddedChange || change == ItemChildRemovedChange) {
        QTimer::singleShot(0, this, SLOT(relayout()));
    }

    return QGraphicsObject::itemChange(change, value);
}

void AutoGroupItem::childEvent(QChildEvent* event) {
    if (event->type() == QEvent::ChildRemoved || event->type() == QEvent::ChildAdded) {
        relayout();
    }

    QGraphicsObject::childEvent(event);
}

void AutoGroupItem::relayout() {
    QList<QGraphicsItem*> validChildren;
    qreal maxWidth = 0;

    // 1. Collect visible children and find maximum width
    for (QGraphicsItem* item : childItems()) {
        if (!item->isVisible()) {
            continue;
        }

        validChildren.append(item);
        maxWidth = qMax(maxWidth, item->boundingRect().width());
    }

    if (validChildren.isEmpty()) {
        return;
    }

    // 2. Adjust X to horizontally center each child based on maxWidth
    for (QGraphicsItem* item : validChildren) {
        const qreal y = item->pos().y();
        const qreal childWidth = item->boundingRect().width();

        if (childWidth == maxWidth) {
            item->setPos(0, y);
        } else {
            // Center X: shift so that each item is centered relative to max width
            qreal x = (maxWidth - childWidth) / 2.0;

            item->setPos(x, y);
        }
    }
}
