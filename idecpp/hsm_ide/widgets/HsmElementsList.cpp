#include "HsmElementsList.hpp"
#include <QDebug>

HsmElementsList::HsmElementsList(QWidget* parent)
    : QListWidget(parent)
{
}

QStringList HsmElementsList::mimeTypes() const
{
    // types = ["hsm/element", "hsm/multiple"]
    // QStringList types;
    // types << "hsm/element";
    return {"hsm/element"};
}

QMimeData* HsmElementsList::mimeData(const QList<QListWidgetItem*>& items) const
{
    qDebug() << "mimeData:" << items.length();
    QMimeData* data = nullptr;

    if (false == items.isEmpty()) {
        data = new QMimeData();

        if (items.length() == 1) {
            qDebug() << items.first()->data(Qt::UserRole);
            data->setData("hsm/element", items.first()->data(Qt::UserRole).toByteArray());
        } else {
            data->setData("hsm/multiple", QByteArray());
        }
    }

    return data;
}
