#ifndef FINALSTATE_HPP
#define FINALSTATE_HPP

#include "State.hpp"

namespace model {

class FinalState : public State {
public:
    explicit FinalState(const QString& name);
    virtual ~FinalState() = default;

    void accept(class IModelVisitor* visitor) override;

    // Getters
    const QString& onStateChangedCallback() const;

    // Setters
    void setOnStateChangedCallback(const QString& callback);

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

private:
    QString mOnStateChangedCallback;
};

};  // namespace model

#endif  // FINALSTATE_HPP
