#ifndef OBJECTUTILS_HPP
#define OBJECTUTILS_HPP

class QObject;

void tryConnectSignal(const QObject* sender, const char* signalName, QObject* receiver, const char* slotName);

#endif  // OBJECTUTILS_HPP