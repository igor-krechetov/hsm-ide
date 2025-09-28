#ifndef HSMENTITYPROPERTYDELEGATE_HPP
#define HSMENTITYPROPERTYDELEGATE_HPP

#include <QStyledItemDelegate>

namespace view {

class HsmEntityPropertyDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit HsmEntityPropertyDelegate(QObject* parent = nullptr);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                         const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
                      const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                             const QModelIndex& index) const override;
};

}; // namespace view

#endif // HSMENTITYPROPERTYDELEGATE_HPP