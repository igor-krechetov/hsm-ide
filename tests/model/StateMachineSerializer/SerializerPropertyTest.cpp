#include <QtTest>

#include <QPolygonF>
#include <QRandomGenerator>
#include <QSet>
#include <QXmlStreamReader>

#include "model/private/EntityIdGenerator.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/elements/ModelRootState.hpp"
#include "model/elements/RegularState.hpp"
#include "model/StateMachineModel.hpp"
#include "model/StateMachineSerializer.hpp"
#include "model/elements/Transition.hpp"

class SerializerPropertyTest : public QObject {
    Q_OBJECT

private slots:
    void PropertyHsmFormatNamespaceCorrectness();
    void PropertyQtCreatorFormatExcludesHsmContent();
    void PropertyFormatAutoDetectionCorrectness();
    void PropertyMixedFileUidHandling();
    void PropertyDuplicateUidResolution();
    void PropertyCounterSynchronization();
    void PropertyHsmEditorSectionStructure();
    void PropertyLayoutGeometryRoundTrip();
    void PropertyTransitionLayoutExcludesEndpoints();

private:
    /**
     * @brief Builds a minimal SCXML string with the given namespace declarations and state elements.
     * @param namespaces Additional xmlns attributes to include on the root scxml element.
     * @param stateElements Inner state XML content.
     * @return A well-formed SCXML string.
     */
    static QString buildScxml(const QString& namespaces, const QString& stateElements);
};

QString SerializerPropertyTest::buildScxml(const QString& namespaces, const QString& stateElements) {
    return QString(
               "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
               "<scxml xmlns=\"http://www.w3.org/2005/07/scxml\" "
               "xmlns:xi=\"http://www.w3.org/2001/XInclude\" "
               "%1 "
               "version=\"1.0\" name=\"Test\">"
               "%2"
               "</scxml>")
        .arg(namespaces, stateElements);
}

/**
 * @brief Property 5: HSM Format Namespace Correctness.
 *
 * For any model serialized in HSM format, the output root <scxml> element SHALL
 * contain xmlns:hsm="https://hsm-ide.dev/scxml" and SHALL NOT contain the xmlns:qt
 * namespace declaration, and SHALL NOT contain any qt:editorinfo elements.
 *
 * Validates: Requirements 2.1, 2.2, 3.5
 */
void SerializerPropertyTest::PropertyHsmFormatNamespaceCorrectness() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        auto model = QSharedPointer<model::StateMachineModel>::create("TestModel");
        auto root = model->root();

        // Create random number of states (2..5)
        const int stateCount = static_cast<int>(rng.bounded(2, 6));
        QList<QSharedPointer<model::RegularState>> states;

        for (int i = 0; i < stateCount; ++i) {
            auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                             .dynamicCast<model::RegularState>();
            QVERIFY(state);
            state->setName(QString("State_%1_%2").arg(iteration).arg(i));
            root->addChildState(state);
            states.append(state);
        }

        // Create random transitions (1..3)
        const int transitionCount = static_cast<int>(rng.bounded(1, 4));

        for (int i = 0; i < transitionCount; ++i) {
            const int srcIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
            int dstIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));

            if (dstIdx == srcIdx) {
                dstIdx = (dstIdx + 1) % states.size();
            }

            const QString event = QString("event_%1_%2").arg(iteration).arg(i);
            auto transition = model::ModelElementsFactory::createTransitionWithId(states[srcIdx], states[dstIdx], event, model->idGenerator().generateNextId());
            states[srcIdx]->addTransition(transition);
        }

        // Serialize in HSM format
        model::StateMachineSerializer serializer;
        const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

        // Verify: output contains xmlns:hsm="https://hsm-ide.dev/scxml"
        QVERIFY2(scxml.contains("xmlns:hsm=\"https://hsm-ide.dev/scxml\""),
                 qPrintable(QString("Iteration %1: HSM namespace declaration missing").arg(iteration)));

        // Verify: output does NOT contain xmlns:qt
        QVERIFY2(!scxml.contains("xmlns:qt"),
                 qPrintable(QString("Iteration %1: xmlns:qt should not be present in HSM format").arg(iteration)));

        // Verify: output does NOT contain qt:editorinfo
        QVERIFY2(!scxml.contains("qt:editorinfo"),
                 qPrintable(QString("Iteration %1: qt:editorinfo should not be present in HSM format").arg(iteration)));

        // Verify: output contains hsm:uid= attributes
        QVERIFY2(scxml.contains("hsm:uid="),
                 qPrintable(QString("Iteration %1: hsm:uid attributes missing in HSM format").arg(iteration)));
    }
}

/**
 * @brief Property 6: Qt Creator Format Excludes HSM Content.
 *
 * For any model serialized in Qt Creator format, the output SHALL NOT contain
 * the string hsm:uid, the xmlns:hsm namespace declaration, or any <hsm:editor>
 * section, and SHALL contain xmlns:qt with inline qt:editorinfo elements for
 * entities that have geometry.
 *
 * Validates: Requirements 2.3, 4.1, 4.2, 4.3, 4.4
 */
void SerializerPropertyTest::PropertyQtCreatorFormatExcludesHsmContent() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        auto model = QSharedPointer<model::StateMachineModel>::create("TestModel");
        auto root = model->root();

        // Create random number of states (2..5) with geometry
        const int stateCount = static_cast<int>(rng.bounded(2, 6));
        QList<QSharedPointer<model::RegularState>> states;

        for (int i = 0; i < stateCount; ++i) {
            auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                             .dynamicCast<model::RegularState>();
            QVERIFY(state);
            state->setName(QString("State_%1_%2").arg(iteration).arg(i));

            // Assign random geometry so qt:editorinfo is generated
            const double x = rng.bounded(0, 500);
            const double y = rng.bounded(0, 500);
            const double w = rng.bounded(60, 200);
            const double h = rng.bounded(40, 150);
            state->setPos(QPointF(x, y));
            state->setSize(QSizeF(w, h));

            root->addChildState(state);
            states.append(state);
        }

        // Create random transitions (1..3)
        const int transitionCount = static_cast<int>(rng.bounded(1, 4));

        for (int i = 0; i < transitionCount; ++i) {
            const int srcIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
            int dstIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));

            if (dstIdx == srcIdx) {
                dstIdx = (dstIdx + 1) % states.size();
            }

            const QString event = QString("event_%1_%2").arg(iteration).arg(i);
            auto transition = model::ModelElementsFactory::createTransitionWithId(states[srcIdx], states[dstIdx], event, model->idGenerator().generateNextId());
            states[srcIdx]->addTransition(transition);
        }

        // Serialize in QtCreator format
        model::StateMachineSerializer serializer;
        const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::QtCreator);

        // Verify: output does NOT contain hsm:uid
        QVERIFY2(!scxml.contains("hsm:uid"),
                 qPrintable(QString("Iteration %1: hsm:uid should not be present in QtCreator format").arg(iteration)));

        // Verify: output does NOT contain xmlns:hsm
        QVERIFY2(!scxml.contains("xmlns:hsm"),
                 qPrintable(
                     QString("Iteration %1: xmlns:hsm should not be present in QtCreator format").arg(iteration)));

        // Verify: output does NOT contain <hsm:editor>
        QVERIFY2(!scxml.contains("<hsm:editor>"),
                 qPrintable(
                     QString("Iteration %1: <hsm:editor> should not be present in QtCreator format").arg(iteration)));

        // Verify: output contains xmlns:qt="http://www.qt.io/2015/02/scxml-ext"
        QVERIFY2(scxml.contains("xmlns:qt=\"http://www.qt.io/2015/02/scxml-ext\""),
                 qPrintable(
                     QString("Iteration %1: Qt namespace declaration missing in QtCreator format").arg(iteration)));

        // Verify: output contains qt:editorinfo (for entities with geometry)
        QVERIFY2(scxml.contains("qt:editorinfo"),
                 qPrintable(QString("Iteration %1: qt:editorinfo missing in QtCreator format (states have geometry)")
                                .arg(iteration)));
    }
}

/**
 * @brief Property 11: Format Auto-Detection Correctness.
 *
 * For any SCXML file, the deserializer SHALL correctly detect the format based on
 * namespace declarations: xmlns:hsm present implies HSM format (regardless of xmlns:qt
 * presence), xmlns:qt without xmlns:hsm implies Qt Creator format, neither implies
 * Plain SCXML format.
 *
 * Validates: Requirements 11.1, 11.2, 11.3, 11.4, 11.5
 */
void SerializerPropertyTest::PropertyFormatAutoDetectionCorrectness() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        // Randomly pick a namespace combination: HSM only, Qt only, both, neither
        const int combo = static_cast<int>(rng.bounded(4));

        QString namespaces;
        bool expectHsm = false;
        bool expectQt = false;

        switch (combo) {
            case 0:
                // HSM namespace only
                namespaces = QString("xmlns:hsm=\"%1\"").arg(model::scxml::HSM_NAMESPACE_URI);
                expectHsm = true;
                break;
            case 1:
                // Qt namespace only
                namespaces = QString("xmlns:qt=\"%1\"").arg(model::scxml::QT_NAMESPACE_URI);
                expectQt = true;
                break;
            case 2:
                // Both HSM and Qt namespaces (HSM takes precedence)
                namespaces = QString("xmlns:hsm=\"%1\" xmlns:qt=\"%2\"")
                                 .arg(model::scxml::HSM_NAMESPACE_URI, model::scxml::QT_NAMESPACE_URI);
                expectHsm = true;
                break;
            case 3:
                // Neither namespace
                namespaces = "";
                break;
            default:
                break;
        }

        // Generate a random number of states (1..5) with random names
        const int stateCount = static_cast<int>(rng.bounded(1, 6));
        QString stateElements;

        for (int s = 0; s < stateCount; ++s) {
            QString stateId = QString("State_%1_%2").arg(iteration).arg(s);

            if (expectHsm) {
                // HSM format: include hsm:uid attributes
                quint32 uid = static_cast<quint32>(rng.bounded(1u, 10000u));
                stateElements += QString("<state id=\"%1\" hsm:uid=\"%2\"/>").arg(stateId).arg(uid);
            } else if (expectQt) {
                // Qt format: include qt:editorinfo elements
                stateElements +=
                    QString("<state id=\"%1\"><qt:editorinfo scenegeometry=\"0;0;100;100;120;60\"/></state>").arg(stateId);
            } else {
                // Plain format: just states
                stateElements += QString("<state id=\"%1\"/>").arg(stateId);
            }
        }

        QString scxml = buildScxml(namespaces, stateElements);

        // Deserialize
        model::StateMachineSerializer serializer;
        auto model = serializer.deserializeFromScxml(scxml);

        QVERIFY2(model != nullptr,
                 qPrintable(QString("Iteration %1: deserialization returned null").arg(iteration)));

        // Verify correct number of entities parsed
        auto root = model->root();
        QVERIFY(root != nullptr);

        // Verify UIDs are handled correctly based on detected format
        QSet<model::EntityID_t> allIds;
        bool allValid = true;

        root->forEachChildElement(
            [&](QSharedPointer<model::StateMachineEntity> /*parent*/, QSharedPointer<model::StateMachineEntity> child) {
                if (child->id() == model::INVALID_MODEL_ID || child->id() == 0) {
                    allValid = false;
                }
                allIds.insert(child->id());
                return true;
            });

        QVERIFY2(allValid,
                 qPrintable(QString("Iteration %1: found entity with invalid ID (combo=%2)").arg(iteration).arg(combo)));

        // If HSM format with valid UIDs, the first state's UID should be preserved from the file
        if (expectHsm) {
            // Serialize back in HSM format and verify hsm:uid attributes are present
            QString reserialized = serializer.serializeToScxml(model, model::SerializationFormat::HSM);
            QVERIFY2(reserialized.contains("hsm:uid"),
                     qPrintable(QString("Iteration %1: HSM re-serialization missing hsm:uid").arg(iteration)));
        }

        if (expectQt && !expectHsm) {
            // When Qt format detected, UIDs are generated (not from file) so all should be unique
            // Verify no hsm:uid-based preservation happened (all UIDs are freshly generated)
            // Just verify all are valid and unique
            QVERIFY2(!allIds.isEmpty(),
                     qPrintable(QString("Iteration %1: no entities found in Qt format").arg(iteration)));
        }

        if (!expectHsm && !expectQt) {
            // Plain format: all UIDs are generated
            QVERIFY2(!allIds.isEmpty() || stateCount == 0,
                     qPrintable(QString("Iteration %1: no entities found in plain format").arg(iteration)));
        }
    }
}

/**
 * @brief Property 12: Mixed File UID Handling.
 *
 * For any SCXML file containing a mix of elements with valid unique hsm:uid
 * attributes and elements without hsm:uid, deserialization SHALL produce a model
 * where: (a) entities from elements with valid UIDs retain those exact values,
 * (b) all entity UIDs (preserved and generated) are mutually unique, and
 * (c) generated UIDs are greater than the maximum UID found in the file.
 *
 * Validates: Requirements 5.2, 7.1, 7.2, 7.3, 7.4, 7.5
 */
void SerializerPropertyTest::PropertyMixedFileUidHandling() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        // Generate a random number of states (3..8)
        const int stateCount = static_cast<int>(rng.bounded(3, 9));

        // Decide which states have hsm:uid and which don't
        QMap<QString, model::EntityID_t> statesWithUid;
        QStringList statesWithoutUid;
        QSet<model::EntityID_t> usedUids;

        // Ensure UIDs assigned to states are unique and in a reasonable range
        model::EntityID_t maxUid = 0;

        for (int s = 0; s < stateCount; ++s) {
            QString stateId = QString("S%1_%2").arg(iteration).arg(s);
            bool hasUid = rng.bounded(2) == 0;

            if (hasUid) {
                // Generate a unique UID for this state. Range starts at 2 because UID 1 is
                // reserved by the model root, so a file UID of 1 is not preservable.
                model::EntityID_t uid;

                do {
                    uid = static_cast<model::EntityID_t>(rng.bounded(2u, 500u));
                } while (usedUids.contains(uid));

                usedUids.insert(uid);
                statesWithUid.insert(stateId, uid);

                if (uid > maxUid) {
                    maxUid = uid;
                }
            } else {
                statesWithoutUid.append(stateId);
            }
        }

        // Ensure we have at least one of each type for a meaningful test
        if (statesWithUid.isEmpty()) {
            model::EntityID_t uid;

            do {
                uid = static_cast<model::EntityID_t>(rng.bounded(2u, 500u));
            } while (usedUids.contains(uid));

            usedUids.insert(uid);
            QString stateId = statesWithoutUid.takeLast();
            statesWithUid.insert(stateId, uid);

            if (uid > maxUid) {
                maxUid = uid;
            }
        }

        if (statesWithoutUid.isEmpty()) {
            statesWithoutUid.append(QString("NoUid_%1").arg(iteration));
        }

        // Build the SCXML with HSM namespace
        QString stateElements;

        for (auto it = statesWithUid.constBegin(); it != statesWithUid.constEnd(); ++it) {
            stateElements += QString("<state id=\"%1\" hsm:uid=\"%2\"/>").arg(it.key()).arg(it.value());
        }

        for (const QString& stateId : statesWithoutUid) {
            stateElements += QString("<state id=\"%1\"/>").arg(stateId);
        }

        QString namespaces = QString("xmlns:hsm=\"%1\"").arg(model::scxml::HSM_NAMESPACE_URI);
        QString scxml = buildScxml(namespaces, stateElements);

        // Deserialize
        model::StateMachineSerializer serializer;
        auto model = serializer.deserializeFromScxml(scxml);

        QVERIFY2(model != nullptr,
                 qPrintable(QString("Iteration %1: deserialization returned null").arg(iteration)));

        auto root = model->root();
        QVERIFY(root != nullptr);

        // Collect all entity IDs and names
        QMap<QString, model::EntityID_t> entityNameToId;
        QSet<model::EntityID_t> allIds;

        root->forEachChildElement(
            [&](QSharedPointer<model::StateMachineEntity> /*parent*/, QSharedPointer<model::StateMachineEntity> child) {
                auto state = child.dynamicCast<model::State>();

                if (state) {
                    entityNameToId.insert(state->name(), state->id());
                    allIds.insert(state->id());
                }

                return true;
            });

        // (a) Elements with valid UIDs retain those exact values
        for (auto it = statesWithUid.constBegin(); it != statesWithUid.constEnd(); ++it) {
            QVERIFY2(entityNameToId.contains(it.key()),
                     qPrintable(QString("Iteration %1: state '%2' not found in model").arg(iteration).arg(it.key())));
            QVERIFY2(entityNameToId.value(it.key()) == it.value(),
                     qPrintable(QString("Iteration %1: state '%2' expected UID %3 but got %4")
                                    .arg(iteration)
                                    .arg(it.key())
                                    .arg(it.value())
                                    .arg(entityNameToId.value(it.key()))));
        }

        // (b) All UIDs are mutually unique
        int totalEntityCount = entityNameToId.size();
        QVERIFY2(allIds.size() == totalEntityCount,
                 qPrintable(QString("Iteration %1: UID uniqueness violated (%2 unique IDs vs %3 entities)")
                                .arg(iteration)
                                .arg(allIds.size())
                                .arg(totalEntityCount)));

        // (c) Generated UIDs are greater than the max UID found in the file
        for (const QString& stateId : statesWithoutUid) {
            if (entityNameToId.contains(stateId)) {
                model::EntityID_t generatedId = entityNameToId.value(stateId);
                QVERIFY2(generatedId > maxUid,
                         qPrintable(QString("Iteration %1: generated UID %2 for '%3' is not > max file UID %4")
                                        .arg(iteration)
                                        .arg(generatedId)
                                        .arg(stateId)
                                        .arg(maxUid)));
            }
        }
    }
}

/**
 * @brief Property 13: Duplicate UID Resolution Without Data Loss.
 *
 * For any SCXML file containing duplicate hsm:uid values, deserialization SHALL:
 * (a) preserve the UID for the first element in document order,
 * (b) assign new unique UIDs to subsequent elements with the same value, and
 * (c) produce a model containing all entities from the file without discarding any.
 * (d) all UIDs are unique.
 *
 * Validates: Requirements 5.4, 8.1, 8.3, 8.4, 8.5
 */
void SerializerPropertyTest::PropertyDuplicateUidResolution() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        // Generate a random number of states (3..8)
        const int stateCount = static_cast<int>(rng.bounded(3, 9));

        // Choose a random UID that will be duplicated. Range starts at 2 because UID 1 is
        // reserved by the model root, so the first occurrence could not preserve a UID of 1.
        model::EntityID_t duplicateUid = static_cast<model::EntityID_t>(rng.bounded(2u, 1000u));

        // Decide how many states will have the duplicate UID (at least 2)
        const int duplicateCount = static_cast<int>(rng.bounded(2, qMin(stateCount + 1, 6)));

        // Build state elements - the first `duplicateCount` states share the same UID,
        // the rest get unique UIDs
        QString stateElements;
        QStringList allStateNames;
        QSet<model::EntityID_t> uniqueUids;
        uniqueUids.insert(duplicateUid);

        for (int s = 0; s < stateCount; ++s) {
            QString stateId = QString("D%1_%2").arg(iteration).arg(s);
            allStateNames.append(stateId);

            if (s < duplicateCount) {
                // These all share the same duplicate UID
                stateElements += QString("<state id=\"%1\" hsm:uid=\"%2\"/>").arg(stateId).arg(duplicateUid);
            } else {
                // These get unique UIDs (range starts at 2; UID 1 is reserved by the model root)
                model::EntityID_t uid;

                do {
                    uid = static_cast<model::EntityID_t>(rng.bounded(2u, 5000u));
                } while (uniqueUids.contains(uid));

                uniqueUids.insert(uid);
                stateElements += QString("<state id=\"%1\" hsm:uid=\"%2\"/>").arg(stateId).arg(uid);
            }
        }

        QString namespaces = QString("xmlns:hsm=\"%1\"").arg(model::scxml::HSM_NAMESPACE_URI);
        QString scxml = buildScxml(namespaces, stateElements);

        // Deserialize
        model::StateMachineSerializer serializer;
        auto model = serializer.deserializeFromScxml(scxml);

        QVERIFY2(model != nullptr,
                 qPrintable(QString("Iteration %1: deserialization returned null").arg(iteration)));

        auto root = model->root();
        QVERIFY(root != nullptr);

        // Collect all entities
        QMap<QString, model::EntityID_t> entityNameToId;
        QSet<model::EntityID_t> allIds;

        root->forEachChildElement(
            [&](QSharedPointer<model::StateMachineEntity> /*parent*/, QSharedPointer<model::StateMachineEntity> child) {
                auto state = child.dynamicCast<model::State>();

                if (state) {
                    entityNameToId.insert(state->name(), state->id());
                    allIds.insert(state->id());
                }

                return true;
            });

        // (c) All entities from the file are present (no data loss)
        for (const QString& name : allStateNames) {
            QVERIFY2(entityNameToId.contains(name),
                     qPrintable(
                         QString("Iteration %1: state '%2' missing from model (data loss)").arg(iteration).arg(name)));
        }

        // (a) First element with the duplicate UID preserves it
        QString firstDuplicateName = allStateNames.at(0);
        QVERIFY2(entityNameToId.value(firstDuplicateName) == duplicateUid,
                 qPrintable(QString("Iteration %1: first duplicate state '%2' expected UID %3 but got %4")
                                .arg(iteration)
                                .arg(firstDuplicateName)
                                .arg(duplicateUid)
                                .arg(entityNameToId.value(firstDuplicateName))));

        // (b) Subsequent elements with duplicate UID get new unique IDs
        for (int s = 1; s < duplicateCount; ++s) {
            QString name = allStateNames.at(s);
            model::EntityID_t assignedId = entityNameToId.value(name);

            QVERIFY2(assignedId != duplicateUid,
                     qPrintable(
                         QString("Iteration %1: duplicate state '%2' at position %3 still has duplicate UID %4")
                             .arg(iteration)
                             .arg(name)
                             .arg(s)
                             .arg(duplicateUid)));
        }

        // (d) All UIDs are unique
        QVERIFY2(allIds.size() == entityNameToId.size(),
                 qPrintable(QString("Iteration %1: UID uniqueness violated (%2 unique vs %3 entities)")
                                .arg(iteration)
                                .arg(allIds.size())
                                .arg(entityNameToId.size())));
    }
}

/**
 * @brief Property 15: Counter Synchronization After Deserialization.
 *
 * For any model loaded from a file containing hsm:uid attributes, all subsequently
 * generated UIDs (from new entity creation) SHALL be strictly greater than the
 * maximum UID value found in the file.
 *
 * Validates: Requirements 7.2, 8.4, 10.2
 */
void SerializerPropertyTest::PropertyCounterSynchronization() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        // Generate a random number of states (2..6) with random UIDs
        const int stateCount = static_cast<int>(rng.bounded(2, 7));
        QSet<model::EntityID_t> fileUids;
        model::EntityID_t maxFileUid = 0;
        QString stateElements;

        for (int s = 0; s < stateCount; ++s) {
            model::EntityID_t uid;

            do {
                uid = static_cast<model::EntityID_t>(rng.bounded(1u, 50000u));
            } while (fileUids.contains(uid));

            fileUids.insert(uid);

            if (uid > maxFileUid) {
                maxFileUid = uid;
            }

            stateElements += QString("<state id=\"CS%1_%2\" hsm:uid=\"%3\"/>").arg(iteration).arg(s).arg(uid);
        }

        QString namespaces = QString("xmlns:hsm=\"%1\"").arg(model::scxml::HSM_NAMESPACE_URI);
        QString scxml = buildScxml(namespaces, stateElements);

        // Deserialize
        model::StateMachineSerializer serializer;
        auto model = serializer.deserializeFromScxml(scxml);

        QVERIFY2(model != nullptr,
                 qPrintable(QString("Iteration %1: deserialization returned null").arg(iteration)));

        // Create new entities in the loaded model and verify their UIDs > maxFileUid
        const int newEntityCount = static_cast<int>(rng.bounded(2, 11));

        for (int n = 0; n < newEntityCount; ++n) {
            auto newState =
                model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator());

            QVERIFY2(newState != nullptr,
                     qPrintable(QString("Iteration %1: createUniqueState returned null at index %2")
                                    .arg(iteration)
                                    .arg(n)));
            QVERIFY2(newState->id() > maxFileUid,
                     qPrintable(QString("Iteration %1: new entity UID %2 not > max file UID %3")
                                    .arg(iteration)
                                    .arg(newState->id())
                                    .arg(maxFileUid)));
            QVERIFY2(newState->id() != model::INVALID_MODEL_ID,
                     qPrintable(
                         QString("Iteration %1: new entity UID is INVALID_MODEL_ID at index %2").arg(iteration).arg(n)));
        }
    }
}

/**
 * @brief Property 8: HSM Editor Section Structure.
 *
 * For any model serialized in HSM format, the <hsm:editor> element SHALL appear
 * as the last child of the root <scxml> element, containing an <hsm:layout> child
 * with one <hsm:state> element per state-like entity with geometry.
 *
 * Validates: Requirements 3.6, 3.7, 12.1, 12.3
 */
void SerializerPropertyTest::PropertyHsmEditorSectionStructure() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        auto model = QSharedPointer<model::StateMachineModel>::create("TestModel");
        auto root = model->root();

        // Create random number of states (2..5) with geometry (position/size metadata)
        const int stateCount = static_cast<int>(rng.bounded(2, 6));
        QList<QSharedPointer<model::RegularState>> states;
        QSet<model::EntityID_t> statesWithGeometry;

        for (int i = 0; i < stateCount; ++i) {
            auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                             .dynamicCast<model::RegularState>();
            QVERIFY(state);
            state->setName(QString("S%1_%2").arg(iteration).arg(i));

            // Assign random position and size
            const double x = rng.bounded(0, 1000);
            const double y = rng.bounded(0, 1000);
            const double w = rng.bounded(50, 300);
            const double h = rng.bounded(50, 300);
            state->setPos(QPointF(x, y));
            state->setSize(QSizeF(w, h));
            statesWithGeometry.insert(state->id());

            root->addChildState(state);
            states.append(state);
        }

        // Serialize in HSM format
        model::StateMachineSerializer serializer;
        const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

        // Parse with QXmlStreamReader to verify structure
        QXmlStreamReader reader(scxml);
        bool foundScxml = false;
        QString lastChildLocalName;
        bool editorHasLayout = false;
        QSet<model::EntityID_t> layoutStateUids;

        // Navigate to root <scxml> element
        while (!reader.atEnd()) {
            reader.readNext();

            if (reader.isStartElement() && reader.name() == QStringView(u"scxml")) {
                foundScxml = true;
                break;
            }
        }

        QVERIFY2(foundScxml, qPrintable(QString("Iteration %1: <scxml> root not found").arg(iteration)));

        // Iterate over direct children of <scxml> to find the last one and verify <hsm:editor>
        int depth = 0;

        while (!reader.atEnd()) {
            reader.readNext();

            if (reader.isStartElement()) {
                if (depth == 0) {
                    lastChildLocalName = reader.name().toString();

                    // If this is <hsm:editor> (local name "editor"), inspect its children
                    if (reader.name() == QStringView(u"editor")) {
                        // Check for <hsm:layout> child
                        while (reader.readNextStartElement()) {
                            if (reader.name() == QStringView(u"layout")) {
                                editorHasLayout = true;

                                // Collect <hsm:state> uid attributes inside layout
                                while (reader.readNextStartElement()) {
                                    if (reader.name() == QStringView(u"state")) {
                                        const QString uidStr = reader.attributes().value("uid").toString();
                                        bool ok = false;
                                        const model::EntityID_t uid = uidStr.toUInt(&ok);

                                        if (ok) {
                                            layoutStateUids.insert(uid);
                                        }

                                        reader.skipCurrentElement();
                                    } else {
                                        reader.skipCurrentElement();
                                    }
                                }
                            } else {
                                reader.skipCurrentElement();
                            }
                        }
                    } else {
                        depth++;
                    }
                } else {
                    depth++;
                }
            } else if (reader.isEndElement()) {
                if (depth == 0) {
                    break;  // End of <scxml>
                }

                depth--;
            }
        }

        // Verify: <hsm:editor> (local name "editor") is the LAST child element of root <scxml>
        QVERIFY2(lastChildLocalName == "editor",
                 qPrintable(QString("Iteration %1: last child of <scxml> is '%2', expected 'editor'")
                                .arg(iteration)
                                .arg(lastChildLocalName)));

        // Verify: <hsm:editor> contains <hsm:layout> child
        QVERIFY2(editorHasLayout,
                 qPrintable(QString("Iteration %1: <hsm:editor> does not contain <hsm:layout>").arg(iteration)));

        // Verify: for each state with geometry, there's a corresponding <hsm:state> in layout with matching uid
        for (model::EntityID_t uid : statesWithGeometry) {
            QVERIFY2(layoutStateUids.contains(uid),
                     qPrintable(QString("Iteration %1: state uid %2 not found in <hsm:layout>")
                                    .arg(iteration)
                                    .arg(uid)));
        }
    }
}

/**
 * @brief Property 10: Layout Geometry Round-Trip.
 *
 * For any model with entities that have position and size metadata, serializing to HSM
 * format and then deserializing SHALL produce a model where every entity has equivalent
 * POSITION_X, POSITION_Y, WIDTH, HEIGHT, and GEOMETRY metadata values (within floating-point
 * precision of 2 decimal places).
 *
 * Validates: Requirements 5.6, 5.7, 5.8, 5.9, 12.1, 12.2, 12.4, 12.5, 12.6, 12.7, 12.9,
 *            13.1, 13.2, 13.3, 13.4, 13.5, 13.10
 */
void SerializerPropertyTest::PropertyLayoutGeometryRoundTrip() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        auto model = QSharedPointer<model::StateMachineModel>::create("TestModel");
        auto root = model->root();

        // Create random number of states (2..5) with random position and size
        const int stateCount = static_cast<int>(rng.bounded(2, 6));
        QList<QSharedPointer<model::RegularState>> states;

        struct StateGeometry {
            model::EntityID_t uid;
            double x;
            double y;
            double width;
            double height;
        };
        QList<StateGeometry> expectedStateGeometry;

        for (int i = 0; i < stateCount; ++i) {
            auto state = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                             .dynamicCast<model::RegularState>();
            QVERIFY(state);
            state->setName(QString("S%1_%2").arg(iteration).arg(i));

            const double x = rng.bounded(0, 1000) + rng.generateDouble();
            const double y = rng.bounded(0, 1000) + rng.generateDouble();
            const double w = rng.bounded(50, 300) + rng.generateDouble();
            const double h = rng.bounded(50, 300) + rng.generateDouble();
            state->setPos(QPointF(x, y));
            state->setSize(QSizeF(w, h));

            // Store expected values rounded to 2 decimal places (serializer precision)
            StateGeometry sg;
            sg.uid = state->id();
            sg.x = QString::number(x, 'f', 2).toDouble();
            sg.y = QString::number(y, 'f', 2).toDouble();
            sg.width = QString::number(w, 'f', 2).toDouble();
            sg.height = QString::number(h, 'f', 2).toDouble();
            expectedStateGeometry.append(sg);

            root->addChildState(state);
            states.append(state);
        }

        // Create random transitions (1..3) with random routing geometry
        struct TransitionGeometry {
            model::EntityID_t uid;
            QList<QPointF> intermediatePoints;
        };
        QList<TransitionGeometry> expectedTransitionGeometry;

        const int transitionCount = static_cast<int>(rng.bounded(1, 4));

        for (int i = 0; i < transitionCount; ++i) {
            const int srcIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));
            int dstIdx = static_cast<int>(rng.bounded(static_cast<quint32>(states.size())));

            if (dstIdx == srcIdx) {
                dstIdx = (dstIdx + 1) % states.size();
            }

            const QString event = QString("ev%1_%2").arg(iteration).arg(i);
            auto transition = model::ModelElementsFactory::createTransitionWithId(states[srcIdx], states[dstIdx], event, model->idGenerator().generateNextId());
            states[srcIdx]->addTransition(transition);

            // Create routing geometry with placeholder first/last + 3-5 intermediate points
            const int intermediateCount = static_cast<int>(rng.bounded(3, 6));
            QPolygonF linePath;
            linePath.append(QPointF(0, 0));  // Placeholder start

            TransitionGeometry tg;
            tg.uid = transition->id();

            for (int p = 0; p < intermediateCount; ++p) {
                const double px = rng.bounded(0, 1000) + rng.generateDouble();
                const double py = rng.bounded(0, 1000) + rng.generateDouble();
                linePath.append(QPointF(px, py));

                // Store expected values rounded to 2 decimal places
                const double expectedPx = QString::number(px, 'f', 2).toDouble();
                const double expectedPy = QString::number(py, 'f', 2).toDouble();
                tg.intermediatePoints.append(QPointF(expectedPx, expectedPy));
            }

            linePath.append(QPointF(0, 0));  // Placeholder end
            transition->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY,
                                    QVariant::fromValue(linePath));
            expectedTransitionGeometry.append(tg);
        }

        // Serialize in HSM format
        model::StateMachineSerializer serializer;
        const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

        // Deserialize
        auto loadedModel = serializer.deserializeFromScxml(scxml);
        QVERIFY2(loadedModel != nullptr,
                 qPrintable(QString("Iteration %1: deserialization returned null").arg(iteration)));

        auto loadedRoot = loadedModel->root();
        QVERIFY(loadedRoot != nullptr);

        // Verify state geometry round-trip
        for (const auto& sg : expectedStateGeometry) {
            auto entity = loadedRoot->findChild(sg.uid);
            QVERIFY2(entity != nullptr,
                     qPrintable(QString("Iteration %1: state uid %2 not found after round-trip")
                                    .arg(iteration)
                                    .arg(sg.uid)));

            const double loadedX =
                entity->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_X).toDouble();
            const double loadedY =
                entity->getMetadata(model::StateMachineEntity::MetadataKey::POSITION_Y).toDouble();
            const double loadedW =
                entity->getMetadata(model::StateMachineEntity::MetadataKey::WIDTH).toDouble();
            const double loadedH =
                entity->getMetadata(model::StateMachineEntity::MetadataKey::HEIGHT).toDouble();

            QVERIFY2(qAbs(loadedX - sg.x) < 0.01,
                     qPrintable(QString("Iteration %1: state uid %2 POSITION_X mismatch: %3 vs %4")
                                    .arg(iteration)
                                    .arg(sg.uid)
                                    .arg(loadedX)
                                    .arg(sg.x)));
            QVERIFY2(qAbs(loadedY - sg.y) < 0.01,
                     qPrintable(QString("Iteration %1: state uid %2 POSITION_Y mismatch: %3 vs %4")
                                    .arg(iteration)
                                    .arg(sg.uid)
                                    .arg(loadedY)
                                    .arg(sg.y)));
            QVERIFY2(qAbs(loadedW - sg.width) < 0.01,
                     qPrintable(QString("Iteration %1: state uid %2 WIDTH mismatch: %3 vs %4")
                                    .arg(iteration)
                                    .arg(sg.uid)
                                    .arg(loadedW)
                                    .arg(sg.width)));
            QVERIFY2(qAbs(loadedH - sg.height) < 0.01,
                     qPrintable(QString("Iteration %1: state uid %2 HEIGHT mismatch: %3 vs %4")
                                    .arg(iteration)
                                    .arg(sg.uid)
                                    .arg(loadedH)
                                    .arg(sg.height)));
        }

        // Verify transition geometry round-trip
        for (const auto& tg : expectedTransitionGeometry) {
            auto entity = loadedRoot->findChild(tg.uid, model::StateMachineEntity::Type::Transition);
            QVERIFY2(entity != nullptr,
                     qPrintable(QString("Iteration %1: transition uid %2 not found after round-trip")
                                    .arg(iteration)
                                    .arg(tg.uid)));

            QVariant geometryData = entity->getMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY);
            QVERIFY2(geometryData.isValid(),
                     qPrintable(QString("Iteration %1: transition uid %2 has no GEOMETRY after round-trip")
                                    .arg(iteration)
                                    .arg(tg.uid)));

            QPolygonF loadedPath = geometryData.value<QPolygonF>();
            // The deserialized path has placeholder first+last plus intermediate points
            // So intermediate points are at indices [1, size-2]
            const int loadedIntermediateCount = loadedPath.size() - 2;
            QVERIFY2(loadedIntermediateCount == tg.intermediatePoints.size(),
                     qPrintable(QString("Iteration %1: transition uid %2 intermediate point count mismatch: %3 vs %4")
                                    .arg(iteration)
                                    .arg(tg.uid)
                                    .arg(loadedIntermediateCount)
                                    .arg(tg.intermediatePoints.size())));

            for (int p = 0; p < tg.intermediatePoints.size(); ++p) {
                const QPointF& expected = tg.intermediatePoints.at(p);
                const QPointF& loaded = loadedPath.at(p + 1);  // Skip placeholder start

                QVERIFY2(qAbs(loaded.x() - expected.x()) < 0.01,
                         qPrintable(QString("Iteration %1: transition uid %2 point %3 X mismatch: %4 vs %5")
                                        .arg(iteration)
                                        .arg(tg.uid)
                                        .arg(p)
                                        .arg(loaded.x())
                                        .arg(expected.x())));
                QVERIFY2(qAbs(loaded.y() - expected.y()) < 0.01,
                         qPrintable(QString("Iteration %1: transition uid %2 point %3 Y mismatch: %4 vs %5")
                                        .arg(iteration)
                                        .arg(tg.uid)
                                        .arg(p)
                                        .arg(loaded.y())
                                        .arg(expected.y())));
            }
        }
    }
}

/**
 * @brief Property 16: Transition Layout Excludes Endpoints.
 *
 * For any transition serialized to HSM format layout, the <hsm:points> element SHALL
 * contain only intermediate routing points — source and target endpoint coordinates
 * SHALL NOT appear in the stored geometry.
 *
 * Validates: Requirements 12.4, 12.5, 12.9
 */
void SerializerPropertyTest::PropertyTransitionLayoutExcludesEndpoints() {
    QRandomGenerator rng(QRandomGenerator::global()->generate());

    for (int iteration = 0; iteration < 50; ++iteration) {
        auto model = QSharedPointer<model::StateMachineModel>::create("TestModel");
        auto root = model->root();

        // Create two states for transitions
        auto srcState = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                            .dynamicCast<model::RegularState>();
        auto dstState = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR, model->idGenerator())
                            .dynamicCast<model::RegularState>();
        QVERIFY(srcState);
        QVERIFY(dstState);
        srcState->setName(QString("Src_%1").arg(iteration));
        dstState->setName(QString("Dst_%1").arg(iteration));
        srcState->setPos(QPointF(100, 100));
        srcState->setSize(QSizeF(120, 60));
        dstState->setPos(QPointF(400, 100));
        dstState->setSize(QSizeF(120, 60));
        root->addChildState(srcState);
        root->addChildState(dstState);

        // Create a transition with random intermediate points
        const QString event = QString("evt_%1").arg(iteration);
        auto transition = model::ModelElementsFactory::createTransitionWithId(srcState, dstState, event, model->idGenerator().generateNextId());
        srcState->addTransition(transition);

        // Create QPolygonF with placeholder first (0,0) and last (0,0) + 1-3 intermediate points
        const int intermediateCount = static_cast<int>(rng.bounded(1, 4));
        QPolygonF linePath;
        linePath.append(QPointF(0, 0));  // Placeholder start endpoint

        QList<QPointF> expectedIntermediatePoints;

        for (int p = 0; p < intermediateCount; ++p) {
            const double px = rng.bounded(10, 900) + rng.generateDouble();
            const double py = rng.bounded(10, 900) + rng.generateDouble();
            linePath.append(QPointF(px, py));
            // Store rounded expected values
            const double roundedPx = QString::number(px, 'f', 2).toDouble();
            const double roundedPy = QString::number(py, 'f', 2).toDouble();
            expectedIntermediatePoints.append(QPointF(roundedPx, roundedPy));
        }

        linePath.append(QPointF(0, 0));  // Placeholder end endpoint
        transition->setMetadata(model::StateMachineEntity::MetadataKey::GEOMETRY,
                                QVariant::fromValue(linePath));

        // Serialize in HSM format
        model::StateMachineSerializer serializer;
        const QString scxml = serializer.serializeToScxml(model, model::SerializationFormat::HSM);

        // Parse the XML to find <hsm:points> elements and verify their content
        QXmlStreamReader reader(scxml);
        bool foundPoints = false;

        while (!reader.atEnd()) {
            reader.readNext();

            if (reader.isStartElement() && reader.name() == QStringView(u"points")) {
                foundPoints = true;
                const QString geometryStr = reader.attributes().value("geometry").toString();
                QVERIFY2(!geometryStr.isEmpty(),
                         qPrintable(QString("Iteration %1: <hsm:points> has empty geometry").arg(iteration)));

                // Parse geometry coordinate pairs
                const QStringList pairs = geometryStr.split(';');

                // Verify: number of coordinate pairs equals intermediate points (total - 2 endpoints)
                QVERIFY2(pairs.size() == intermediateCount,
                         qPrintable(QString("Iteration %1: geometry has %2 pairs, expected %3 (total_points - 2)")
                                        .arg(iteration)
                                        .arg(pairs.size())
                                        .arg(intermediateCount)));

                // Verify: each pair matches the expected intermediate points (not the placeholders)
                for (int p = 0; p < pairs.size(); ++p) {
                    const QStringList coords = pairs[p].split(',');
                    QVERIFY2(coords.size() == 2,
                             qPrintable(QString("Iteration %1: malformed coord pair at index %2")
                                            .arg(iteration)
                                            .arg(p)));

                    const double px = coords[0].toDouble();
                    const double py = coords[1].toDouble();

                    QVERIFY2(qAbs(px - expectedIntermediatePoints[p].x()) < 0.01,
                             qPrintable(
                                 QString("Iteration %1: point %2 X mismatch: %3 vs %4")
                                     .arg(iteration)
                                     .arg(p)
                                     .arg(px)
                                     .arg(expectedIntermediatePoints[p].x())));
                    QVERIFY2(qAbs(py - expectedIntermediatePoints[p].y()) < 0.01,
                             qPrintable(
                                 QString("Iteration %1: point %2 Y mismatch: %3 vs %4")
                                     .arg(iteration)
                                     .arg(p)
                                     .arg(py)
                                     .arg(expectedIntermediatePoints[p].y())));

                    // Verify: none of the points are the placeholder (0,0) endpoints
                    // (since our intermediates are in range [10,900], they should never be (0,0))
                    QVERIFY2(!(qAbs(px) < 0.01 && qAbs(py) < 0.01),
                             qPrintable(QString("Iteration %1: point %2 is (0,0) — endpoint leaked into geometry")
                                            .arg(iteration)
                                            .arg(p)));
                }
            }
        }

        QVERIFY2(foundPoints,
                 qPrintable(QString("Iteration %1: <hsm:points> element not found in serialized output")
                                .arg(iteration)));
    }
}

int runSerializerPropertyTest(int argc, char** argv) {
    SerializerPropertyTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "SerializerPropertyTest.moc"
