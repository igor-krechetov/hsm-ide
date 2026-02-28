#include <gtest/gtest.h>

#include "model/ModelRootState.hpp"

namespace {

/**
 * @brief Verify model root exposes only the name property.
 *
 * Use-case: Root metadata editor allows renaming state machine only.
 *
 */
TEST(ModelRootStateTest, NameOnlyProperties) {
    auto root = QSharedPointer<model::ModelRootState>::create("Root");

    EXPECT_EQ(model::StateType::MODEL_ROOT, root->stateType());
    EXPECT_EQ(QStringList({"name"}), root->properties());

    EXPECT_TRUE(root->setProperty("name", "Renamed"));
    EXPECT_EQ(QString("Renamed"), root->getProperty("name").toString());
}

}  // namespace
