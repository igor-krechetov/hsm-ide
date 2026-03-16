#ifndef THEMEMANAGER_HPP
#define THEMEMANAGER_HPP

#include <QObject>

#include "Theme.hpp"

class ThemeManager : public QObject {
    Q_OBJECT

public:
    static ThemeManager& instance();

    const view::theme::Theme& theme() const {
        return mActiveTheme;
    }

    void setTheme(const view::theme::Theme& theme);

signals:
    void themeChanged();

private:
    ThemeManager();
    static view::theme::Theme createDefaultTheme();

private:
    view::theme::Theme mActiveTheme;
};

#endif  // THEMEMANAGER_HPP
