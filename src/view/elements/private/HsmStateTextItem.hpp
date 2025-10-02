#ifndef HSMSTATETEXTITEM_HPP
#define HSMSTATETEXTITEM_HPP

#include <QGraphicsTextItem>
#include <QObject>

namespace view {

class HsmStateTextItem : public QGraphicsTextItem {
    Q_OBJECT
    
public:
    HsmStateTextItem(QGraphicsItem* parent = nullptr);

    void makeMovable(const bool enable);

signals:
    void editingFinished();
    void positionChanged();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QString mOriginalText;
};

} // namespace view

#endif // HSMSTATETEXTITEM_HPP
