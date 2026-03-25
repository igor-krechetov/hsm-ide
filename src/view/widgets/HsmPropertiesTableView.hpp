#ifndef HSMPROPERTIESTABLEVIEW_HPP
#define HSMPROPERTIESTABLEVIEW_HPP

#include <QTreeView>

class HsmPropertiesTableView : public QTreeView {
    Q_OBJECT

public:
    explicit HsmPropertiesTableView(QWidget* parent = nullptr);
    virtual ~HsmPropertiesTableView() = default;
    void setModel(QAbstractItemModel* model) override;

protected:
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
    bool edit(const QModelIndex& index, QAbstractItemView::EditTrigger trigger, QEvent* event) override;
};

#endif  // HSMPROPERTIESTABLEVIEW_HPP
