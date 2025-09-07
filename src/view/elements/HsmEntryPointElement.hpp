#ifndef HSMENTRYPOINTELEMENT_HPP
#define HSMENTRYPOINTELEMENT_HPP

#include <QRectF>

#include "private/HsmConnectableElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmEntryPointElement : public HsmConnectableElement {
public:
    HsmEntryPointElement();
    virtual ~HsmEntryPointElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

};  // namespace view

#endif  // HSMENTRYPOINTELEMENT_HPP
