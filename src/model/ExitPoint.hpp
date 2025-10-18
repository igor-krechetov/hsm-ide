#ifndef EXITPOINT_HPP
#define EXITPOINT_HPP

#include <QString>

#include "State.hpp"

namespace model {

class ExitPoint : public State {
public:
    explicit ExitPoint(const QString& name);
    virtual ~ExitPoint() = default;

    void accept(class IModelVisitor* visitor) override;

    // Getters
    const QString& event() const;
    const QString& onStateChangedCallback() const;
    const QString& onEnteringCallback() const;
    const QString& onExitingCallback() const;

    // Setters
    void setEvent(const QString& event);
    void setOnStateChangedCallback(const QString& callback);
    void setOnEnteringCallback(const QString& callback);
    void setOnExitingCallback(const QString& callback);

    bool setProperty(const QString& key, const QVariant& value) override;
    QVariant getProperty(const QString& key) const override;
    QStringList properties() const override;

private:
    QString mEvent;
    QString mOnStateChangedCallback;
    QString mOnEnteringCallback;
    QString mOnExitingCallback;
};

};  // namespace model

#endif  // EXITPOINT_HPP
