#ifndef STATEMACHINEENTITYVIEWMODEL_HPP
#define STATEMACHINEENTITYVIEWMODEL_HPP

#include <QAbstractTableModel>
#include <QSharedPointer>

#include "model/StateMachineEntity.hpp"
#include "model/StateMachineModel.hpp"

namespace view {

class StateMachineEntityViewModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit StateMachineEntityViewModel(const QSharedPointer<model::StateMachineModel>& model, QObject* parent = nullptr);
    virtual ~StateMachineEntityViewModel() override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Select by id or pointer
    void selectEntityById(const model::EntityID_t id);
    void selectEntity(const QSharedPointer<model::StateMachineEntity>& entity);

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    // For drop-down support
    QStringList valueOptions(int row) const;

private:
    QSharedPointer<model::StateMachineModel> mModel;
    QSharedPointer<model::StateMachineEntity> mSelectedEntity;
    QMetaObject::Connection mEntitySignalConnection;
};

}  // namespace view

#endif  // STATEMACHINEENTITYVIEWMODEL_HPP