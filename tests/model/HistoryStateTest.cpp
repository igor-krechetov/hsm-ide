#include "../QtTestCompat.hpp"

#include "model/HistoryState.hpp"
#include "model/RegularState.hpp"
#include "model/Transition.hpp"

namespace {

/**
 * @brief Verify history state stores a default transition and history type property.
 *
 * Use-case: SCXML history node has default fallback transition and configurable depth.
 *
 */
TEST(HistoryStateTest, DefaultTransitionAndProperties) {
    auto source = QSharedPointer<model::RegularState>::create("S");
    auto target = QSharedPointer<model::RegularState>::create("T");
    auto history = QSharedPointer<model::HistoryState>::create("H", model::HistoryType::SHALLOW);
    auto tr = QSharedPointer<model::Transition>::create(source, target, "resume");

    EXPECT_TRUE(history->addChild(tr));
    EXPECT_EQ(tr, history->defaultTransition());

    EXPECT_TRUE(history->setProperty(model::HistoryState::cKeyHistoryType, static_cast<int>(model::HistoryType::DEEP)));
    EXPECT_EQ(model::HistoryType::DEEP, history->historyType());

    history->deleteChild(tr->id());
    EXPECT_EQ(nullptr, history->defaultTransition());
}

}  // namespace
