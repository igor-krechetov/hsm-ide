#ifndef IMETADATASERIALIZER_HPP
#define IMETADATASERIALIZER_HPP

#include <QSharedPointer>
#include <QString>
#include <QXmlStreamWriter>

#include "model/ModelTypes.hpp"

class QXmlStreamReader;

namespace model {

class StateMachineEntity;
class StateMachineModel;

namespace scxml {
constexpr char HSM_NAMESPACE_URI[] = "https://hsm-ide.dev/scxml";
constexpr char HSM_NAMESPACE_PREFIX[] = "hsm";
constexpr char HSM_UID_ATTR[] = "hsm:uid";

constexpr char QT_NAMESPACE_URI[] = "http://www.qt.io/2015/02/scxml-ext";
constexpr char QT_NAMESPACE_PREFIX[] = "qt";

constexpr char SCXML_NAMESPACE_URI[] = "http://www.w3.org/2005/07/scxml";
constexpr char XINCLUDE_NAMESPACE_URI[] = "http://www.w3.org/2001/XInclude";

// HSM editor section element names
constexpr char HSM_EDITOR_ELEMENT[] = "hsm:editor";
constexpr char HSM_LAYOUT_ELEMENT[] = "hsm:layout";
constexpr char HSM_METADATA_ELEMENT[] = "hsm:metadata";
constexpr char HSM_STATE_ELEMENT[] = "hsm:state";
constexpr char HSM_TRANSITION_ELEMENT[] = "hsm:transition";
constexpr char HSM_COMMENT_ELEMENT[] = "hsm:comment";
constexpr char HSM_POINTS_ELEMENT[] = "hsm:points";
constexpr char HSM_LABEL_ELEMENT[] = "hsm:label";
};  // namespace scxml

enum class SerializationFormat {
    HSM,       // hsm:uid attributes + <hsm:editor> section, no xmlns:qt
    QtCreator  // No hsm:uid, inline qt:editorinfo elements, no xmlns:hsm
};

enum class DetectedFormat {
    HSM,        // xmlns:hsm present on root <scxml>
    QtCreator,  // xmlns:qt present, no xmlns:hsm
    PlainSCXML  // Neither xmlns:hsm nor xmlns:qt present
};

/**
 * @brief Thin wrapper restricting XML writes to attributes only (compile-time safety).
 *
 * Prevents strategies from accidentally writing child elements during the attribute phase.
 */
class AttributeWriter {
public:
    explicit AttributeWriter(QXmlStreamWriter& writer)
        : mWriter(writer) {}

    inline void writeAttribute(const QString& name, const QString& value) {
        mWriter.writeAttribute(name, value);
    }

private:
    QXmlStreamWriter& mWriter;
};

/**
 * @brief Strategy interface for format-specific metadata serialization/deserialization.
 *
 * Separates HSM-format and Qt Creator-format logic from the core SCXML structure handling
 * in StateMachineSerializer. The SCXML model structure (states, transitions, actions) is
 * format-agnostic; only UID handling and geometry layout differ between formats.
 */
class IMetadataSerializer {
public:
    virtual ~IMetadataSerializer() = default;

    // --- Serialization ---

    /// Called once before entity traversal. Write format-specific namespace declarations.
    virtual void beginSerialization(QXmlStreamWriter& writer, const QSharedPointer<StateMachineModel>& model) = 0;

    /// Called when entering a state-like entity during tree traversal (before writing children).
    virtual void notifyEnterState(const StateMachineEntity* entity) = 0;

    /// Called when leaving a state-like entity during tree traversal (after children are written).
    virtual void notifyExitState(const StateMachineEntity* entity) = 0;

    /// Per-entity Phase 1: write additional attributes. No child elements allowed.
    virtual void writeEntityAttributes(AttributeWriter& writer, const StateMachineEntity* entity) = 0;

    /// Per-entity Phase 2: write child metadata elements. No attributes on parent allowed after this.
    virtual void writeEntityChildMetadata(QXmlStreamWriter& writer, const StateMachineEntity* entity) = 0;

    /// Called once after all entities serialized, before closing </scxml>.
    virtual void endSerialization(QXmlStreamWriter& writer, const QSharedPointer<StateMachineModel>& model) = 0;

    // --- Deserialization ---

    /// Inspect root <scxml> namespace declarations.
    virtual DetectedFormat detectFormat(const QXmlStreamReader& reader) const = 0;

    /// Per-entity: resolve the UID (parse from attribute or generate).
    virtual EntityID_t resolveEntityId(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) = 0;

    /// Try to consume an inline metadata child element during entity parsing.
    /// Returns true if consumed, false if unrecognized.
    virtual bool parseMetadataElement(QXmlStreamReader& reader,
                                      StateMachineEntity* entity,
                                      const QSharedPointer<StateMachineModel>& model) = 0;

    /// Try to consume a top-level element (direct child of <scxml>).
    /// Returns true if consumed, false if unrecognized.
    virtual bool parseTopLevelElement(QXmlStreamReader& reader, const QSharedPointer<StateMachineModel>& model) = 0;

    /// Called once after all elements parsed. Post-processing.
    virtual void endDeserialization(const QSharedPointer<StateMachineModel>& model) = 0;

    // --- Query ---
    virtual SerializationFormat format() const = 0;
};

}  // namespace model

#endif  // IMETADATASERIALIZER_HPP
