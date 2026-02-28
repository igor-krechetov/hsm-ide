#include "../QtTestCompat.hpp"

#include "model/ExitPoint.hpp"

namespace {

/**
 * @brief Verify exit point event/callback properties.
 *
 * Use-case: Exit point emits auto event and lifecycle callbacks.
 *
 */
TEST(ExitPointTest, EventAndCallbackProperties) {
    auto exit = QSharedPointer<model::ExitPoint>::create("XP");

    EXPECT_TRUE(exit->setProperty("event", "done"));
    EXPECT_TRUE(exit->setProperty("onEnteringCallback", "inCb"));
    EXPECT_TRUE(exit->setProperty("onExitingCallback", "outCb"));

    EXPECT_EQ(QString("done"), exit->event());
    EXPECT_EQ(QString("inCb"), exit->onEnteringCallback());
    EXPECT_EQ(QString("outCb"), exit->onExitingCallback());
}

}  // namespace
