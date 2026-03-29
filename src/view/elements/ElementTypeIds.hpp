#ifndef ELEMENTTYPEIDS_HPP
#define ELEMENTTYPEIDS_HPP

#include <QGraphicsItem>

#include "private/HsmElement.hpp"

namespace view {
// For HsmElement-based items: UserType + HsmElementType
constexpr int HSM_ELEMENT_TYPE_BASE = QGraphicsItem::UserType;
constexpr int HSM_ELEMENT_TYPE_INITIAL = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::INITIAL);
constexpr int HSM_ELEMENT_TYPE_FINAL = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::FINAL);
constexpr int HSM_ELEMENT_TYPE_ENTRY_POINT = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::ENTRY_POINT);
constexpr int HSM_ELEMENT_TYPE_EXIT_POINT = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::EXIT_POINT);
constexpr int HSM_ELEMENT_TYPE_STATE = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::STATE);
constexpr int HSM_ELEMENT_TYPE_TRANSITION = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::TRANSITION);
constexpr int HSM_ELEMENT_TYPE_HISTORY = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::HISTORY);
constexpr int HSM_ELEMENT_TYPE_INCLUDE = HSM_ELEMENT_TYPE_BASE + static_cast<int>(HsmElementType::INCLUDE);

// For non-HsmElement-based items, assign unique IDs
constexpr int ELEMENT_TYPE_HELPERS_BASE = QGraphicsItem::UserType + 1000;
constexpr int ELEMENT_TYPE_AUTOGROUP = ELEMENT_TYPE_HELPERS_BASE;
constexpr int ELEMENT_TYPE_CONNECTION_ARROW = QGraphicsItem::UserType + 1001;
constexpr int ELEMENT_TYPE_GRIP = QGraphicsItem::UserType + 1002;
constexpr int ELEMENT_TYPE_BOUNDARY_GRIP = QGraphicsItem::UserType + 1003;
constexpr int ELEMENT_TYPE_STATE_BODY_SECTION = QGraphicsItem::UserType + 1004;
constexpr int ELEMENT_TYPE_STATE_TEXT = QGraphicsItem::UserType + 1005;

// Add more as needed

}  // namespace view

#define IS_HSM_ELEMENT_TYPE(_type) (((_type) >= view::HSM_ELEMENT_TYPE_BASE) && ((_type) < view::ELEMENT_TYPE_HELPERS_BASE))

#endif  // ELEMENTTYPEIDS_HPP
