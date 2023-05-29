#ifndef HSMSTATEELEMENT_HPP
#define HSMSTATEELEMENT_HPP

#include <QRectF>

#include "private/HsmConnectableElement.hpp"

class QPainter;
class QWidget;

class HsmStateElement : public HsmConnectableElement {
public:
    HsmStateElement();
    virtual ~HsmStateElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

#endif  // HSMSTATEELEMENT_HPP
