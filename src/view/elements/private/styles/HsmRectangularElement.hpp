#ifndef HSMRECTANGULARELEMENT_HPP
#define HSMRECTANGULARELEMENT_HPP

#include <QBrush>
#include <QPen>

#include "view/elements/private/HsmResizableElement.hpp"

class QPainter;
class QWidget;
class QStyleOptionGraphicsItem;

namespace view {

class HsmRectangularElement : public HsmResizableElement {
public:
    explicit HsmRectangularElement(const HsmElementType elementType);
    HsmRectangularElement(const HsmElementType elementType, const QSizeF& size);
    virtual ~HsmRectangularElement() = default;

protected:
    void paintRectangularBody(QPainter* painter, const QBrush& backgroundBrush) const;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    // Inset of the painted body within mOuterRect. mOuterRect is a grid multiple and is
    // not inflated by the pen, so this inset must be >= ceil(selectedBorderPen.widthF() / 2)
    // (half of the widest 3.0px pen = 1.5 -> 2) to keep the whole stroke inside the rect.
    static constexpr int cOuterBorderAdjustment = 2;
};

};  // namespace view

#endif  // HSMRECTANGULARELEMENT_HPP
