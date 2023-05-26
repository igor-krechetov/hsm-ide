#ifndef HSMEXITPOINTELEMENT_HPP
#define HSMEXITPOINTELEMENT_HPP

#include <QRectF>

#include "HsmConnectableElement.hpp"

class HsmExitPointElement : public HsmConnectableElement {
public:
    HsmExitPointElement();
    virtual ~HsmExitPointElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

#endif  // HSMEXITPOINTELEMENT_HPP
