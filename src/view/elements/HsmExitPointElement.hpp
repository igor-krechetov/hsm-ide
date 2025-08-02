#ifndef HSMEXITPOINTELEMENT_HPP
#define HSMEXITPOINTELEMENT_HPP

#include <QRectF>

#include "private/HsmConnectableElement.hpp"

namespace view {

class HsmExitPointElement : public HsmConnectableElement {
public:
    HsmExitPointElement();
    virtual ~HsmExitPointElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

}; // namespace view

#endif  // HSMEXITPOINTELEMENT_HPP
