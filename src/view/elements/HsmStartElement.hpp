#ifndef HSMSTARTELEMENT_HPP
#define HSMSTARTELEMENT_HPP

#include <QRectF>

#include "private/HsmConnectableElement.hpp"

class QPainter;
class QWidget;

class HsmStartElement : public HsmConnectableElement {
public:
    HsmStartElement();
    virtual ~HsmStartElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

#endif  // HSMSTARTELEMENT_HPP
