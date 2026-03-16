#ifndef HSMEXITPOINTELEMENT_HPP
#define HSMEXITPOINTELEMENT_HPP

#include "private/styles/HsmRoundElement.hpp"

namespace view {

class HsmExitPointElement : public HsmRoundElement {
public:
    HsmExitPointElement();
    virtual ~HsmExitPointElement() = default;

    bool isConnectable() const override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

};  // namespace view

#endif  // HSMEXITPOINTELEMENT_HPP
