#ifndef HSMSTATETEXTITEM_HPP
#define HSMSTATETEXTITEM_HPP

#include <QGraphicsTextItem>
#include <QObject>
#include <QPointer>

namespace view {

class HsmStateTextItem : public QGraphicsTextItem {
    Q_OBJECT

public:
    // Logical parent is used for selection purposes when TextItem get's the focus
    HsmStateTextItem(QGraphicsItem* parent, QGraphicsItem* logicalParent);

    void makeMovable(const bool enable);
    void link(const QPointer<HsmStateTextItem>& other);

signals:
    void editingFinished();
    void positionChanged();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

private:
    QString mOriginalText;
    QGraphicsItem* mLogicalParent = nullptr;
};

}  // namespace view

#endif  // HSMSTATETEXTITEM_HPP
