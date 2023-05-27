#ifndef HSMELEMENTSLIST_HPP
#define HSMELEMENTSLIST_HPP

#include <QListWidget>
#include <QMimeData>

class HsmElementsList : public QListWidget {
public:
    HsmElementsList(QWidget* parent);
    virtual ~HsmElementsList() = default;

protected:
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QList<QListWidgetItem*> items) const override;
};

#endif // HSMELEMENTSLIST_HPP
