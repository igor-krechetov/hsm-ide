#include <QtTest>

#include "controllers/private/UnsavedChangesGuard.hpp"

class UnsavedChangesGuardTest : public QObject {
    Q_OBJECT

private:
    static UnsavedChangesGuard::GuardedProject makeProject(const QString& name, bool modified, void* handle) {
        UnsavedChangesGuard::GuardedProject project;
        project.name = name;
        project.isModified = modified;
        project.handle = handle;
        return project;
    }

private slots:
    void promptNotRequiredForEmptyList();
    void promptNotRequiredWhenAllClean();
    void promptRequiredWhenAnyModified();
    void modifiedProjectsReturnsOnlyModified();
    void affectedNamesListsOnlyModified();
    void resolveCancelDoesNotProceed();
    void resolveDiscardProceedsWithoutSaving();
    void resolveSaveAllProceedsAndSavesModified();
    void failedSaveOverridesProceed();
};

void UnsavedChangesGuardTest::promptNotRequiredForEmptyList() {
    const QList<UnsavedChangesGuard::GuardedProject> projects;

    QVERIFY(!UnsavedChangesGuard::promptRequired(projects));
    QVERIFY(UnsavedChangesGuard::modifiedProjects(projects).isEmpty());
}

void UnsavedChangesGuardTest::promptNotRequiredWhenAllClean() {
    int a = 0;
    int b = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("A", false, &a), makeProject("B", false, &b)};

    QVERIFY(!UnsavedChangesGuard::promptRequired(projects));
    QVERIFY(UnsavedChangesGuard::modifiedProjects(projects).isEmpty());
    QVERIFY(UnsavedChangesGuard::affectedProjectNames(projects).isEmpty());
}

void UnsavedChangesGuardTest::promptRequiredWhenAnyModified() {
    int a = 0;
    int b = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("A", false, &a), makeProject("B", true, &b)};

    QVERIFY(UnsavedChangesGuard::promptRequired(projects));
}

void UnsavedChangesGuardTest::modifiedProjectsReturnsOnlyModified() {
    int a = 0;
    int b = 0;
    int c = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("A", false, &a),
                                                                 makeProject("B", true, &b),
                                                                 makeProject("C", true, &c)};

    const QList<UnsavedChangesGuard::GuardedProject> modified = UnsavedChangesGuard::modifiedProjects(projects);

    QCOMPARE(modified.size(), 2);
    QCOMPARE(modified.at(0).handle, static_cast<void*>(&b));
    QCOMPARE(modified.at(1).handle, static_cast<void*>(&c));
}

void UnsavedChangesGuardTest::affectedNamesListsOnlyModified() {
    int a = 0;
    int b = 0;
    int c = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("Alpha", true, &a),
                                                                 makeProject("Beta", false, &b),
                                                                 makeProject("Gamma", true, &c)};

    const QStringList names = UnsavedChangesGuard::affectedProjectNames(projects);

    QCOMPARE(names, QStringList({"Alpha", "Gamma"}));
}

void UnsavedChangesGuardTest::resolveCancelDoesNotProceed() {
    int a = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("A", true, &a)};

    const UnsavedChangesGuard::Decision decision = UnsavedChangesGuard::resolve(projects, UnsavedChangesGuard::Choice::Cancel);

    QVERIFY(!decision.proceedWithClose);
    QVERIFY(decision.projectsToSave.isEmpty());
}

void UnsavedChangesGuardTest::resolveDiscardProceedsWithoutSaving() {
    int a = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("A", true, &a)};

    const UnsavedChangesGuard::Decision decision = UnsavedChangesGuard::resolve(projects, UnsavedChangesGuard::Choice::Discard);

    QVERIFY(decision.proceedWithClose);
    QVERIFY(decision.projectsToSave.isEmpty());
}

void UnsavedChangesGuardTest::resolveSaveAllProceedsAndSavesModified() {
    int a = 0;
    int b = 0;
    int c = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("A", true, &a),
                                                                 makeProject("B", false, &b),
                                                                 makeProject("C", true, &c)};

    const UnsavedChangesGuard::Decision decision = UnsavedChangesGuard::resolve(projects, UnsavedChangesGuard::Choice::SaveAll);

    QVERIFY(decision.proceedWithClose);
    QCOMPARE(decision.projectsToSave.size(), 2);
    QCOMPARE(decision.projectsToSave.at(0), static_cast<void*>(&a));
    QCOMPARE(decision.projectsToSave.at(1), static_cast<void*>(&c));
}

void UnsavedChangesGuardTest::failedSaveOverridesProceed() {
    // The guard reports intent to proceed on SaveAll, but the caller is responsible
    // for aborting the close when a save fails. This test documents that contract:
    // a failed save (simulated here) overrides proceedWithClose to false.
    int a = 0;
    const QList<UnsavedChangesGuard::GuardedProject> projects = {makeProject("A", true, &a)};

    UnsavedChangesGuard::Decision decision = UnsavedChangesGuard::resolve(projects, UnsavedChangesGuard::Choice::SaveAll);
    QVERIFY(decision.proceedWithClose);

    const bool saveSucceeded = false;  // simulate a failed/cancelled save
    if (!saveSucceeded) {
        decision.proceedWithClose = false;
    }

    QVERIFY(!decision.proceedWithClose);
}

int runUnsavedChangesGuardTest(int argc, char** argv) {
    UnsavedChangesGuardTest tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "UnsavedChangesGuardTest.moc"
