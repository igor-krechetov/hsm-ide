#include "HsmStateTextItem.hpp"

#include <QFocusEvent>
#include <QGraphicsScene>
#include <QKeyEvent>

namespace view {

HsmStateTextItem::HsmStateTextItem(QGraphicsItem* parent)
    : QGraphicsTextItem(parent) {}

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

}  // namespace view
