#ifndef HSMSTATEBODYSECTION_HPP
#define HSMSTATEBODYSECTION_HPP

#include <QGraphicsRectItem>
#include <QObject>

namespace view {

class HsmStateBodySection : public QObject, public QGraphicsRectItem {
    Q_OBJECT

public:
    HsmStateBodySection(QGraphicsItem* parent = nullptr);
    virtual ~HsmStateBodySection() = default;

    bool hasSubstates() const;

signals:
    void substatesChanged(const bool hasSubstates);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    bool mHasSubstates = false;
};

}  // namespace view

#endif  // HSMSTATEBODYSECTION_HPP
