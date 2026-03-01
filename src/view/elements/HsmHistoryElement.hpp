#ifndef HSMHISTORYELEMENT_HPP
#define HSMHISTORYELEMENT_HPP

#include "private/styles/HsmRoundElement.hpp"

class QPainter;
class QWidget;

namespace view {

class HsmHistoryElement : public HsmRoundElement {
public:
    HsmHistoryElement();
    virtual ~HsmHistoryElement() = default;

    bool isConnectable() const override;
    bool canBeTopLevel() const override;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
};

};  // namespace view

#endif  // HSMHISTORYELEMENT_HPP
