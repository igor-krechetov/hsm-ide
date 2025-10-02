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
    QMimeData* mimeData(const QList<QListWidgetItem*>& items) const override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void startDrag(Qt::DropActions supportedActions) override;

private:
    QMimeData* createMimeData(QListWidgetItem* item) const;

private:
    QPoint m_pressPos;
};

#endif  // HSMELEMENTSLIST_HPP
