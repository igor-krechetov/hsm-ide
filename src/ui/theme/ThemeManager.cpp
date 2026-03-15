#include "ThemeManager.hpp"

ThemeManager& ThemeManager::instance() {
    static ThemeManager manager;

    return manager;
}

void ThemeManager::setTheme(const view::theme::Theme& theme) {
    mActiveTheme = theme;
    emit themeChanged();
}

ThemeManager::ThemeManager()
    : QObject(nullptr) {
    mActiveTheme = createDefaultTheme();
}

view::theme::Theme ThemeManager::createDefaultTheme() {
    view::theme::Theme defaultTheme;

    defaultTheme.node.borderPen = QPen(QColor("#5A80A8"), 2.0, Qt::SolidLine);
    defaultTheme.node.highlightBorderPen = QPen(QColor("#2E75C8"), 3.0, Qt::DotLine);
    defaultTheme.node.selectedBorderPen = QPen(QColor("#1E62D0"), 3.0, Qt::DotLine);
    defaultTheme.node.backgroundBrush = QBrush(QColor("#E8F1FA"));
    defaultTheme.node.substateBackgroundBrush = QBrush(QColor("#DFF4E5"));
    defaultTheme.node.includeBackgroundBrush = QBrush(QColor("#F5F0D8"));
    defaultTheme.node.mainBrush = QBrush(QColor("#1A1A1A"));
    defaultTheme.node.textColor = QColor("#1A1A1A");
    defaultTheme.node.editingBackgroundBrush = QBrush(QColor(255, 255, 200));
    defaultTheme.node.labelFont.setBold(true);

    defaultTheme.transition.pen = QPen(QColor("#1A1A1A"), 1.0, Qt::SolidLine);
    defaultTheme.transition.selectedPen = QPen(QColor("#1A1A1A"), 1.0, Qt::DashLine);
    defaultTheme.transition.debugSelectionPen = QPen(QColor("red"));

    defaultTheme.grid.majorLinePen = QPen(Qt::lightGray, 1.0, Qt::DashLine);
    defaultTheme.grid.minorLinePen = QPen(Qt::lightGray, 1.0, Qt::DashLine);

    defaultTheme.grip.color = QColor("red");
    defaultTheme.grip.hoverColor = QColor("green");
    defaultTheme.grip.debugPen = QPen(Qt::black, 1.0, Qt::SolidLine);

    defaultTheme.connectionArrow.validBrush = QBrush(QColor("green"));
    defaultTheme.connectionArrow.invalidBrush = QBrush(QColor("red"));

    return defaultTheme;
}
