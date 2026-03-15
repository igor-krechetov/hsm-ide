#include "HsmStateTextItem.hpp"

#include <QFocusEvent>
#include <QGraphicsScene>
#include <QKeyEvent>
#include <QPainter>
#include <QTextDocument>

#include "view/theme/ThemeManager.hpp"

namespace view {

HsmStateTextItem::HsmStateTextItem(QGraphicsItem* parent, QGraphicsItem* logicalParent)
    : QGraphicsTextItem(parent)
    , mLogicalParent(logicalParent) {
    const auto& theme = ThemeManager::instance().theme();

    setFont(theme.node.labelFont);
    setDefaultTextColor(theme.node.textColor);
    // setZValue(10);
    setTextInteractionFlags(Qt::TextEditorInteraction);

    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    document()->setDocumentMargin(0);

    QObject::connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, [this]() {
        const auto& theme = ThemeManager::instance().theme();

        setFont(theme.node.labelFont);
        setDefaultTextColor(theme.node.textColor);
        update();
    });
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

    if (nullptr != mLogicalParent) {
        // NOTE: in case of transitions, this selects the AutoGroupItem
        mLogicalParent->setSelected(true);
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

void HsmStateTextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    bool editing = hasFocus() && (textInteractionFlags() & Qt::TextEditorInteraction);

    if (editing) {
        // --- EDITING MODE RENDERING ---
        painter->setBrush(ThemeManager::instance().theme().node.editingBackgroundBrush);
        // painter->setPen(Qt::NoPen);
        painter->drawRect(boundingRect());
    } else {
        // --- NORMAL MODE RENDERING ---
        // painter->setPen(QPen(Qt::darkGray, 1));
        // painter->drawRect(boundingRect());
    }

    // Draw actual text normally
    QGraphicsTextItem::paint(painter, option, widget);
}

}  // namespace view
