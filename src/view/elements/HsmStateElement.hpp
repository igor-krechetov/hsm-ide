#ifndef HSMSTATEELEMENT_HPP
#define HSMSTATEELEMENT_HPP

#include <QRectF>

#include "private/HsmConnectableElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmStateElement : public HsmConnectableElement {
    Q_OBJECT

public:
    HsmStateElement();
    virtual ~HsmStateElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

}; // namespace view

#endif  // HSMSTATEELEMENT_HPP
