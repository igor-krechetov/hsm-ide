#ifndef HSMELEMENTSLISTDELEGATE_HPP
#define HSMELEMENTSLISTDELEGATE_HPP

#include <QStyledItemDelegate>

class HsmElementsListDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit HsmElementsListDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    static constexpr int cIconSize = 50;    // icon area size
    static constexpr int cTextWidth = 100;  // width allowed for wrapping
    static constexpr int cSpacing = 5;      // space between icon and text
};

#endif  // HSMELEMENTSLISTDELEGATE_HPP