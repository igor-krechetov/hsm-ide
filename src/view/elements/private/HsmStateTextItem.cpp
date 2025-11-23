#include "HsmStateTextItem.hpp"

#include <QFocusEvent>
#include <QGraphicsScene>
#include <QKeyEvent>

namespace view {

HsmStateTextItem::HsmStateTextItem(QGraphicsItem* parent)
    : QGraphicsTextItem(parent) {
    QFont font;
    font.setBold(true);

    setFont(font);

    setDefaultTextColor(QColor("#1A1A1A"));
    // setZValue(10);
    setTextInteractionFlags(Qt::TextEditorInteraction);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

void HsmStateTextItem::makeMovable(const bool enable) {
    setFlag(QGraphicsItem::ItemIsMovable, enable);
    setFlag(QGraphicsItem::ItemIsSelectable, enable);
}

void HsmStateTextItem::focusInEvent(QFocusEvent* event) {
    QGraphicsTextItem::focusInEvent(event);

    if (scene()) {
        scene()->clearSelection();
    }

    if (parentItem()) {
        parentItem()->setSelected(true);
    }

    mOriginalText = toPlainText();
}

void HsmStateTextItem::focusOutEvent(QFocusEvent* event) {
    QGraphicsTextItem::focusOutEvent(event);

    if (toPlainText() != mOriginalText) {
        emit editingFinished();
    }
}

void HsmStateTextItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        emit editingFinished();
        event->ignore();  // Prevent newline
        clearFocus();
        return;
    }

    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete || event->text() != "\n") {
        QGraphicsTextItem::keyPressEvent(event);
    }
    // Ignore newline
}

QVariant HsmStateTextItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == QGraphicsItem::ItemPositionHasChanged) {
        emit positionChanged();
    }

    return QGraphicsTextItem::itemChange(change, value);
}

}  // namespace view
