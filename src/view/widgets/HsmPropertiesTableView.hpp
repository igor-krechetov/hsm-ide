#ifndef HSMPROPERTIESTABLEVIEW_HPP
#define HSMPROPERTIESTABLEVIEW_HPP

#include <QTableView>

class HsmPropertiesTableView : public QTableView {
    Q_OBJECT

public:
    explicit HsmPropertiesTableView(QWidget* parent = nullptr);
    virtual ~HsmPropertiesTableView() = default;

protected:
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;
    bool edit(const QModelIndex& index, QAbstractItemView::EditTrigger trigger, QEvent* event) override;
};

#endif  // HSMPROPERTIESTABLEVIEW_HPP
