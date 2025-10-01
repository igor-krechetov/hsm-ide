#ifndef HSMFINALELEMENT_HPP
#define HSMFINALELEMENT_HPP

#include "private/styles/HsmRoundElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmFinalElement : public HsmRoundElement {
public:
    HsmFinalElement();
    virtual ~HsmFinalElement() = default;

    bool isConnectable() const override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

};  // namespace view

#endif  // HSMFINALELEMENT_HPP
