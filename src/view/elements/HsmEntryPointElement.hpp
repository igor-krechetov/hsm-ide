#ifndef HSMENTRYPOINTELEMENT_HPP
#define HSMENTRYPOINTELEMENT_HPP

#include "private/styles/HsmRoundElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmEntryPointElement : public HsmRoundElement {
public:
    HsmEntryPointElement();
    virtual ~HsmEntryPointElement() = default;

    bool acceptsConnections() const override;
    bool canBeTopLevel() const override;

protected:
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

};  // namespace view

#endif  // HSMENTRYPOINTELEMENT_HPP
