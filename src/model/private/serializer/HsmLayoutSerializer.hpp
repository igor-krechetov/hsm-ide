#ifndef HSMLAYOUTSERIALIZER_HPP
#define HSMLAYOUTSERIALIZER_HPP

#include <QSharedPointer>

#include "model/ParseErrorCollector.hpp"

class QXmlStreamWriter;
class QXmlStreamReader;

namespace model {

class StateMachineModel;
class StateMachineEntity;

class HsmLayoutSerializer {
public:
    HsmLayoutSerializer() = default;
    ~HsmLayoutSerializer() = default;

    void writeEditorSection(const QSharedPointer<StateMachineModel>& model, QXmlStreamWriter& writer);

    void parseEditorSection(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model);

    void setParseReport(ParseErrorCollector* report);

private:
    void writeLayoutSection(const QSharedPointer<StateMachineModel>& model, QXmlStreamWriter& writer);
    void writeStateLayout(const StateMachineEntity* entity, QXmlStreamWriter& writer);
    void writeTransitionLayout(const StateMachineEntity* entity, QXmlStreamWriter& writer);

    void parseLayoutSection(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model);
    void parseStateLayout(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model);
    void parseTransitionLayout(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model);
    void parseCommentLayout(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model);

    ParseErrorCollector* mParseReport = nullptr;
};

}  // namespace model

#endif  // HSMLAYOUTSERIALIZER_HPP
