#include "UnsavedChangesGuard.hpp"

QList<UnsavedChangesGuard::GuardedProject> UnsavedChangesGuard::modifiedProjects(const QList<GuardedProject>& projects) {
    QList<GuardedProject> result;

    for (const GuardedProject& project : projects) {
        if (project.isModified) {
            result.append(project);
        }
    }

    return result;
}

bool UnsavedChangesGuard::promptRequired(const QList<GuardedProject>& projects) {
    bool required = false;

    for (const GuardedProject& project : projects) {
        if (project.isModified) {
            required = true;
            break;
        }
    }

    return required;
}

QStringList UnsavedChangesGuard::affectedProjectNames(const QList<GuardedProject>& projects) {
    QStringList names;

    for (const GuardedProject& project : projects) {
        if (project.isModified) {
            names.append(project.name);
        }
    }

    return names;
}

UnsavedChangesGuard::Decision UnsavedChangesGuard::resolve(const QList<GuardedProject>& projects, Choice choice) {
    Decision decision;

    switch (choice) {
        case Choice::Cancel:
            decision.proceedWithClose = false;
            break;

        case Choice::Discard:
            decision.proceedWithClose = true;
            break;

        case Choice::SaveAll:
            decision.proceedWithClose = true;

            for (const GuardedProject& project : projects) {
                if (project.isModified) {
                    decision.projectsToSave.append(project.handle);
                }
            }
            break;
    }

    return decision;
}
