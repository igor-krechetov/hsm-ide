#ifndef HSMFINALELEMENT_HPP
#define HSMFINALELEMENT_HPP

#include <QRectF>

#include "HsmConnectableElement.hpp"

class QPainter;
class QWidget;

class HsmFinalElement : public HsmConnectableElement {
public:
    HsmFinalElement();
    virtual ~HsmFinalElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

#endif  // HSMFINALELEMENT_HPP
