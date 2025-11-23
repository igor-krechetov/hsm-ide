#include "HsmElementsList.hpp"

#include <QApplication>
#include <QDebug>
#include <QDrag>
#include <QMouseEvent>

#include "private/HsmElementsListDelegate.hpp"
#include "view/elements/HsmElementsFactory.hpp"

HsmElementsList::HsmElementsList(QWidget* parent)
    : QListWidget(parent) {
    setItemDelegate(new HsmElementsListDelegate(this));

    setViewMode(QListView::IconMode);
    setUniformItemSizes(true);  // to keep items the same size regardless of icon or text
    setWordWrap(true);
    setWrapping(false);  // to keep items in a single column
    setTextElideMode(Qt::ElideNone);
    setResizeMode(QListView::Adjust);
    setMouseTracking(true);

    setMinimumWidth(112);
    setMaximumWidth(112);

    // initialize list of HSM elements
    auto items = view::HsmElementsFactory::createElementsList();

    for (QListWidgetItem* newItem : items) {
        addItem(newItem);
    }
}

QStringList HsmElementsList::mimeTypes() const {
    // types = ["hsm/element", "hsm/multiple"]
    // QStringList types;
    // types << "hsm/element";
    return {"hsm/element"};
}

QMimeData* HsmElementsList::mimeData(const QList<QListWidgetItem*>& items) const {
    qDebug() << "mimeData:" << items.length();
    QMimeData* data = nullptr;

    if (false == items.isEmpty()) {
        if (items.length() == 1) {
            data = createMimeData(items.first());
        } else {
            data = new QMimeData();
            data->setData("hsm/multiple", QByteArray());
        }
    }

    return data;
}

void HsmElementsList::mousePressEvent(QMouseEvent* event) {
    m_pressPos = event->pos();
    QListWidget::mousePressEvent(event);
}

void HsmElementsList::mouseMoveEvent(QMouseEvent* event) {
    if (!(event->buttons() & Qt::LeftButton)) {
        QListWidget::mouseMoveEvent(event);
    } else if ((event->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance()) {
        QListWidget::mouseMoveEvent(event);
    } else {
        QListWidgetItem* it = itemAt(m_pressPos);

        if (it) {
            setCurrentItem(it);
            startDrag(Qt::CopyAction);
        }
    }
}

void HsmElementsList::startDrag(Qt::DropActions supportedActions) {
    QListWidgetItem* item = currentItem();

    if (item) {
        QDrag* drag = new QDrag(this);

        drag->setMimeData(createMimeData(item));

        if (!item->icon().isNull()) {
            drag->setPixmap(item->icon().pixmap(50, 50));
            // drag->setHotSpot(QPoint(drag->pixmap().width(), drag->pixmap().height()));
            drag->setHotSpot(QPoint(0, 0));
        }

        drag->exec(supportedActions);
    }
}

QMimeData* HsmElementsList::createMimeData(QListWidgetItem* item) const {
    QMimeData* data = nullptr;

    if (item) {
        data = new QMimeData();

        qDebug() << item->data(Qt::UserRole);
        data->setData("hsm/element", item->data(Qt::UserRole).toByteArray());
    }

    return data;
}
