#include <gtest/gtest.h>

#include "model/ModelTypes.hpp"

namespace {

/**
 * @brief Verify transition type conversion helpers round-trip supported values.
 *
 * Use-case: Persist transition type in model properties and restore from integer values.
 *
 */
TEST(ModelTypesTest, TransitionTypeConversions) {
    EXPECT_EQ(model::TransitionType::EXTERNAL, model::transitionTypeFromInt(static_cast<int>(model::TransitionType::EXTERNAL)));
    EXPECT_EQ(model::TransitionType::INTERNAL, model::transitionTypeFromInt(static_cast<int>(model::TransitionType::INTERNAL)));
    EXPECT_EQ(model::TransitionType::INVALID, model::transitionTypeFromInt(777));

    EXPECT_EQ(QString("External"), model::transitionTypeToString(model::TransitionType::EXTERNAL));
    EXPECT_EQ(QString("Internal"), model::transitionTypeToString(model::TransitionType::INTERNAL));
    EXPECT_EQ(QString(""), model::transitionTypeToString(model::TransitionType::INVALID));
}

/**
 * @brief Verify history type conversion helpers round-trip supported values.
 *
 * Use-case: Persist history type in model properties and restore from integer values.
 *
 */
TEST(ModelTypesTest, HistoryTypeConversions) {
    EXPECT_EQ(model::HistoryType::SHALLOW, model::historyTypeFromInt(static_cast<int>(model::HistoryType::SHALLOW)));
    EXPECT_EQ(model::HistoryType::DEEP, model::historyTypeFromInt(static_cast<int>(model::HistoryType::DEEP)));
    EXPECT_EQ(model::HistoryType::INVALID, model::historyTypeFromInt(-8));

    EXPECT_EQ(QString("Shallow"), model::historyTypeToString(model::HistoryType::SHALLOW));
    EXPECT_EQ(QString("Deep"), model::historyTypeToString(model::HistoryType::DEEP));
    EXPECT_EQ(QString(""), model::historyTypeToString(model::HistoryType::INVALID));
}

}  // namespace
