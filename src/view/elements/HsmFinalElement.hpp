#ifndef HSMFINALELEMENT_HPP
#define HSMFINALELEMENT_HPP

#include <QRectF>

#include "private/HsmConnectableElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmFinalElement : public HsmConnectableElement {
public:
    HsmFinalElement();
    virtual ~HsmFinalElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

}; // namespace view

#endif  // HSMFINALELEMENT_HPP
