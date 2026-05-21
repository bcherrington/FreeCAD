/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Library General Public License for more details.                      *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "CompactMainWindowChrome.h"
#include "CompactTitleBarStyle.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QDockWidget>
#include <QDirIterator>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLayoutItem>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QSet>
#include <QStatusBar>
#include <QStyle>
#include <QStyleOptionMenuItem>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>
#include <QWidgetAction>

#include <algorithm>
#include <functional>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>

#include "Action.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "DockWindowManager.h"
#include "Document.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "Macro.h"
#include "OverlayManager.h"

using namespace Gui;

namespace
{
constexpr auto CompactTopBarObjectName = "_fc_compact_top_bar";
constexpr auto CompactToolBarObjectName = "_fc_compact_tool_bar";
constexpr auto CompactMenuBarObjectName = "_fc_compact_menu_bar";
constexpr auto CompactLeftStripObjectName = "_fc_compact_left_panel_rail";
constexpr auto CompactRightStripObjectName = "_fc_compact_right_panel_rail";
constexpr auto CompactLegacyLeftStripObjectName = "_fc_compact_left_panel_strip";
constexpr auto CompactLegacyRightStripObjectName = "_fc_compact_right_panel_strip";
constexpr auto CompactLegacyBottomStripObjectName = "_fc_compact_bottom_panel_strip";
constexpr int CompactPanelStripMargin = 3;
constexpr int CompactResizeBorderWidth = 6;

QString trText(const char* text)
{
    return QCoreApplication::translate("Gui::CompactMainWindowChrome", text);
}

QPoint globalPosition(const QMouseEvent* event)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    return event->globalPos();
#else
    return event->globalPosition().toPoint();
#endif
}

QIcon hamburgerIcon(const QWidget* widget)
{
    const QPalette palette = widget ? widget->palette() : qApp->palette();
    const QColor background = palette.color(QPalette::Button);
    QColor foreground = palette.color(QPalette::ButtonText);

    if (std::abs(foreground.lightness() - background.lightness()) < 80) {
        foreground = background.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
    }

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(foreground, 2.2, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(2, 4), QPointF(14, 4));
    painter.drawLine(QPointF(2, 8), QPointF(14, 8));
    painter.drawLine(QPointF(2, 12), QPointF(14, 12));
    return QIcon(pixmap);
}

QIcon closeGlyphIcon(const QWidget* widget)
{
    const QPalette palette = widget ? widget->palette() : qApp->palette();
    const QColor background = palette.color(QPalette::Button);
    QColor foreground = palette.color(QPalette::ButtonText);

    if (std::abs(foreground.lightness() - background.lightness()) < 80) {
        foreground = background.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
    }

    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(foreground, 1.8, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(5, 5), QPointF(11, 11));
    painter.drawLine(QPointF(11, 5), QPointF(5, 11));
    return QIcon(pixmap);
}

class CompactDocumentMenuRow: public QWidget
{
public:
    CompactDocumentMenuRow(QString text, QIcon icon, QWidget* parent = nullptr)
        : QWidget(parent)
        , rowText(std::move(text))
        , rowIcon(std::move(icon))
    {}

    void setActivateCallback(std::function<void()> callback)
    {
        activateCallback = std::move(callback);
    }

protected:
    QSize sizeHint() const override
    {
        QStyleOptionMenuItem option;
        initStyleOption(&option);
        auto menu = qobject_cast<QMenu*>(parentWidget());
        QWidget* styleWidget = menu ? static_cast<QWidget*>(menu)
                                    : const_cast<CompactDocumentMenuRow*>(this);
        QSize size = styleWidget->style()
                         ->sizeFromContents(QStyle::CT_MenuItem, &option, QSize(), styleWidget);
        size.rwidth() += CloseButtonWidth;
        size.rheight() = std::max(size.height(), CloseButtonWidth + 6);
        return size;
    }

    void paintEvent(QPaintEvent*) override
    {
        QStyleOptionMenuItem option;
        initStyleOption(&option);
        if (underMouse()) {
            option.state |= QStyle::State_Selected;
        }

        QPainter painter(this);
        auto menu = qobject_cast<QMenu*>(parentWidget());
        QWidget* styleWidget = menu ? static_cast<QWidget*>(menu) : this;
        styleWidget->style()->drawControl(QStyle::CE_MenuItem, &option, &painter, styleWidget);
    }

    bool event(QEvent* event) override
    {
        if (event->type() == QEvent::Enter || event->type() == QEvent::Leave) {
            update();
        }

        return QWidget::event(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->pos()) && activateCallback) {
            activateCallback();
            event->accept();
            return;
        }

        QWidget::mouseReleaseEvent(event);
    }

private:
    void initStyleOption(QStyleOptionMenuItem* option) const
    {
        if (auto menu = qobject_cast<QMenu*>(parentWidget())) {
            option->initFrom(menu);
        }
        else {
            option->initFrom(this);
        }
        option->rect = rect();
        option->menuItemType = QStyleOptionMenuItem::Normal;
        option->checkType = QStyleOptionMenuItem::NotCheckable;
        option->checked = false;
        option->menuHasCheckableItems = false;
        option->text = rowText;
        option->icon = rowIcon;
        option->maxIconWidth = CompactTitleBarStyle::iconSize();
        option->reservedShortcutWidth = CloseButtonWidth;
        option->state |= QStyle::State_Enabled;
    }

    QString rowText;
    QIcon rowIcon;
    std::function<void()> activateCallback;
    static constexpr int CloseButtonWidth = 20;
};

QIcon documentIcon(const MDIView* view, const QWidget* fallbackWidget)
{
    QIcon icon;
    if (view) {
        icon = view->windowIcon();
    }
    if (icon.isNull() && fallbackWidget) {
        icon = fallbackWidget->style()->standardIcon(QStyle::SP_FileIcon);
    }
    return icon;
}

bool isCompactUiDock(const QDockWidget* dock)
{
    if (!dock) {
        return false;
    }

    const QString name = dock->objectName();
    return name == QLatin1String(CompactLegacyLeftStripObjectName)
        || name == QLatin1String(CompactLegacyRightStripObjectName)
        || name == QLatin1String(CompactLegacyBottomStripObjectName);
}

bool isCompactUiDockCandidate(const QDockWidget* dock)
{
    if (!dock || isCompactUiDock(dock)) {
        return false;
    }

    QAction* action = dock->toggleViewAction();
    return action && action->isVisible() && !dock->objectName().isEmpty();
}

int compactPanelStripWidth()
{
    return CompactTitleBarStyle::buttonSize(nullptr).width() + (2 * CompactPanelStripMargin);
}

QToolBar* createButtonToolBar(QWidget* parent, Qt::Orientation orientation)
{
    auto toolbar = new QToolBar(parent);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setOrientation(orientation);
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    const int iconSize = CompactTitleBarStyle::iconSize();
    toolbar->setIconSize(QSize(iconSize, iconSize));
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->layout()->setContentsMargins(0, 0, 0, 0);
    return toolbar;
}

void addToolBarStretch(QToolBar* toolbar)
{
    if (!toolbar) {
        return;
    }

    auto spacer = new QWidget(toolbar);
    if (toolbar->orientation() == Qt::Vertical) {
        spacer->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
    }
    else {
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    }
    toolbar->addWidget(spacer);
}

bool addCommandToMenu(QMenu* menu, const char* commandName)
{
    if (!menu || !commandName
        || !Gui::Application::Instance->commandManager().getCommandByName(commandName)) {
        return false;
    }

    return Gui::Application::Instance->commandManager().addTo(commandName, menu);
}

QAction* addCommandToToolBar(QToolBar* toolbar, const char* commandName)
{
    if (!toolbar || !commandName
        || !Gui::Application::Instance->commandManager().getCommandByName(commandName)) {
        return nullptr;
    }

    const auto actionCount = toolbar->actions().size();
    if (!Gui::Application::Instance->commandManager().addTo(commandName, toolbar)) {
        return nullptr;
    }

    const auto actions = toolbar->actions();
    if (actions.size() <= actionCount) {
        return nullptr;
    }

    auto action = actions.at(actionCount);
    if (auto button = qobject_cast<QToolButton*>(toolbar->widgetForAction(action))) {
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::StrongFocus);
        CompactTitleBarStyle::applyIconButtonMetrics(button, toolbar);
    }
    return action;
}

void addToolbarGap(QToolBar* toolbar, int width = CompactTitleBarStyle::groupGap())
{
    if (!toolbar) {
        return;
    }

    auto spacer = new QWidget(toolbar);
    spacer->setFixedWidth(width);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    toolbar->addWidget(spacer);
}

void addMenuHeader(QMenu* menu, const QString& text)
{
    if (!menu || text.isEmpty()) {
        return;
    }

    auto header = menu->addAction(text);
    header->setEnabled(false);
    QFont font = header->font();
    font.setBold(true);
    header->setFont(font);
}

QToolButton* addDropdownButtonToToolBar(
    QToolBar* toolbar,
    const QString& text,
    const QIcon& icon,
    QMenu* menu
)
{
    if (!toolbar || !menu) {
        return nullptr;
    }

    auto button = new QToolButton(toolbar);
    button->setText(text);
    button->setIcon(icon);
    button->setMenu(menu);
    button->setPopupMode(QToolButton::InstantPopup);
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::StrongFocus);
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    CompactTitleBarStyle::applyMenuButtonMetrics(button, toolbar);
    toolbar->addWidget(button);
    return button;
}

void connectRightAlignedPopup(QToolButton* button, QMenu* menu)
{
    if (!button || !menu) {
        return;
    }

    button->setMenu(nullptr);
    button->setPopupMode(QToolButton::DelayedPopup);
    QObject::connect(button, &QToolButton::clicked, button, [button, menu]() {
        const QSize menuSize = menu->sizeHint();
        const QPoint popupPosition = button->mapToGlobal(
            QPoint(button->width() - menuSize.width(), button->height())
        );
        menu->popup(popupPosition);
    });
}

QString documentDisplayName(const App::Document* document)
{
    if (!document) {
        return trText("No document");
    }

    const QString label = QString::fromUtf8(document->Label.getValue());
    return label.isEmpty() ? QString::fromUtf8(document->getName()) : label;
}

QString viewDisplayName(const MDIView* view)
{
    if (!view) {
        return trText("No document");
    }

    const QString title = view->windowTitle();
    if (!title.isEmpty()) {
        QString displayTitle = title;
        displayTitle.replace(
            QStringLiteral("[*]"),
            view->isWindowModified() ? QStringLiteral("*") : QString()
        );
        displayTitle.replace(QStringLiteral("&&"), QStringLiteral("&"));
        return displayTitle;
    }

    return documentDisplayName(view->getAppDocument());
}

void addDocumentViewRow(QMenu* menu, MainWindow* mainWindow, MDIView* view, bool active)
{
    if (!menu || !mainWindow || !view) {
        return;
    }

    const QString label = viewDisplayName(view);
    auto rowAction = new QWidgetAction(menu);
    auto row = new CompactDocumentMenuRow(label, documentIcon(view, mainWindow), menu);
    row->setMouseTracking(true);
    auto layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 3, 4, 3);
    layout->addStretch();
    if (active) {
        QFont font = row->font();
        font.setBold(true);
        row->setFont(font);
    }

    auto closeButton = new QToolButton(row);
    closeButton->setIcon(closeGlyphIcon(row));
    closeButton->setToolTip(trText("Close"));
    closeButton->setAutoRaise(true);
    closeButton->setFixedSize(QSize(20, 20));
    closeButton->setIconSize(QSize(
        std::min(CompactTitleBarStyle::iconSize(), 16),
        std::min(CompactTitleBarStyle::iconSize(), 16)
    ));
    closeButton->setStyleSheet(QStringLiteral(
        "QToolButton {"
        " border: none;"
        " background: transparent;"
        " padding: 2px;"
        "}"
        "QToolButton:hover {"
        " background: palette(button);"
        " border: 1px solid palette(mid);"
        " border-radius: 2px;"
        "}"
        "QToolButton:pressed { background: palette(mid); }"
    ));
    layout->addWidget(closeButton);

    QPointer<MDIView> guardedView(view);
    row->setActivateCallback([menu, mainWindow, guardedView]() {
        if (guardedView) {
            mainWindow->setActiveWindow(guardedView.data());
        }
        menu->close();
    });
    QObject::connect(closeButton, &QToolButton::clicked, row, [menu, mainWindow, guardedView]() {
        if (guardedView) {
            mainWindow->setActiveWindow(guardedView.data());
            mainWindow->closeActiveWindow();
        }
        menu->close();
    });

    rowAction->setDefaultWidget(row);
    menu->addAction(rowAction);
}

QStringList recentMacroFiles()
{
    QStringList files;

    if (auto command
        = Gui::Application::Instance->commandManager().getCommandByName("Std_RecentMacros")) {
        command->initAction();
        if (auto group = qobject_cast<ActionGroup*>(command->getAction())) {
            for (auto action : group->actions()) {
                if (action && action->isVisible() && !action->toolTip().isEmpty()) {
                    files.append(action->toolTip());
                }
            }
        }
    }

    return files;
}

QStringList allMacroFiles()
{
    const std::string macroPath
        = App::GetApplication()
              .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Macro")
              ->GetASCII("MacroPath", App::Application::getUserMacroDir().c_str());

    QStringList files;
    QDirIterator it(
        QString::fromUtf8(macroPath.c_str()),
        QStringList() << QStringLiteral("*.FCMacro") << QStringLiteral("*.py"),
        QDir::Files,
        QDirIterator::Subdirectories
    );

    while (it.hasNext()) {
        files.append(it.next());
    }

    files.sort(Qt::CaseInsensitive);
    return files;
}

QString macroDisplayName(const QString& filePath)
{
    const QFileInfo info(filePath);
    return info.completeBaseName();
}

QString editModeText(int mode)
{
    QString text = QCoreApplication::translate(
        "EditMode",
        Gui::Application::Instance->getUserEditModeUIStrings(mode).first.c_str()
    );
    return Action::cleanTitle(text);
}

QString editModeToolTip(int mode)
{
    return QCoreApplication::translate(
        "EditMode",
        Gui::Application::Instance->getUserEditModeUIStrings(mode).second.c_str()
    );
}

QIcon editModeIcon(int mode)
{
    QString modeName = QString::fromStdString(
        Gui::Application::Instance->getUserEditModeUIStrings(mode).first
    );
    modeName.remove(QLatin1Char('&'));

    QIcon icon = BitmapFactory().iconFromTheme(
        qPrintable(QStringLiteral("Std_UserEditMode") + modeName)
    );
    if (icon.isNull()) {
        icon = BitmapFactory().iconFromTheme("Std_UserEditModeDefault");
    }

    return icon;
}

void setEditMode(int mode)
{
    App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")
        ->SetInt("UserEditMode", mode);
    Gui::Application::Instance->setUserEditMode(mode);
}

bool addRecentFilesToMenu(QMenu* menu)
{
    if (!menu) {
        return false;
    }

    auto command = Gui::Application::Instance->commandManager().getCommandByName("Std_RecentFiles");
    if (!command) {
        return false;
    }

    command->initAction();
    auto recentFiles = qobject_cast<RecentFilesAction*>(command->getAction());
    if (!recentFiles) {
        return false;
    }

    bool addedRecentFile = false;
    QAction* clearAction = nullptr;
    for (auto action : recentFiles->actions()) {
        if (!action || !action->isVisible() || action->isSeparator()) {
            continue;
        }

        if (action->toolTip().isEmpty()) {
            if (!action->text().isEmpty()) {
                clearAction = action;
            }
            continue;
        }

        menu->addAction(action);
        addedRecentFile = true;
    }

    if (clearAction) {
        if (addedRecentFile) {
            menu->addSeparator();
        }
        menu->addAction(clearAction);
        return true;
    }

    return addedRecentFile;
}

bool isStandardTopLevelMenu(const QAction* action)
{
    if (!action) {
        return true;
    }

    const QString id = action->data().toString().isEmpty() ? action->objectName()
                                                           : action->data().toString();
    const QString title = Action::cleanTitle(action->text()).remove(QLatin1Char('&'));
    static const QSet<QString> standardMenus = {
        QStringLiteral(""),
        QStringLiteral("Separator"),
        QStringLiteral("&File"),
        QStringLiteral("&Edit"),
        QStringLiteral("&View"),
        QStringLiteral("&Tools"),
        QStringLiteral("&Macro"),
        QStringLiteral("&Windows"),
        QStringLiteral("&Help"),
        QStringLiteral("File"),
        QStringLiteral("Edit"),
        QStringLiteral("View"),
        QStringLiteral("Tools"),
        QStringLiteral("Macro"),
        QStringLiteral("Windows"),
        QStringLiteral("Help"),
    };

    return standardMenus.contains(id) || standardMenus.contains(title);
}

void copyMenuActions(const QMenu* source, QMenu* target)
{
    if (!source || !target) {
        return;
    }

    for (auto action : source->actions()) {
        if (!action || !action->isVisible()) {
            continue;
        }

        if (action->isSeparator()) {
            target->addSeparator();
            continue;
        }

        if (auto submenu = action->menu()) {
            auto copiedMenu = new QMenu(Action::cleanTitle(action->text()), target);
            copiedMenu->setIcon(action->icon());
            copyMenuActions(submenu, copiedMenu);
            target->addMenu(copiedMenu);
            continue;
        }

        auto copiedAction = target->addAction(action->icon(), Action::cleanTitle(action->text()));
        copiedAction->setEnabled(action->isEnabled());
        copiedAction->setCheckable(action->isCheckable());
        copiedAction->setChecked(action->isChecked());
        copiedAction->setToolTip(action->toolTip());
        copiedAction->setStatusTip(action->statusTip());
        copiedAction->setWhatsThis(action->whatsThis());
        QObject::connect(copiedAction, &QAction::triggered, action, [action]() { action->trigger(); });
    }
}

void populateWorkbenchMenu(QMenu* menu)
{
    if (!menu) {
        return;
    }

    menu->clear();

    if (auto command = Gui::Application::Instance->commandManager().getCommandByName("Std_Workbench")) {
        command->initAction();
        if (auto workbenchGroup = qobject_cast<WorkbenchGroup*>(command->getAction())) {
            menu->addActions(workbenchGroup->getEnabledWbActions());
        }
    }

    if (!menu->actions().isEmpty()) {
        menu->addSeparator();
    }
    addCommandToMenu(menu, "Std_AddonMgr");
}

void clearStrip(QWidget* content)
{
    if (!content || !content->layout()) {
        return;
    }

    QLayoutItem* item = nullptr;
    while ((item = content->layout()->takeAt(0)) != nullptr) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

void addStripSpacer(QToolBar* toolbar)
{
    addToolBarStretch(toolbar);
}

QWidget* createStrip(MainWindow* window, QWidget** content, const QString& objectName)
{
    auto container = new QFrame(window);
    container->setObjectName(objectName + QStringLiteral("Content"));
    container->setFixedWidth(compactPanelStripWidth());
    container->setFrameShape(QFrame::NoFrame);
    container->setAutoFillBackground(true);

    auto layout = new QVBoxLayout(container);
    layout->setContentsMargins(
        CompactPanelStripMargin,
        CompactPanelStripMargin,
        CompactPanelStripMargin,
        CompactPanelStripMargin
    );
    layout->setSpacing(CompactPanelStripMargin);

    container->hide();
    *content = container;
    return container;
}

QCursor cursorForEdges(Qt::Edges edges)
{
    if (edges == (Qt::LeftEdge | Qt::TopEdge) || edges == (Qt::RightEdge | Qt::BottomEdge)) {
        return Qt::SizeFDiagCursor;
    }
    if (edges == (Qt::RightEdge | Qt::TopEdge) || edges == (Qt::LeftEdge | Qt::BottomEdge)) {
        return Qt::SizeBDiagCursor;
    }
    if (edges & (Qt::LeftEdge | Qt::RightEdge)) {
        return Qt::SizeHorCursor;
    }
    return Qt::SizeVerCursor;
}

QRect resizedGeometry(
    const QRect& startGeometry,
    const QPoint& delta,
    Qt::Edges edges,
    const QSize& minimumSize,
    const QSize& maximumSize
)
{
    QRect geometry = startGeometry;
    if (edges & Qt::LeftEdge) {
        geometry.setLeft(geometry.left() + delta.x());
    }
    if (edges & Qt::RightEdge) {
        geometry.setRight(geometry.right() + delta.x());
    }
    if (edges & Qt::TopEdge) {
        geometry.setTop(geometry.top() + delta.y());
    }
    if (edges & Qt::BottomEdge) {
        geometry.setBottom(geometry.bottom() + delta.y());
    }

    const int minWidth = std::max(1, minimumSize.width());
    const int minHeight = std::max(1, minimumSize.height());
    const int maxWidth = maximumSize.width() > 0 ? maximumSize.width() : QWIDGETSIZE_MAX;
    const int maxHeight = maximumSize.height() > 0 ? maximumSize.height() : QWIDGETSIZE_MAX;

    if (geometry.width() < minWidth) {
        if (edges & Qt::LeftEdge) {
            geometry.setLeft(geometry.right() - minWidth + 1);
        }
        else {
            geometry.setRight(geometry.left() + minWidth - 1);
        }
    }
    if (geometry.height() < minHeight) {
        if (edges & Qt::TopEdge) {
            geometry.setTop(geometry.bottom() - minHeight + 1);
        }
        else {
            geometry.setBottom(geometry.top() + minHeight - 1);
        }
    }
    if (geometry.width() > maxWidth) {
        if (edges & Qt::LeftEdge) {
            geometry.setLeft(geometry.right() - maxWidth + 1);
        }
        else {
            geometry.setRight(geometry.left() + maxWidth - 1);
        }
    }
    if (geometry.height() > maxHeight) {
        if (edges & Qt::TopEdge) {
            geometry.setTop(geometry.bottom() - maxHeight + 1);
        }
        else {
            geometry.setBottom(geometry.top() + maxHeight - 1);
        }
    }

    return geometry;
}
}  // namespace

CompactMainWindowChrome::CompactMainWindowChrome(MainWindow* mainWindow)
    : QObject(mainWindow)
    , mainWindow(mainWindow)
{
    framelessWindow = mainWindow && (mainWindow->windowFlags() & Qt::FramelessWindowHint);
    setup();
}

CompactMainWindowChrome::~CompactMainWindowChrome()
{
    shuttingDown = true;
    clearWorkbenchMenuButtons();
    setGlobalEventFilterActive(false);
}

bool CompactMainWindowChrome::shouldUseFramelessWindow()
{
    auto group = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/MainWindow"
    );
    return group->GetBool("CompactJetBrainsLayout", false)
        && group->GetBool("CompactJetBrainsFramelessWindow", false);
}

void CompactMainWindowChrome::setup()
{
    if (!mainWindow || topBar) {
        return;
    }

    topBar = new QFrame(mainWindow);
    topBar->setObjectName(QLatin1String(CompactTopBarObjectName));
    topBar->setAutoFillBackground(true);
    topBar->installEventFilter(this);
    topBar->hide();

    auto topBarLayout = new QHBoxLayout(topBar);
    topBarLayout->setContentsMargins(4, 2, 4, 2);
    topBarLayout->setSpacing(4);

    auto leftTitleToolBar = createButtonToolBar(topBar, Qt::Horizontal);
    appIconButton = new QToolButton(leftTitleToolBar);
    setButtonTextMetadata(appIconButton, trText("Window menu"));
    setupFlatButton(appIconButton);
    QIcon appIcon = mainWindow->windowIcon().isNull() ? QApplication::windowIcon()
                                                      : mainWindow->windowIcon();
    if (appIcon.isNull()) {
        appIcon = mainWindow->style()->standardIcon(QStyle::SP_TitleBarMenuButton);
    }
    appIconButton->setIcon(appIcon);
    CompactTitleBarStyle::applyIconButtonMetrics(appIconButton, leftTitleToolBar);
    auto windowMenu = new QMenu(appIconButton);
    auto restoreAction = windowMenu->addAction(trText("Restore"), mainWindow, [this]() {
        mainWindow->showNormal();
    });
    windowMenu->addAction(trText("Minimize"), mainWindow, &MainWindow::showMinimized);
    auto maximizeAction
        = windowMenu->addAction(trText("Maximize"), mainWindow, &MainWindow::showMaximized);
    windowMenu->addSeparator();
    windowMenu->addAction(trText("Close"), mainWindow, &MainWindow::close);
    connect(windowMenu, &QMenu::aboutToShow, this, [this, restoreAction, maximizeAction]() {
        restoreAction->setEnabled(mainWindow->isMinimized() || mainWindow->isMaximized());
        maximizeAction->setEnabled(!mainWindow->isMaximized());
    });
    appIconButton->setMenu(windowMenu);
    appIconButton->setPopupMode(QToolButton::InstantPopup);
    leftTitleToolBar->addWidget(appIconButton);

    switchArea = new QWidget(topBar);
    switchArea->installEventFilter(this);
    auto switchLayout = new QVBoxLayout(switchArea);
    switchLayout->setContentsMargins(0, 0, 0, 0);
    switchLayout->setSpacing(0);

    auto compactToolBar = createButtonToolBar(switchArea, Qt::Horizontal);
    toolBar = compactToolBar;
    toolBar->setObjectName(QLatin1String(CompactToolBarObjectName));
    toolBar->installEventFilter(this);

    menuButton = new QToolButton(toolBar);
    menuButton->setIcon(hamburgerIcon(menuButton));
    setButtonTextMetadata(menuButton, trText("Show the main menu"));
    setupFlatButton(menuButton);
    CompactTitleBarStyle::applyIconButtonMetrics(menuButton, compactToolBar);
    compactToolBar->addWidget(menuButton);
    addToolbarGap(compactToolBar, CompactTitleBarStyle::tightGap());

    documentButton = new QToolButton(toolBar);
    documentButton->setPopupMode(QToolButton::InstantPopup);
    documentButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    documentButton->setIcon(mainWindow->style()->standardIcon(QStyle::SP_FileDialogContentsView));
    documentButton->setMenu(new QMenu(documentButton));
    documentButton->setAutoRaise(true);
    documentButton->setFocusPolicy(Qt::StrongFocus);
    documentButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setButtonTextMetadata(documentButton, trText("Documents"));
    CompactTitleBarStyle::applyMenuButtonMetrics(documentButton, compactToolBar);
    compactToolBar->addWidget(documentButton);
    connect(
        documentButton->menu(),
        &QMenu::aboutToShow,
        this,
        &CompactMainWindowChrome::rebuildDocumentMenu
    );
    addToolbarGap(compactToolBar, CompactTitleBarStyle::tightGap());

    auto workbenchMenuMarker = new QWidget(compactToolBar);
    workbenchMenuMarker->setFixedWidth(0);
    workbenchMenuMarker->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
    workbenchMenuInsertionPoint = compactToolBar->addWidget(workbenchMenuMarker);
    rebuildWorkbenchMenuButtons();
    addToolBarStretch(compactToolBar);

    macroButton = new QToolButton(toolBar);
    macroButton->setIcon(BitmapFactory().iconFromTheme("accessories-text-editor"));
    setButtonTextMetadata(macroButton, trText("Macro"));
    macroButton->setText(trText("Macro"));
    macroButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    macroButton->setAutoRaise(true);
    macroButton->setFocusPolicy(Qt::StrongFocus);
    macroButton->setPopupMode(QToolButton::InstantPopup);
    macroButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    macroButton->setMenu(new QMenu(macroButton));
    CompactTitleBarStyle::applyMenuButtonMetrics(macroButton, compactToolBar);
    compactToolBar->addWidget(macroButton);
    connect(macroButton->menu(), &QMenu::aboutToShow, this, &CompactMainWindowChrome::rebuildMacroMenu);

    auto runMacroButton = new QToolButton(toolBar);
    runMacroButton->setIcon(BitmapFactory().iconFromTheme("media-playback-start"));
    setButtonTextMetadata(runMacroButton, trText("Run selected macro"));
    setupFlatButton(runMacroButton);
    CompactTitleBarStyle::applyIconButtonMetrics(runMacroButton, compactToolBar);
    compactToolBar->addWidget(runMacroButton);
    connect(runMacroButton, &QToolButton::clicked, this, &CompactMainWindowChrome::runSelectedMacro);
    addCommandToToolBar(compactToolBar, "Std_DlgMacroRecord");

    addToolbarGap(compactToolBar);
    editModeButton = new QToolButton(toolBar);
    editModeButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    editModeButton->setAutoRaise(true);
    editModeButton->setFocusPolicy(Qt::StrongFocus);
    editModeButton->setPopupMode(QToolButton::InstantPopup);
    editModeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    editModeButton->setMenu(new QMenu(editModeButton));
    CompactTitleBarStyle::applyMenuButtonMetrics(editModeButton, compactToolBar);
    compactToolBar->addWidget(editModeButton);
    connect(editModeButton->menu(), &QMenu::aboutToShow, this, [this]() {
        auto menu = editModeButton ? editModeButton->menu() : nullptr;
        if (!menu) {
            return;
        }

        menu->clear();
        const int activeMode = Gui::Application::Instance->getUserEditMode();
        for (const auto& [mode, modeStrings] : Gui::Application::Instance->listUserEditModes()) {
            Q_UNUSED(modeStrings)
            auto action = menu->addAction(editModeIcon(mode), editModeText(mode), this, [mode]() {
                setEditMode(mode);
            });
            action->setCheckable(true);
            action->setChecked(mode == activeMode);
            action->setToolTip(editModeToolTip(mode));
        }
    });
    updateEditModeButton();

    workbenchButton = new QToolButton(toolBar);
    workbenchButton->setIcon(BitmapFactory().iconFromTheme("freecad"));
    setButtonTextMetadata(workbenchButton, trText("Workbench"));
    workbenchButton->setText(trText("Workbench"));
    workbenchButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    workbenchButton->setAutoRaise(true);
    workbenchButton->setFocusPolicy(Qt::StrongFocus);
    workbenchButton->setPopupMode(QToolButton::InstantPopup);
    workbenchButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    auto workbenchMenu = new QMenu(workbenchButton);
    populateWorkbenchMenu(workbenchMenu);
    workbenchButton->setMenu(workbenchMenu);
    compactToolBar->addWidget(workbenchButton);
    CompactTitleBarStyle::applyMenuButtonMetrics(workbenchButton, compactToolBar);
    updateWorkbenchButton();

    if (auto command = Gui::Application::Instance->commandManager().getCommandByName("Std_Workbench")) {
        command->initAction();
        if (auto workbenchGroup = qobject_cast<WorkbenchGroup*>(command->getAction())) {
            connect(
                workbenchGroup,
                &WorkbenchGroup::workbenchListRefreshed,
                workbenchButton,
                [workbenchMenu](QList<QAction*>) { populateWorkbenchMenu(workbenchMenu); }
            );
        }
    }

    addToolbarGap(compactToolBar, CompactTitleBarStyle::tightGap());
    addCommandToToolBar(compactToolBar, "Std_Recompute");

    addToolbarGap(compactToolBar);

    auto settingsButton = new QToolButton(toolBar);
    settingsButton->setIcon(BitmapFactory().iconFromTheme("Std_DlgParameter"));
    if (settingsButton->icon().isNull()) {
        settingsButton->setIcon(BitmapFactory().pixmap("Std_DlgParameter"));
    }
    if (settingsButton->icon().isNull()) {
        settingsButton->setIcon(mainWindow->style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    }
    setButtonTextMetadata(settingsButton, trText("Settings"));
    settingsButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    settingsButton->setAutoRaise(true);
    settingsButton->setFocusPolicy(Qt::StrongFocus);
    settingsButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    CompactTitleBarStyle::applyIconMenuButtonMetrics(settingsButton, compactToolBar);
    settingsButton->setPopupMode(QToolButton::InstantPopup);
    auto settingsMenu = new QMenu(settingsButton);
    addCommandToMenu(settingsMenu, "Std_DlgPreferences");
    addCommandToMenu(settingsMenu, "Std_DlgParameter");
    addCommandToMenu(settingsMenu, "Std_DlgCustomize");
    settingsMenu->addAction(trText("Save and restore..."), this, []() {
        Gui::Application::Instance->commandManager().runCommandByName("Std_DlgPreferences");
    });
    addCommandToMenu(settingsMenu, "Std_AddonMgr");
    connectRightAlignedPopup(settingsButton, settingsMenu);
    compactToolBar->addWidget(settingsButton);

    addToolbarGap(compactToolBar);
    auto helpMenu = new QMenu(toolBar);
    addCommandToMenu(helpMenu, "Std_OnlineHelp");
    addCommandToMenu(helpMenu, "Std_OnlineHelpWebsite");
    addCommandToMenu(helpMenu, "Std_WhatsThis");
    helpMenu->addSeparator();
    addCommandToMenu(helpMenu, "Std_FreeCADUserHub");
    addCommandToMenu(helpMenu, "Std_FreeCADForum");
    addCommandToMenu(helpMenu, "Std_ReportBug");
    helpMenu->addSeparator();
    addCommandToMenu(helpMenu, "Std_RestartInSafeMode");
    addCommandToMenu(helpMenu, "Std_About");
    QIcon helpIcon = BitmapFactory().iconFromTheme("help-browser");
    if (helpIcon.isNull()) {
        helpIcon = mainWindow->style()->standardIcon(QStyle::SP_DialogHelpButton);
    }
    auto helpButton = addDropdownButtonToToolBar(compactToolBar, QString(), helpIcon, helpMenu);
    if (helpButton) {
        setButtonTextMetadata(helpButton, trText("Help"));
        helpButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        CompactTitleBarStyle::applyIconMenuButtonMetrics(helpButton, compactToolBar);
        connectRightAlignedPopup(helpButton, helpMenu);
    }

    menuBar = new QMenuBar(switchArea);
    menuBar->setObjectName(QLatin1String(CompactMenuBarObjectName));
    menuBar->installEventFilter(this);
    menuBar->setContentsMargins(0, 0, 0, 0);
    menuBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    menuBar->hide();

    switchLayout->setAlignment(Qt::AlignVCenter);
    switchLayout->addWidget(toolBar, 0, Qt::AlignVCenter);
    switchLayout->addWidget(menuBar, 0, Qt::AlignVCenter);

    auto windowControlsToolBar = createButtonToolBar(topBar, Qt::Horizontal);
    minimizeButton = createTitleButton(trText("Minimize"), windowControlsToolBar);
    maximizeButton = createTitleButton(trText("Maximize"), windowControlsToolBar);
    closeButton = createTitleButton(trText("Close"), windowControlsToolBar);
    windowControlsToolBar->addWidget(minimizeButton);
    windowControlsToolBar->addWidget(maximizeButton);
    windowControlsToolBar->addWidget(closeButton);

    topBarLayout->addWidget(leftTitleToolBar);
    topBarLayout->addWidget(switchArea, 1);
    topBarLayout->addWidget(windowControlsToolBar);
    topBarLayout->setAlignment(leftTitleToolBar, Qt::AlignVCenter);
    topBarLayout->setAlignment(switchArea, Qt::AlignVCenter);
    topBarLayout->setAlignment(windowControlsToolBar, Qt::AlignVCenter);

    connect(menuButton, &QToolButton::clicked, this, &CompactMainWindowChrome::showMainMenu);
    connect(menuBar, &QMenuBar::triggered, this, [this]() {
        QTimer::singleShot(0, this, &CompactMainWindowChrome::hideMainMenu);
    });
    connect(minimizeButton, &QToolButton::clicked, mainWindow, &MainWindow::showMinimized);
    connect(maximizeButton, &QToolButton::clicked, this, [this]() {
        mainWindow->isMaximized() ? mainWindow->showNormal() : mainWindow->showMaximized();
    });
    connect(closeButton, &QToolButton::clicked, mainWindow, &MainWindow::close);
    connect(mainWindow, &MainWindow::workbenchActivated, this, [this]() {
        QTimer::singleShot(0, this, &CompactMainWindowChrome::refreshPanelStrips);
        QTimer::singleShot(0, this, &CompactMainWindowChrome::updateWorkbenchButton);
        QTimer::singleShot(0, this, &CompactMainWindowChrome::rebuildWorkbenchMenuButtons);
    });
    connect(mainWindow, &MainWindow::mainWindowClosed, this, [this]() {
        shuttingDown = true;
        clearWorkbenchMenuButtons();
    });
    newDocumentConnection = App::GetApplication().signalNewDocument.connect(
        [this](const App::Document&, bool) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    deleteDocumentConnection = App::GetApplication().signalDeleteDocument.connect(
        [this](const App::Document&) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    activeDocumentConnection = App::GetApplication().signalActiveDocument.connect(
        [this](const App::Document&) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    relabelDocumentConnection = App::GetApplication().signalRelabelDocument.connect(
        [this](const App::Document&) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    renameDocumentConnection = App::GetApplication().signalRenameDocument.connect(
        [this](const App::Document&) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    changedDocumentConnection = App::GetApplication().signalChangedDocument.connect(
        [this](const App::Document&, const App::Property&) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    finishSaveDocumentConnection = App::GetApplication().signalFinishSaveDocument.connect(
        [this](const App::Document&, const std::string&) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    activateViewConnection = Gui::Application::Instance->signalActivateView.connect(
        [this](const Gui::MDIView*) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    closeViewConnection = Gui::Application::Instance->signalCloseView.connect(
        [this](const Gui::MDIView*) {
            QTimer::singleShot(0, this, &CompactMainWindowChrome::updateDocumentButton);
        }
    );
    userEditModeConnection = Gui::Application::Instance->signalUserEditModeChanged.connect(
        [this](int) { QTimer::singleShot(0, this, &CompactMainWindowChrome::updateEditModeButton); }
    );

    removeLegacyDockStrips();

    leftStrip = createStrip(mainWindow, &leftStripContent, QLatin1String(CompactLeftStripObjectName));
    rightStrip
        = createStrip(mainWindow, &rightStripContent, QLatin1String(CompactRightStripObjectName));
    createResizeGrips();
}

void CompactMainWindowChrome::setActive(bool enabled)
{
    if (!topBar) {
        return;
    }

    topBar->setVisible(enabled);
    leftStrip->setVisible(enabled);
    rightStrip->setVisible(enabled);

    if (enabled && !active) {
        setGlobalEventFilterActive(true);
        menuBarVisibleBefore = mainWindow->menuBar()->isVisible();
        contentsMarginsBefore = mainWindow->contentsMargins();
        contentsMarginsSaved = true;
        if (auto tabBar
            = mainWindow->getMdiArea()->findChild<QTabBar*>(QStringLiteral("mdiAreaTabBar"))) {
            mdiTabBarVisibleBefore = tabBar->isVisible();
            mdiTabBarMinimumHeightBefore = tabBar->minimumHeight();
            mdiTabBarMaximumHeightBefore = tabBar->maximumHeight();
            mdiTabBarVisibilitySaved = true;
        }
        mainWindow->menuBar()->hide();
        updateMdiTabBarVisibility();
        syncMenuBar();
        const QList<QDockWidget*> docks = managedDockContainers();
        for (auto dock : docks) {
            const auto slot = panelSlotForDock(dock);
            const bool wasVisible = dock->isVisible();
            mainWindow->addDockWidget(dockAreaForSlot(slot), dock);
            dock->setVisible(wasVisible);
        }
    }
    else if (!enabled && active) {
        hideMainMenu();
        setGlobalEventFilterActive(false);
        mainWindow->menuBar()->setVisible(menuBarVisibleBefore);
        if (contentsMarginsSaved) {
            mainWindow->setContentsMargins(contentsMarginsBefore);
            contentsMarginsSaved = false;
        }
        if (auto tabBar
            = mainWindow->getMdiArea()->findChild<QTabBar*>(QStringLiteral("mdiAreaTabBar"))) {
            tabBar->setMinimumHeight(mdiTabBarMinimumHeightBefore);
            tabBar->setMaximumHeight(mdiTabBarMaximumHeightBefore);
            tabBar->setVisible(!mdiTabBarVisibilitySaved || mdiTabBarVisibleBefore);
            tabBar->updateGeometry();
            mainWindow->getMdiArea()->updateGeometry();
        }
        mdiTabBarVisibilitySaved = false;
        finishManualResize();
        titleDragActive = false;
    }

    active = enabled;
    if (enabled && toolBar && menuBar && !menuBar->isVisible()) {
        toolBar->show();
    }

    updateDocumentButton();
    updateMdiTabBarVisibility();
    updateWindowControls();
    layoutChrome();
    refreshPanelStrips();
}

bool CompactMainWindowChrome::isActive() const
{
    return active;
}

void CompactMainWindowChrome::setGlobalEventFilterActive(bool enabled)
{
    if (enabled == eventFilterInstalled) {
        return;
    }

    if (enabled) {
        qApp->installEventFilter(this);
    }
    else {
        qApp->removeEventFilter(this);
    }
    eventFilterInstalled = enabled;
}

void CompactMainWindowChrome::syncMenuBar()
{
    if (!menuBar) {
        return;
    }

    for (auto action : menuBar->actions()) {
        menuBar->removeAction(action);
    }
    menuBar->addActions(mainWindow->menuBar()->actions());
}

void CompactMainWindowChrome::updateDocumentButton()
{
    if (shuttingDown || !documentButton) {
        return;
    }

    auto activeView = qobject_cast<MDIView*>(mainWindow->activeWindow());
    const QString label = activeView ? viewDisplayName(activeView) : documentDisplayName(nullptr);
    documentButton->setText(label);
    documentButton->setIcon(documentIcon(activeView, mainWindow));
    documentButton->setToolTip(label);
    documentButton->setAccessibleName(trText("Current tab"));
    documentButton->setStatusTip(trText("Current tab"));
    CompactTitleBarStyle::resizeMenuButton(documentButton);
}

void CompactMainWindowChrome::rebuildDocumentMenu()
{
    if (shuttingDown || !documentButton || !documentButton->menu()) {
        return;
    }

    auto menu = documentButton->menu();
    menu->clear();

    addCommandToMenu(menu, "Std_New");
    addCommandToMenu(menu, "Std_Open");
    addCommandToMenu(menu, "Std_Import");
    addCommandToMenu(menu, "Std_Export");
    menu->addSeparator();
    addCommandToMenu(menu, "Std_Save");
    addCommandToMenu(menu, "Std_SaveAll");
    menu->addSeparator();
    addCommandToMenu(menu, "Std_CloseActiveWindow");
    addCommandToMenu(menu, "Std_CloseAllWindows");
    menu->addSeparator();

    const QList<QWidget*> views = mainWindow->windows();
    auto activeView = mainWindow->activeWindow();
    if (!views.isEmpty()) {
        for (auto widget : views) {
            auto view = qobject_cast<MDIView*>(widget);
            if (!view) {
                continue;
            }

            addDocumentViewRow(menu, mainWindow, view, view == activeView);
        }
        menu->addSeparator();
    }

    addRecentFilesToMenu(menu);
}

void CompactMainWindowChrome::rebuildMacroMenu()
{
    if (shuttingDown || !macroButton || !macroButton->menu()) {
        return;
    }

    auto menu = macroButton->menu();
    menu->clear();

    const auto addMacroAction = [this, menu](const QString& filePath) {
        auto action = menu->addAction(macroDisplayName(filePath), this, [this, filePath]() {
            selectMacro(filePath);
        });
        action->setToolTip(filePath);
    };

    const QStringList recentFiles = recentMacroFiles();
    if (!recentFiles.isEmpty()) {
        addMenuHeader(menu, trText("Recent"));
        for (const auto& file : recentFiles) {
            addMacroAction(file);
        }
    }

    if (!menu->actions().isEmpty()) {
        menu->addSeparator();
    }

    const QStringList macroFiles = allMacroFiles();
    for (const auto& file : macroFiles) {
        auto action = menu->addAction(macroDisplayName(file), this, [this, file]() {
            selectMacro(file);
        });
        action->setToolTip(file);
    }

    menu->addSeparator();
    menu->addAction(trText("Edit Macros"), this, []() {
        Gui::Application::Instance->commandManager().runCommandByName("Std_DlgMacroExecute");
    });
}

void CompactMainWindowChrome::selectMacro(const QString& filePath)
{
    selectedMacroFile = filePath;
    if (!macroButton) {
        return;
    }

    const QString label = filePath.isEmpty() ? trText("Macro") : macroDisplayName(filePath);
    macroButton->setText(label);
    macroButton->setToolTip(filePath.isEmpty() ? label : filePath);
    macroButton->setAccessibleName(label);
    macroButton->setStatusTip(label);
    CompactTitleBarStyle::resizeMenuButton(macroButton);
}

void CompactMainWindowChrome::runSelectedMacro()
{
    if (selectedMacroFile.isEmpty()) {
        Gui::Application::Instance->commandManager().runCommandByName("Std_DlgMacroExecute");
        return;
    }

    try {
        mainWindow->setCursor(Qt::WaitCursor);
        mainWindow->appendRecentMacro(selectedMacroFile);
        Gui::Application::Instance->macroManager()->run(
            Gui::MacroManager::File,
            selectedMacroFile.toUtf8()
        );
        if (Gui::Application::Instance->activeDocument()) {
            Gui::Application::Instance->activeDocument()->getDocument()->recompute();
        }
        mainWindow->unsetCursor();
    }
    catch (const Base::SystemExitException&) {
        Base::PyGILStateLocker locker;
        Base::PyException e;
        e.reportException();
        mainWindow->unsetCursor();
    }
    catch (const Base::Exception& e) {
        e.reportException();
        mainWindow->unsetCursor();
    }
}

void CompactMainWindowChrome::updateEditModeButton()
{
    if (shuttingDown || !editModeButton) {
        return;
    }

    const int mode = Gui::Application::Instance->getUserEditMode();
    const QString text = editModeText(mode);
    const QIcon icon = editModeIcon(mode);

    editModeButton->setText(text);
    editModeButton->setIcon(icon);
    editModeButton->update();
    setButtonTextMetadata(editModeButton, text);
    CompactTitleBarStyle::applyMenuButtonMetrics(editModeButton, qobject_cast<QToolBar*>(toolBar));
}

void CompactMainWindowChrome::updateWorkbenchButton()
{
    if (shuttingDown || !workbenchButton) {
        return;
    }

    if (auto command = Gui::Application::Instance->commandManager().getCommandByName("Std_Workbench")) {
        command->initAction();
        if (auto workbenchGroup = qobject_cast<WorkbenchGroup*>(command->getAction())) {
            for (auto action : workbenchGroup->getEnabledWbActions()) {
                if (action && action->isChecked()) {
                    const QString text = Action::cleanTitle(action->text());
                    workbenchButton->setText(text);
                    workbenchButton->setIcon(action->icon());
                    setButtonTextMetadata(workbenchButton, text);
                    CompactTitleBarStyle::resizeMenuButton(workbenchButton);
                    return;
                }
            }
        }
    }

    workbenchButton->setText(trText("Workbench"));
    setButtonTextMetadata(workbenchButton, trText("Workbench"));
    CompactTitleBarStyle::resizeMenuButton(workbenchButton);
}

void CompactMainWindowChrome::rebuildWorkbenchMenuButtons()
{
    auto compactToolBar = qobject_cast<QToolBar*>(toolBar);
    if (!compactToolBar) {
        return;
    }

    clearWorkbenchMenuButtons();

    if (shuttingDown || !mainWindow || !mainWindow->menuBar()) {
        return;
    }

    for (auto action : mainWindow->menuBar()->actions()) {
        if (!action || !action->isVisible() || action->isSeparator()
            || isStandardTopLevelMenu(action)) {
            continue;
        }

        auto sourceMenu = action->menu();
        if (!sourceMenu) {
            continue;
        }

        auto button = new QToolButton(compactToolBar);
        button->setText(Action::cleanTitle(action->text()));
        button->setIcon(action->icon());
        auto copiedMenu = new QMenu(Action::cleanTitle(action->text()), button);
        copyMenuActions(sourceMenu, copiedMenu);
        button->setMenu(copiedMenu);
        button->setPopupMode(QToolButton::InstantPopup);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setAutoRaise(true);
        button->setFocusPolicy(Qt::StrongFocus);
        button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        CompactTitleBarStyle::applyMenuButtonMetrics(button, compactToolBar);
        CompactTitleBarStyle::resizeMenuButton(button);
        if (button) {
            setButtonTextMetadata(button, Action::cleanTitle(action->text()));
            QAction* insertedAction = workbenchMenuInsertionPoint
                ? compactToolBar->insertWidget(workbenchMenuInsertionPoint, button)
                : compactToolBar->addWidget(button);
            workbenchMenuTitleActions.push_back(insertedAction);
        }
    }
}

void CompactMainWindowChrome::clearWorkbenchMenuButtons()
{
    auto compactToolBar = qobject_cast<QToolBar*>(toolBar);
    if (!compactToolBar) {
        workbenchMenuTitleActions.clear();
        return;
    }

    for (auto action : workbenchMenuTitleActions) {
        if (!action) {
            continue;
        }
        compactToolBar->removeAction(action);
        delete action;
    }
    workbenchMenuTitleActions.clear();
}

void CompactMainWindowChrome::updateMdiTabBarVisibility()
{
    if (!mainWindow || !active) {
        return;
    }

    if (auto tabBar = mainWindow->getMdiArea()->findChild<QTabBar*>(QStringLiteral("mdiAreaTabBar"))) {
        if (!mdiTabBarVisibilitySaved) {
            mdiTabBarVisibleBefore = tabBar->isVisible();
            mdiTabBarMinimumHeightBefore = tabBar->minimumHeight();
            mdiTabBarMaximumHeightBefore = tabBar->maximumHeight();
            mdiTabBarVisibilitySaved = true;
        }
        tabBar->setMinimumHeight(0);
        tabBar->setMaximumHeight(0);
        tabBar->hide();
        tabBar->updateGeometry();
        mainWindow->getMdiArea()->updateGeometry();
    }
}

void CompactMainWindowChrome::updateHamburgerIcon()
{
    if (menuButton) {
        menuButton->setIcon(hamburgerIcon(menuButton));
    }
}

void CompactMainWindowChrome::updateWindowControls()
{
    if (!mainWindow) {
        return;
    }

    if (appIconButton) {
        QIcon appIcon = mainWindow->windowIcon().isNull() ? QApplication::windowIcon()
                                                          : mainWindow->windowIcon();
        if (appIcon.isNull()) {
            appIcon = mainWindow->style()->standardIcon(QStyle::SP_TitleBarMenuButton);
        }
        appIconButton->setIcon(appIcon);
    }

    if (minimizeButton) {
        minimizeButton->setIcon(mainWindow->style()->standardIcon(QStyle::SP_TitleBarMinButton));
    }

    if (maximizeButton) {
        const bool maximized = mainWindow->isMaximized();
        maximizeButton->setIcon(mainWindow->style()->standardIcon(
            maximized ? QStyle::SP_TitleBarNormalButton : QStyle::SP_TitleBarMaxButton
        ));
        setButtonTextMetadata(maximizeButton, maximized ? trText("Restore") : trText("Maximize"));
    }

    if (closeButton) {
        closeButton->setIcon(mainWindow->style()->standardIcon(QStyle::SP_TitleBarCloseButton));
    }
}

void CompactMainWindowChrome::showMainMenu()
{
    if (!menuBar) {
        return;
    }

    if (menuBar->isVisible()) {
        hideMainMenu();
        return;
    }

    syncMenuBar();
    if (toolBar) {
        toolBar->hide();
    }
    menuBar->show();
    layoutChrome();
    QTimer::singleShot(0, this, &CompactMainWindowChrome::openFirstMenu);
}

void CompactMainWindowChrome::hideMainMenu()
{
    if (!active || !menuBar || !menuBar->isVisible()) {
        return;
    }

    menuBar->hide();
    if (toolBar) {
        toolBar->show();
    }
    layoutChrome();
}

void CompactMainWindowChrome::openFirstMenu()
{
    if (!menuBar || !menuBar->isVisible()) {
        return;
    }

    const auto actions = menuBar->actions();
    if (actions.isEmpty()) {
        return;
    }

    QAction* firstAction = actions.constFirst();
    menuBar->setActiveAction(firstAction);

    if (QMenu* firstMenu = firstAction->menu()) {
        const QRect actionRect = menuBar->actionGeometry(firstAction);
        firstMenu->popup(menuBar->mapToGlobal(actionRect.bottomLeft()));
    }
}

void CompactMainWindowChrome::layoutChrome()
{
    if (!active || !topBar) {
        setResizeGripsVisible(false);
        return;
    }

    updateWindowControls();
    updateDocumentButton();
    updateMdiTabBarVisibility();
    layoutTopBar();
    applyContentsMargins();
    layoutWorkArea();
    layoutPanelStrips();
    layoutResizeGrips();
}

void CompactMainWindowChrome::applyContentsMargins()
{
    if (!active || !contentsMarginsSaved) {
        return;
    }

    const int topBarHeight = topBar && topBar->isVisible() ? topBar->height() : 0;
    mainWindow->setContentsMargins(
        contentsMarginsBefore.left(),
        contentsMarginsBefore.top() + topBarHeight,
        contentsMarginsBefore.right(),
        contentsMarginsBefore.bottom()
    );
}

void CompactMainWindowChrome::layoutWorkArea()
{
    auto central = mainWindow->centralWidget();
    if (!active || !central) {
        return;
    }

    const int panelStripWidth = compactPanelStripWidth();
    QRect geometry = central->geometry();
    geometry.setLeft(panelStripWidth);
    geometry.setRight(mainWindow->width() - panelStripWidth - 1);
    if (geometry.isValid()) {
        central->setGeometry(geometry);
    }
}

void CompactMainWindowChrome::layoutTopBar()
{
    if (!active || !topBar) {
        return;
    }

    if (toolBar && menuBar) {
        const int childHeight = std::max(toolBar->sizeHint().height(), menuBar->sizeHint().height());
        toolBar->setFixedHeight(childHeight);
        menuBar->setFixedHeight(menuBar->sizeHint().height());
        if (switchArea) {
            switchArea->setFixedHeight(childHeight);
        }
    }

    const int topBarHeight = topBar->sizeHint().height();
    topBar->setGeometry(0, 0, mainWindow->width(), topBarHeight);
    topBar->raise();
}

void CompactMainWindowChrome::layoutPanelStrips()
{
    if (!active || !leftStrip || !rightStrip) {
        return;
    }

    int top = 0;

    if (auto central = mainWindow->centralWidget()) {
        top = central->geometry().top();
    }
    else if (topBar && topBar->isVisible()) {
        top = topBar->geometry().bottom() + 1;
    }
    else if (mainWindow->menuBar() && mainWindow->menuBar()->isVisible()) {
        top = mainWindow->menuBar()->geometry().bottom() + 1;
    }

    int bottom = mainWindow->height();
    if (mainWindow->statusBar() && mainWindow->statusBar()->isVisible()) {
        bottom = mainWindow->statusBar()->geometry().top();
    }

    const int panelStripWidth = compactPanelStripWidth();
    const int stripHeight = std::max(0, bottom - top);
    leftStrip->setFixedWidth(panelStripWidth);
    rightStrip->setFixedWidth(panelStripWidth);
    leftStrip->setGeometry(0, top, panelStripWidth, stripHeight);
    rightStrip->setGeometry(mainWindow->width() - panelStripWidth, top, panelStripWidth, stripHeight);
    leftStrip->raise();
    rightStrip->raise();
    if (topBar) {
        topBar->raise();
    }
}

void CompactMainWindowChrome::refreshPanelStrips()
{
    if (!leftStripContent || !rightStripContent) {
        return;
    }

    clearStrip(leftStripContent);
    clearStrip(rightStripContent);

    auto leftStripToolBar = createButtonToolBar(leftStripContent, Qt::Vertical);
    auto rightStripToolBar = createButtonToolBar(rightStripContent, Qt::Vertical);
    leftStripContent->layout()->addWidget(leftStripToolBar);
    rightStripContent->layout()->addWidget(rightStripToolBar);

    auto addButton = [this](QDockWidget* dock, QToolBar* toolbar, PanelSlot slot) {
        const QString title = dockTitle(dock);
        auto action = new QAction(dockIcon(dock, slot), title, toolbar);
        action->setToolTip(title);
        action->setStatusTip(title);
        action->setCheckable(true);
        action->setChecked(dock->isVisible());

        connect(action, &QAction::triggered, this, [this, dock, slot]() {
            if (dock->isVisible()) {
                dock->toggleViewAction()->activate(QAction::Trigger);
                refreshPanelStrips();
                return;
            }

            const auto group = panelGroup(slot);
            const QList<QDockWidget*> docks = managedDockContainers();
            for (auto other : docks) {
                if (other == dock) {
                    continue;
                }

                if (panelGroup(panelSlotForDock(other)) == group && other->isVisible()) {
                    other->toggleViewAction()->activate(QAction::Trigger);
                }
            }

            if (!dock->isVisible()) {
                dock->toggleViewAction()->activate(QAction::Trigger);
            }
            dock->raise();
            refreshPanelStrips();
        });

        toolbar->addAction(action);
        if (auto button = qobject_cast<QToolButton*>(toolbar->widgetForAction(action))) {
            button->setAccessibleName(title);
            CompactTitleBarStyle::applyIconButtonMetrics(button, toolbar);
        }
    };

    QList<PanelEntry> entries;
    const QList<QDockWidget*> docks = managedDockContainers();
    for (auto dock : docks) {
        const auto slot = panelSlotForDock(dock);
        entries.push_back({dock, slot, panelOrderForDock(dock, slot)});
    }

    std::sort(entries.begin(), entries.end(), [this](const PanelEntry& left, const PanelEntry& right) {
        if (left.slot != right.slot) {
            return static_cast<int>(left.slot) < static_cast<int>(right.slot);
        }
        if (left.order != right.order) {
            return left.order < right.order;
        }

        return dockTitle(left.dock).localeAwareCompare(dockTitle(right.dock)) < 0;
    });

    bool addedLeftTop = false;
    bool addedLeftSeparator = false;
    bool addedRightTop = false;
    bool addedRightSeparator = false;
    bool addedLeftBottomSpacer = false;
    bool addedRightBottomSpacer = false;
    for (const auto& entry : entries) {
        switch (entry.slot) {
            case PanelSlot::LeftTop:
                addButton(entry.dock, leftStripToolBar, entry.slot);
                addedLeftTop = true;
                break;
            case PanelSlot::LeftLower:
                if (!addedLeftSeparator) {
                    if (addedLeftTop) {
                        leftStripToolBar->addSeparator();
                    }
                    addedLeftSeparator = true;
                }
                addButton(entry.dock, leftStripToolBar, entry.slot);
                break;
            case PanelSlot::RightTop:
                addButton(entry.dock, rightStripToolBar, entry.slot);
                addedRightTop = true;
                break;
            case PanelSlot::RightLower:
                if (!addedRightSeparator) {
                    if (addedRightTop) {
                        rightStripToolBar->addSeparator();
                    }
                    addedRightSeparator = true;
                }
                addButton(entry.dock, rightStripToolBar, entry.slot);
                break;
            case PanelSlot::BottomLeft:
                if (!addedLeftBottomSpacer) {
                    addStripSpacer(leftStripToolBar);
                    addedLeftBottomSpacer = true;
                }
                addButton(entry.dock, leftStripToolBar, entry.slot);
                break;
            case PanelSlot::BottomRight:
                if (!addedRightBottomSpacer) {
                    addStripSpacer(rightStripToolBar);
                    addedRightBottomSpacer = true;
                }
                addButton(entry.dock, rightStripToolBar, entry.slot);
                break;
        }
    }

    if (!addedLeftBottomSpacer) {
        addStripSpacer(leftStripToolBar);
    }
    if (!addedRightBottomSpacer) {
        addStripSpacer(rightStripToolBar);
    }

    layoutPanelStrips();
}

void CompactMainWindowChrome::createResizeGrips()
{
    const QList<ResizeGrip> grips = {
        {new QWidget(mainWindow), Qt::LeftEdge},
        {new QWidget(mainWindow), Qt::RightEdge},
        {new QWidget(mainWindow), Qt::TopEdge},
        {new QWidget(mainWindow), Qt::BottomEdge},
        {new QWidget(mainWindow), Qt::LeftEdge | Qt::TopEdge},
        {new QWidget(mainWindow), Qt::RightEdge | Qt::TopEdge},
        {new QWidget(mainWindow), Qt::LeftEdge | Qt::BottomEdge},
        {new QWidget(mainWindow), Qt::RightEdge | Qt::BottomEdge},
    };

    for (const auto& grip : grips) {
        grip.widget->setObjectName(QStringLiteral("_fc_compact_resize_grip"));
        grip.widget->setCursor(cursorForEdges(grip.edges));
        grip.widget->installEventFilter(this);
        grip.widget->hide();
        resizeGripEdges.insert(grip.widget, grip.edges);
        resizeGrips.push_back(grip);
    }
}

void CompactMainWindowChrome::layoutResizeGrips()
{
    const bool visible = active && framelessWindow && !mainWindow->isMaximized()
        && !mainWindow->isFullScreen();
    setResizeGripsVisible(visible);
    if (!visible) {
        return;
    }

    const int width = mainWindow->width();
    const int height = mainWindow->height();
    const int border = CompactResizeBorderWidth;

    for (const auto& grip : resizeGrips) {
        QRect geometry;
        if (grip.edges == Qt::LeftEdge) {
            geometry = QRect(0, border, border, height - 2 * border);
        }
        else if (grip.edges == Qt::RightEdge) {
            geometry = QRect(width - border, border, border, height - 2 * border);
        }
        else if (grip.edges == Qt::TopEdge) {
            geometry = QRect(border, 0, width - 2 * border, border);
        }
        else if (grip.edges == Qt::BottomEdge) {
            geometry = QRect(border, height - border, width - 2 * border, border);
        }
        else if (grip.edges == (Qt::LeftEdge | Qt::TopEdge)) {
            geometry = QRect(0, 0, border, border);
        }
        else if (grip.edges == (Qt::RightEdge | Qt::TopEdge)) {
            geometry = QRect(width - border, 0, border, border);
        }
        else if (grip.edges == (Qt::LeftEdge | Qt::BottomEdge)) {
            geometry = QRect(0, height - border, border, border);
        }
        else if (grip.edges == (Qt::RightEdge | Qt::BottomEdge)) {
            geometry = QRect(width - border, height - border, border, border);
        }

        grip.widget->setGeometry(geometry);
        grip.widget->raise();
    }
}

void CompactMainWindowChrome::setResizeGripsVisible(bool visible)
{
    for (const auto& grip : resizeGrips) {
        grip.widget->setVisible(visible);
    }
}

bool CompactMainWindowChrome::eventFilter(QObject* watched, QEvent* event)
{
    if (!mainWindow || !active) {
        return QObject::eventFilter(watched, event);
    }

    if (resizeGripEdges.contains(watched)) {
        auto grip = qobject_cast<QWidget*>(watched);
        const Qt::Edges edges = resizeGripEdges.value(watched);
        if (event->type() == QEvent::MouseButtonPress) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                if (QWindow* window = mainWindow->windowHandle();
                    window && window->startSystemResize(edges)) {
                    return true;
                }
                startManualResize(edges, mouseEvent, grip);
                return true;
            }
        }
        if (manualResizeActive && event->type() == QEvent::MouseMove) {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if (mouseEvent->buttons() & Qt::LeftButton) {
                updateManualResize(mouseEvent);
                return true;
            }
            finishManualResize();
        }
        if (manualResizeActive
            && (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::Leave)) {
            finishManualResize();
            return true;
        }
    }

    auto compactTitleWidget = watched->isWidgetType() ? qobject_cast<QWidget*>(watched) : nullptr;
    const bool compactTitleBarEvent = topBar && compactTitleWidget
        && (compactTitleWidget == topBar || topBar->isAncestorOf(compactTitleWidget));
    const bool compactTitleDragSource = compactTitleBarEvent
        && !qobject_cast<QToolButton*>(compactTitleWidget)
        && !qobject_cast<QMenuBar*>(compactTitleWidget);

    if (compactTitleDragSource && event->type() == QEvent::MouseButtonDblClick) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            mainWindow->isMaximized() ? mainWindow->showNormal() : mainWindow->showMaximized();
            return true;
        }
    }

    if (compactTitleDragSource && event->type() == QEvent::MouseButtonPress) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            if (QWindow* window = mainWindow->windowHandle(); window && window->startSystemMove()) {
                titleDragActive = false;
                return true;
            }

            titleDragActive = true;
            titleDragGlobalPosition = globalPosition(mouseEvent);
            titleDragWindowPosition = mainWindow->frameGeometry().topLeft();
            return true;
        }
    }
    else if (titleDragActive && event->type() == QEvent::MouseMove) {
        auto mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->buttons() & Qt::LeftButton) {
            const QPoint position = globalPosition(mouseEvent);
            if (mainWindow->isMaximized()) {
                mainWindow->showNormal();
                titleDragWindowPosition
                    = QPoint(position.x() - mainWindow->width() / 2, position.y() - 14);
                titleDragGlobalPosition = position;
            }
            mainWindow->move(titleDragWindowPosition + position - titleDragGlobalPosition);
            return true;
        }
        titleDragActive = false;
    }
    else if (
        titleDragActive
        && (event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::Leave)
    ) {
        titleDragActive = false;
    }

    if (menuBar && menuBar->isVisible() && event->type() == QEvent::MouseButtonPress) {
        const bool insideTopBar = compactTitleBarEvent;
        const bool insideMenu = watched->inherits("QMenu");
        if (!insideTopBar && !insideMenu) {
            hideMainMenu();
        }
    }

    return QObject::eventFilter(watched, event);
}

void CompactMainWindowChrome::startManualResize(Qt::Edges edges, QMouseEvent* event, QWidget* grip)
{
    manualResizeActive = true;
    manualResizeEdges = edges;
    manualResizeGlobalPosition = globalPosition(event);
    manualResizeGeometry = mainWindow->geometry();
    if (grip) {
        grip->grabMouse();
    }
}

void CompactMainWindowChrome::updateManualResize(QMouseEvent* event)
{
    if (!manualResizeActive || mainWindow->isMaximized() || mainWindow->isFullScreen()) {
        return;
    }

    const QPoint delta = globalPosition(event) - manualResizeGlobalPosition;
    mainWindow->setGeometry(resizedGeometry(
        manualResizeGeometry,
        delta,
        manualResizeEdges,
        mainWindow->minimumSize(),
        mainWindow->maximumSize()
    ));
}

void CompactMainWindowChrome::finishManualResize()
{
    if (!manualResizeActive) {
        return;
    }

    manualResizeActive = false;
    if (QWidget* mouseGrabber = QWidget::mouseGrabber()) {
        if (resizeGripEdges.contains(mouseGrabber)) {
            mouseGrabber->releaseMouse();
        }
    }
}

QList<QDockWidget*> CompactMainWindowChrome::managedDockContainers() const
{
    QList<QDockWidget*> docks;
    const QList<QWidget*> widgets = DockWindowManager::instance()->getDockWindows();
    for (auto widget : widgets) {
        auto parent = widget ? widget->parentWidget() : nullptr;
        while (parent) {
            if (auto dock = qobject_cast<QDockWidget*>(parent)) {
                if (isCompactUiDockCandidate(dock) && !docks.contains(dock)) {
                    docks.push_back(dock);
                }
                break;
            }
            parent = parent->parentWidget();
        }
    }

    return docks;
}

QString CompactMainWindowChrome::dockActionId(const QDockWidget* dock) const
{
    if (!dock || !dock->toggleViewAction()) {
        return {};
    }

    return dock->toggleViewAction()->data().toString();
}

const CompactMainWindowChrome::KnownPanel* CompactMainWindowChrome::knownPanelForActionId(
    const QString& actionId
) const
{
    static constexpr KnownPanel panels[] = {
        {"Std_TreeView", PanelSlot::LeftTop, 10, "tree-doc-single", QStyle::SP_DirIcon},
        {"Std_ComboView", PanelSlot::LeftTop, 20, "tree-doc-multi", QStyle::SP_FileDialogDetailedView},
        {"Std_SelectionView", PanelSlot::LeftLower, 10, "view-select", QStyle::SP_FileDialogContentsView},
        {"Std_PropertyView", PanelSlot::LeftLower, 20, "document-properties", QStyle::SP_FileDialogListView},
        {"Std_TaskView",
         PanelSlot::RightTop,
         10,
         "qss:overlay/icons/taskshow.svg",
         QStyle::SP_FileDialogDetailedView},
        {"Std_TaskWatcher",
         PanelSlot::RightLower,
         10,
         "qss:overlay/icons/taskshow.svg",
         QStyle::SP_MessageBoxWarning},
        {"Std_DAGView", PanelSlot::RightLower, 20, "dagViewVisible", QStyle::SP_ComputerIcon},
        {"Std_PythonView", PanelSlot::BottomLeft, 10, "applications-python", QStyle::SP_ComputerIcon},
        {"Std_ReportView", PanelSlot::BottomRight, 10, "MacroEditor", QStyle::SP_MessageBoxInformation},
    };

    for (const auto& panel : panels) {
        if (actionId == QLatin1String(panel.actionId)) {
            return &panel;
        }
    }

    return nullptr;
}

QString CompactMainWindowChrome::dockTitle(const QDockWidget* dock) const
{
    QString title = dock->windowTitle();
    if (title.isEmpty()) {
        title = dock->objectName();
    }

    title.remove(QLatin1Char('&'));
    return title;
}

CompactMainWindowChrome::PanelGroup CompactMainWindowChrome::panelGroup(PanelSlot slot) const
{
    switch (slot) {
        case PanelSlot::LeftTop:
            return PanelGroup::LeftTop;
        case PanelSlot::LeftLower:
            return PanelGroup::LeftLower;
        case PanelSlot::RightTop:
            return PanelGroup::RightTop;
        case PanelSlot::RightLower:
            return PanelGroup::RightLower;
        case PanelSlot::BottomLeft:
            return PanelGroup::BottomLeft;
        case PanelSlot::BottomRight:
            return PanelGroup::BottomRight;
    }

    return PanelGroup::LeftLower;
}

CompactMainWindowChrome::PanelSlot CompactMainWindowChrome::fallbackSlotForDock(QDockWidget* dock) const
{
    switch (mainWindow->dockWidgetArea(dock)) {
        case Qt::RightDockWidgetArea:
            return PanelSlot::RightTop;
        case Qt::BottomDockWidgetArea:
            return PanelSlot::BottomLeft;
        case Qt::TopDockWidgetArea:
        case Qt::LeftDockWidgetArea:
        default:
            return PanelSlot::LeftLower;
    }
}

CompactMainWindowChrome::PanelSlot CompactMainWindowChrome::panelSlotForDock(QDockWidget* dock) const
{
    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        return knownPanel->slot;
    }

    return fallbackSlotForDock(dock);
}

Qt::DockWidgetArea CompactMainWindowChrome::dockAreaForSlot(PanelSlot slot) const
{
    switch (slot) {
        case PanelSlot::RightTop:
        case PanelSlot::RightLower:
            return Qt::RightDockWidgetArea;
        case PanelSlot::BottomLeft:
        case PanelSlot::BottomRight:
            return Qt::BottomDockWidgetArea;
        case PanelSlot::LeftTop:
        case PanelSlot::LeftLower:
        default:
            return Qt::LeftDockWidgetArea;
    }
}

int CompactMainWindowChrome::panelOrderForDock(const QDockWidget* dock, PanelSlot slot) const
{
    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        return knownPanel->order;
    }

    const int fallbackBase = 1000;
    return fallbackBase + static_cast<int>(slot);
}

QIcon CompactMainWindowChrome::dockIcon(const QDockWidget* dock, PanelSlot slot) const
{
    QIcon icon;

    if (const auto* knownPanel = knownPanelForActionId(dockActionId(dock))) {
        icon = BitmapFactory().iconFromTheme(knownPanel->iconName);
        if (icon.isNull()) {
            icon = BitmapFactory().pixmap(knownPanel->iconName);
        }
        if (icon.isNull()) {
            icon = QApplication::style()->standardIcon(
                static_cast<QStyle::StandardPixmap>(knownPanel->fallbackIcon)
            );
        }
    }

    if (icon.isNull() && dock->widget()) {
        icon = dock->widget()->windowIcon();
    }

    if (icon.isNull()) {
        icon = dock->windowIcon();
    }

    if (icon.isNull()) {
        const auto group = panelGroup(slot);
        const auto fallback = group == PanelGroup::RightTop || group == PanelGroup::RightLower
            ? QStyle::SP_FileDialogDetailedView
            : QStyle::SP_FileDialogListView;
        icon = QApplication::style()->standardIcon(fallback);
    }

    return icon;
}

QToolButton* CompactMainWindowChrome::createTitleButton(const QString& tooltip, QWidget* parent)
{
    auto button = new QToolButton(parent);
    setButtonTextMetadata(button, tooltip);
    setupFlatButton(button);
    CompactTitleBarStyle::applyIconButtonMetrics(button, qobject_cast<QToolBar*>(parent));
    return button;
}

void CompactMainWindowChrome::setButtonTextMetadata(QToolButton* button, const QString& text)
{
    if (!button) {
        return;
    }

    button->setToolTip(text);
    button->setStatusTip(text);
    button->setAccessibleName(text);
}

void CompactMainWindowChrome::setupFlatButton(QToolButton* button)
{
    button->setAutoRaise(true);
    button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    button->setFocusPolicy(Qt::StrongFocus);
}

void CompactMainWindowChrome::removeLegacyDockStrips()
{
    const QList<QDockWidget*> docks = mainWindow->findChildren<QDockWidget*>();
    for (auto dock : docks) {
        if (isCompactUiDock(dock)) {
            dock->hide();
            dock->deleteLater();
        }
    }
}

#include "moc_CompactMainWindowChrome.cpp"
