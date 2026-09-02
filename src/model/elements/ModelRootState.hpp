#ifndef MODELROOTSTATE_HPP
#define MODELROOTSTATE_HPP

#include "RegularState.hpp"

namespace model {

class ModelRootState : public RegularState {
public:
    explicit ModelRootState(const QString& name);
    virtual ~ModelRootState() = default;

    QStringList properties() const override;
    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
};

};  // namespace model

#endif  // MODELROOTSTATE_HPP
