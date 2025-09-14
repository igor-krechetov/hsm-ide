#ifndef HSMRECTANGULARELEMENT_HPP
#define HSMRECTANGULARELEMENT_HPP

#include <QPen>
#include <QBrush>

#include "view/elements/private/HsmResizableElement.hpp"

class QPainter;
class QWidget;
class QStyleOptionGraphicsItem;

namespace view {

class HsmRectangularElement : public HsmResizableElement {
public:
    HsmRectangularElement(const HsmElementType elementType);
    virtual ~HsmRectangularElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    QPen mPenNormalMode;
    QPen mPenHighlightMode;
    QPen mPenSelectedBorder;
    QBrush mBackgroundBrush;
    QBrush mMainBrush;
};

};  // namespace view

#endif  // HSMRECTANGULARELEMENT_HPP
