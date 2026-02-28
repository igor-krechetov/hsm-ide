#include "../QtTestCompat.hpp"

#include "model/FinalState.hpp"

namespace {

/**
 * @brief Verify final state callback property behavior.
 *
 * Use-case: Configure completion callback for final state entry.
 *
 */
TEST(FinalStateTest, CallbackPropertyRoundTrip) {
    auto state = QSharedPointer<model::FinalState>::create("Done");

    EXPECT_TRUE(state->setProperty("onStateChangedCallback", "completeCb"));
    EXPECT_EQ(QString("completeCb"), state->onStateChangedCallback());
    EXPECT_EQ(QString("completeCb"), state->getProperty("onStateChangedCallback").toString());
}

}  // namespace
