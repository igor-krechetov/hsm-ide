#ifndef HSMHISTORYELEMENT_HPP
#define HSMHISTORYELEMENT_HPP

#include <QRectF>

#include "private/HsmConnectableElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmHistoryElement : public HsmConnectableElement {
public:
    HsmHistoryElement();
    virtual ~HsmHistoryElement() = default;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF mItemRect;
};

};  // namespace view

#endif  // HSMHISTORYELEMENT_HPP
