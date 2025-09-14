#include "ObjectUtils.hpp"

#include <QDebug>
#include <QMetaMethod>
#include <QObject>

void tryConnectSignal(const QObject* sender, const char* signalName, QObject* receiver, const char* slotName) {
    if (nullptr != sender && nullptr != receiver) {
        auto signalMethod = sender->metaObject()->method(sender->metaObject()->indexOfSignal(signalName));
        auto meta = receiver->metaObject();
        int index = receiver->metaObject()->indexOfSlot(slotName);
        auto slotMethod = receiver->metaObject()->method(receiver->metaObject()->indexOfSlot(slotName));

        // qDebug() << index;
        // qDebug() << signalMethod.name();
        // qDebug() << signalMethod.methodSignature();
        // qDebug() << slotMethod.name();
        // qDebug() << slotMethod.methodSignature();

        if (slotMethod.isValid()) {
            QObject::connect(sender, signalMethod, receiver, slotMethod);
        } else {
            // qWarning() << "Failed to connect " << signalName << " to " << slotName;
        }
    } else {
        qCritical() << Q_FUNC_INFO << "sender or receiver is NULL";
    }
}