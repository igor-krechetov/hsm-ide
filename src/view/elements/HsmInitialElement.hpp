#ifndef HSMINITIALELEMENT_HPP
#define HSMINITIALELEMENT_HPP

#include "private/styles/HsmRoundElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmInitialElement : public HsmRoundElement {
public:
    HsmInitialElement();
    virtual ~HsmInitialElement() = default;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

};  // namespace view

#endif  // HSMINITIALELEMENT_HPP
