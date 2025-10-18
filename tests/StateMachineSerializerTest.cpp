

#include <QtTest>
#include <QCoreApplication>
#include "model/StateMachineSerializer.hpp"
#include "model/StateMachineModel.hpp"
#include "model/ModelElementsFactory.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

class StateMachineSerializerTest : public QObject
{
    Q_OBJECT

public:
    StateMachineSerializerTest();
    ~StateMachineSerializerTest();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSerialization();
    void testDeserialization();
    void testValidation();

private:
    model::StateMachineSerializer* serializer;
};

StateMachineSerializerTest::StateMachineSerializerTest()
    : serializer(new model::StateMachineSerializer())
{
}

StateMachineSerializerTest::~StateMachineSerializerTest()
{
    delete serializer;
}

void StateMachineSerializerTest::initTestCase()
{
    // Initialize resources if needed
}

void StateMachineSerializerTest::cleanupTestCase()
{
    // Clean up resources if needed
}

void StateMachineSerializerTest::testSerialization()
{
    // Create a simple state machine model
    QSharedPointer<model::StateMachineModel> model(new model::StateMachineModel("TestMachine"));

    // Get the root state
    QSharedPointer<model::RegularState> rootState = model->root();

    // Create a child state
    QSharedPointer<model::RegularState> childState1 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    childState1->setName("ChildState1");
    rootState->addChildState(childState1);

    QSharedPointer<model::RegularState> childState2 = model::ModelElementsFactory::createUniqueState(model::StateType::REGULAR).dynamicCast<model::RegularState>();
    childState2->setName("ChildState2");
    rootState->addChildState(childState2);

    // Create a transition
    QSharedPointer<model::Transition> transition = model::ModelElementsFactory::createUniqueTransition(childState1, childState2);
    transition->setEvent("event1");
    // childState1->addTransition(transition);

    // Serialize the model
    QString scxml = serializer->serializeToScxml(model);

    // Check if serialization was successful
    QVERIFY(!scxml.isEmpty());
    QVERIFY(scxml.contains("scxml"));
    QVERIFY(scxml.contains("state"));
    QVERIFY(scxml.contains("transition"));
    qDebug() << scxml;
}

void StateMachineSerializerTest::testDeserialization()
{
    // Create a simple SCXML string
    QString scxml = R"(
        <scxml xmlns="http://www.w3.org/2005/07/scxml" version="1.0" name="TestMachine">
            <state id="RootState">
                <state id="ChildState"/>
                <transition event="event1" target="ChildState"/>
            </state>
        </scxml>
    )";

    // Deserialize the SCXML
    QSharedPointer<model::StateMachineModel> modelPtr = serializer->deserializeFromScxml(scxml);

    // Check if deserialization was successful
    QVERIFY(modelPtr);
    QVERIFY(modelPtr->name() == "TestMachine");

    // Check if the root state exists
    QSharedPointer<model::RegularState> rootState = modelPtr->root();
    QVERIFY(rootState);
    QVERIFY(rootState->name() == modelPtr->name());

    QCOMPARE(rootState->childrenEntities().size(), 1);
    // Check if the child state exists
    auto ptrChildStateL1 = rootState->childrenEntities().first();
    QVERIFY(ptrChildStateL1);
    QSharedPointer<model::RegularState> childStateL1 = ptrChildStateL1.dynamicCast<model::RegularState>();
    QVERIFY(childStateL1);
    QVERIFY(childStateL1->name() == "RootState");

    QCOMPARE(childStateL1->childrenEntities().size(), 2);
    auto ptrChildStateL2 = childStateL1->childrenEntities().first();
    QVERIFY(ptrChildStateL2);
    QSharedPointer<model::RegularState> childStateL2 = ptrChildStateL2.dynamicCast<model::RegularState>();
    QVERIFY(childStateL2);
    QVERIFY(childStateL2->name() == "ChildState");

    // Check if the transition exists
    QSharedPointer<model::Transition> transition = childStateL1->childrenEntities().last().dynamicCast<model::Transition>();
    QVERIFY(transition);
    QVERIFY(transition->event() == "event1");
    QVERIFY(transition->target()->name() == "ChildState");
}

void StateMachineSerializerTest::testValidation()
{
    // Create a valid SCXML string
    QString validScxml = R"(
        <scxml xmlns="http://www.w3.org/2005/07/scxml" version="1.0">
            <state id="RootState"/>
        </scxml>
    )";

    // Create an invalid SCXML string (missing xmlns attribute)
    QString invalidScxml = R"(
        <scxml version="1.0">
            <state id="RootState"/>
        </scxml>
    )";

    // Validate the SCXML strings
    QVERIFY(serializer->validateScxmlStructure(validScxml));
    QVERIFY(!serializer->validateScxmlStructure(invalidScxml));
}

QTEST_MAIN(StateMachineSerializerTest)

#include "StateMachineSerializerTest.moc"

