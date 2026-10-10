#ifndef UNSAVEDCHANGESGUARD_HPP
#define UNSAVEDCHANGESGUARD_HPP

#include <QList>
#include <QString>
#include <QStringList>

// GUI-free decision logic for the unsaved-changes close guard.
//
// This class deliberately contains no Qt widget code so it can be unit-tested
// without a running GUI. MainEditorController owns the orchestration (collecting
// projects and driving saves) and delegates dialog presentation to the view; this
// class only reports which projects are at risk and maps a user choice to the
// actions the caller must take.
class UnsavedChangesGuard {
public:
    // A read-only view of what the guard needs to know about a project.
    struct GuardedProject {
        QString name;     // display name for the prompt
        bool isModified;  // true if it has unsaved changes
        void* handle;     // opaque identity passed back in the Decision (e.g. ProjectController*)
    };

    enum class Choice { SaveAll, Discard, Cancel };

    struct Decision {
        bool proceedWithClose = false;  // false => abort, keep everything open
        QList<void*> projectsToSave;    // handles the caller must save first
    };

    // Convenience accessor returning the subset of projects that would lose unsaved
    // changes. Not used by the orchestration flow (which uses promptRequired +
    // affectedProjectNames + resolve); provided for callers and tests that want the
    // modified projects directly.
    static QList<GuardedProject> modifiedProjects(const QList<GuardedProject>& projects);

    // True if a prompt is required (i.e. at least one project is modified).
    static bool promptRequired(const QList<GuardedProject>& projects);

    // Builds the list of affected (modified) project names for the prompt body.
    static QStringList affectedProjectNames(const QList<GuardedProject>& projects);

    // Maps a user choice to the actions the caller must take.
    static Decision resolve(const QList<GuardedProject>& projects, Choice choice);
};

#endif  // UNSAVEDCHANGESGUARD_HPP
