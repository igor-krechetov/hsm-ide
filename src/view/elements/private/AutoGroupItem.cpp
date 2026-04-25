#include "AutoGroupItem.hpp"

#include <QChildEvent>
#include <QGraphicsItem>
#include <QPainter>
#include <QTimer>

#include "HsmStateTextItem.hpp"
#include "view/elements/ElementTypeIds.hpp"
#include "view/theme/ThemeManager.hpp"

namespace view {

AutoGroupItem::AutoGroupItem(QGraphicsItem* parent)
    : QGraphicsObject(parent) {
    setFlags(ItemIsMovable | ItemSendsGeometryChanges);
}

void AutoGroupItem::setDirection(const AutoLayoutDirection direction) {
    mLayoutDirection = direction;
    relayout();
}

void AutoGroupItem::makeMovable(const bool enable) {
    setFlag(QGraphicsItem::ItemIsMovable, enable);
    setFlag(QGraphicsItem::ItemIsSelectable, enable);
}

QRectF AutoGroupItem::boundingRect() const {
    QRectF newRect = childrenBoundingRect();


    return newRect;
}

void AutoGroupItem::addItem(QGraphicsItem* item) {
    if (nullptr != item) {
        prepareGeometryChange();
        item->setParentItem(this);

        if (nullptr != item && item->type() == view::ELEMENT_TYPE_STATE_TEXT) {
            HsmStateTextItem* stateTextItem = dynamic_cast<HsmStateTextItem*>(item);

            connect(stateTextItem, &HsmStateTextItem::editingFinished, this, &AutoGroupItem::relayout);
        }
    }

    update();
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
    switch (mLayoutDirection) {
        case AutoLayoutDirection::HORIZONTAL:
            relayoutHorizontal();
            break;
        case AutoLayoutDirection::VERTICAL:
            relayoutVertical();
            break;
    }
}

void AutoGroupItem::relayoutVertical() {
    QMap<int, QGraphicsItem*> validChildren;
    qreal maxWidth = 0;

    // 1. Collect visible children and find maximum width
    for (QGraphicsItem* item : childItems()) {
        if (!item->isVisible()) {
            continue;
        }

        validChildren.insert(item->data(static_cast<int>(AutoLayoutDirection::VERTICAL)).toInt(), item);
        maxWidth = qMax(maxWidth, item->boundingRect().width());
    }

    if (validChildren.isEmpty() == false) {
        // 2. Adjust X to horizontally center each child based on maxWidth
        int offsetY = 0;

        for (int index = 0; index < validChildren.size(); ++index) {
            QGraphicsItem* item = validChildren.value(index);
            const qreal y = item->pos().y();
            const qreal childWidth = item->boundingRect().width();

            if (childWidth == maxWidth) {
                item->setPos(0, offsetY);
            } else {
                // Center X: shift so that each item is centered relative to max width
                qreal x = (maxWidth - childWidth) / 2.0;

                item->setPos(x, offsetY);
            }

            offsetY += item->boundingRect().height() + 3;  // Add some spacing between items
        }
    }
}

void AutoGroupItem::relayoutHorizontal() {
    QMap<int, QGraphicsItem*> validChildren;
    qreal maxHeight = 0;

    // 1. Collect visible children and find maximum height
    for (QGraphicsItem* item : childItems()) {
        if (!item->isVisible()) {
            continue;
        }

        validChildren.insert(item->data(static_cast<int>(AutoLayoutDirection::HORIZONTAL)).toInt(), item);
        maxHeight = qMax(maxHeight, item->boundingRect().height());
    }

    if (validChildren.isEmpty() == false) {
        int offsetX = 0;

        // 2. Adjust Y to vertically center each child based on maxHeight
        for (int index = 0; index < validChildren.size(); ++index) {
            QGraphicsItem* item = validChildren.value(index);
            const qreal childHeight = item->boundingRect().height();

            if (childHeight == maxHeight) {
                item->setPos(offsetX, 0);
            } else {
                // Center Y: shift so that each item is centered relative to max height
                qreal y = (maxHeight - childHeight) / 2.0;

                item->setPos(offsetX, y);
            }

            offsetX += item->boundingRect().width() + 3;  // Add some spacing between items
        }
    }
}

int AutoGroupItem::type() const {
    return view::ELEMENT_TYPE_AUTOGROUP;
}

};  // namespace view