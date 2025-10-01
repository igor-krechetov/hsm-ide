#ifndef HSMSTATETEXTITEM_HPP
#define HSMSTATETEXTITEM_HPP

#include <QGraphicsTextItem>
#include <QObject>

namespace view {

class HsmStateTextItem : public QGraphicsTextItem {
    Q_OBJECT
public:
    HsmStateTextItem(QGraphicsItem* parent = nullptr);

signals:
    void editingFinished();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QString mOriginalText;
};

} // namespace view

#endif // HSMSTATETEXTITEM_HPP
