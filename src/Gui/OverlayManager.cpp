/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include <limits>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QGraphicsView>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMdiArea>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QTextStream>
#include <QTimerEvent>
#include <QToolTip>
#include <QScrollBar>

#include <QPainterPath>
#include <QPropertyAnimation>

#include <algorithm>
#include <array>
#include <unordered_map>

#include "OverlayManager.h"

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include "Application.h"
#include "BitmapFactory.h"
#include "Control.h"
#include "CompactTitleBarStyle.h"
#include "MainWindow.h"
#include "MDIView.h"
#include "NaviCube.h"
#include "OverlayParams.h"
#include "OverlayWidgets.h"
#include "TaskView/TaskView.h"
#include "Tree.h"
#include "TreeParams.h"
#include "View3DInventorViewer.h"

FC_LOG_LEVEL_INIT("Dock", true, true);

using namespace Gui;

static std::array<OverlayTabWidget*, 4> _Overlays;
static bool _compactRailTabOwnershipEnabled = false;
static constexpr auto OverlayTitleRoleProperty = "_fc_overlay_title_role";
static constexpr auto OverlayHeaderOwnerProperty = "_fc_overlay_header_owner";
static constexpr auto OverlayTitleRoleDock = "dock";
static constexpr auto OverlayTitleRoleOverlayHost = "overlay-host";
static constexpr auto OverlayTitleRolePlaceholder = "placeholder";
static constexpr auto OverlayHeaderOwnerDock = "dock";
static constexpr auto OverlayHeaderOwnerOverlay = "overlay";

static inline OverlayTabWidget* findTabWidget(QWidget* widget = nullptr, bool filterDialog = false)
{
    if (!widget) {
        widget = qApp->focusWidget();
    }
    for (auto w = widget; w; w = w->parentWidget()) {
        auto tabWidget = qobject_cast<OverlayTabWidget*>(w);
        if (tabWidget) {
            return tabWidget;
        }
        auto proxy = qobject_cast<OverlayProxyWidget*>(w);
        if (proxy) {
            return proxy->getOwner();
        }
        if (filterDialog && w->windowType() != Qt::Widget) {
            break;
        }
    }
    return nullptr;
}

class OverlayStyleSheet: public ParameterGrp::ObserverType
{
public:
    OverlayStyleSheet()
    {
        handle = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/MainWindow"
        );

        update();

        handle->Attach(this);
    }

    static OverlayStyleSheet* instance()
    {
        static OverlayStyleSheet* instance;

        if (!instance) {
            instance = new OverlayStyleSheet;
        }

        return instance;
    }

    void OnChange(Base::Subject<const char*>&, const char* sReason)
    {
        if (!sReason) {
            return;
        }

        if (strcmp(sReason, "StyleSheet") == 0 || strcmp(sReason, "OverlayActiveStyleSheet") == 0) {
            OverlayManager::instance()->refresh(nullptr, true);
        }
    }

    void update()
    {
        activeStyleSheet.clear();

        QString overlayStylesheetFileName = detectOverlayStyleSheetFileName();
        loadFromFile(overlayStylesheetFileName);

        // If after loading the result is still empty we need to apply some defaults
        if (activeStyleSheet.isEmpty()) {
            activeStyleSheet = _default;
        }

        activeStyleSheet = Application::Instance->replaceVariablesInQss(activeStyleSheet);
    }

    ParameterGrp::handle handle;
    QString activeStyleSheet;
    bool hideTab = false;

private:
    QString detectOverlayStyleSheetFileName() const
    {
        QString mainStyleSheet = QString::fromUtf8(handle->GetASCII("StyleSheet").c_str());
        QString overlayStyleSheet = QString::fromUtf8(
            handle->GetASCII("OverlayActiveStyleSheet").c_str()
        );

        if (overlayStyleSheet.isEmpty()) {
            // User did not choose any stylesheet, we need to choose one based on main stylesheet
            if (mainStyleSheet.contains(QStringLiteral("light"), Qt::CaseInsensitive)) {
                overlayStyleSheet = QStringLiteral("overlay:Light Theme + Dark Background.qss");
            }
            else {
                // by default FreeCAD uses somewhat dark background for 3D View.
                // if user has not explicitly selected light theme, the "Dark Outline" looks best
                overlayStyleSheet = QStringLiteral("overlay:Dark Theme + Dark Background.qss");
            }
        }
        else if (!overlayStyleSheet.isEmpty() && !QFile::exists(overlayStyleSheet)) {
            // User did choose one of predefined stylesheets, we need to qualify it with namespace
            overlayStyleSheet = QStringLiteral("overlay:%1").arg(overlayStyleSheet);
        }

        return overlayStyleSheet;
    }

    void loadFromFile(const QString& name)
    {
        if (!QFile::exists(name)) {
            return;
        }

        QFile file(name);

        if (file.open(QFile::ReadOnly)) {
            activeStyleSheet = QTextStream(&file).readAll();
        }
    }

    static const QString _default;
};

const QString OverlayStyleSheet::_default = QStringLiteral("overlay:Light Theme + Dark Background.qss");

// -----------------------------------------------------------

struct OverlayInfo
{
    const char* name;
    OverlayTabWidget* tabWidget;
    Qt::DockWidgetArea dockArea;
    std::unordered_map<QDockWidget*, OverlayInfo*>& overlayMap;
    ParameterGrp::handle hGrp;
    fastsignals::scoped_connection conn;

    OverlayInfo(
        QWidget* parent,
        const char* name,
        Qt::DockWidgetArea pos,
        std::unordered_map<QDockWidget*, OverlayInfo*>& map
    )
        : name(name)
        , dockArea(pos)
        , overlayMap(map)
    {
        tabWidget = new OverlayTabWidget(parent, dockArea);
        tabWidget->setObjectName(QString::fromUtf8(name));
        tabWidget->getProxyWidget()->setObjectName(tabWidget->objectName() + QStringLiteral("Proxy"));
        tabWidget->setMovable(true);
        hGrp = App::GetApplication()
                   .GetUserParameter()
                   .GetGroup("BaseApp")
                   ->GetGroup("MainWindow")
                   ->GetGroup("DockWindows")
                   ->GetGroup(name);
        conn = App::GetApplication().GetUserParameter().signalParamChanged.connect(
            [this](ParameterGrp* Param, ParameterGrp::ParamType, const char* Name, const char*) {
                if (hGrp == Param && Name && !tabWidget->isSaving()) {
                    // This will prevent saving settings which will mess up the
                    // just restored ones
                    tabWidget->restore(nullptr);
                    OverlayManager::instance()->reload();
                }
            }
        );
    }

    bool addWidget(QDockWidget* dock, bool forced = true)
    {
        if (!dock) {
            return false;
        }
        if (tabWidget->dockWidgetIndex(dock) >= 0) {
            return false;
        }
        overlayMap[dock] = this;
        bool visible = dock->isVisible();

        auto focus = qApp->focusWidget();
        if (focus && findTabWidget(focus) != tabWidget) {
            focus = nullptr;
        }

        tabWidget->addWidget(dock, dock->windowTitle());

        if (focus) {
            tabWidget->setCurrent(dock);
            focus = qApp->focusWidget();
            if (focus) {
                focus->clearFocus();
            }
        }

        if (forced) {
            auto mw = getMainWindow();
            for (auto d : mw->findChildren<QDockWidget*>()) {
                if (mw->dockWidgetArea(d) == dockArea && d->toggleViewAction()->isChecked()) {
                    addWidget(d, false);
                }
            }
            if (visible) {
                dock->show();
                tabWidget->setCurrent(dock);
            }
        }
        else {
            tabWidget->saveTabs();
        }
        return true;
    }

    void removeWidget()
    {
        if (!tabWidget->count()) {
            return;
        }

        tabWidget->hide();

        QPointer<QWidget> focus = qApp->focusWidget();

        QDockWidget* lastDock = tabWidget->currentDockWidget();
        if (lastDock) {
            tabWidget->removeWidget(lastDock);
        }
        while (tabWidget->count()) {
            QDockWidget* dock = tabWidget->dockWidget(0);
            if (!dock) {
                tabWidget->removeTab(0);
                continue;
            }
            tabWidget->removeWidget(dock, lastDock);
            lastDock = dock;
        }

        if (focus) {
            focus->setFocus();
        }

        tabWidget->saveTabs();
    }

    void save()
    {}

    void restore()
    {
        tabWidget->restore(hGrp);
        for (int i = 0, c = tabWidget->count(); i < c; ++i) {
            auto dock = tabWidget->dockWidget(i);
            if (dock) {
                overlayMap[dock] = this;
            }
        }
    }
};

enum class ToggleMode
{
    Unset,
    Set,
    Toggle,
    Transparent,
    Check,
};

class OverlayManager::Private
{
public:
    QPointer<QWidget> lastIntercept;
    QTimer _timer;
    QTimer _reloadTimer;

    bool mouseTransparent = false;
    bool intercepting = false;

    std::unordered_map<QDockWidget*, OverlayInfo*> _overlayMap;
    OverlayInfo _left;
    OverlayInfo _right;
    OverlayInfo _top;
    OverlayInfo _bottom;
    std::array<OverlayInfo*, 4> _overlayInfos;
    QCursor _cursor;

    QPoint _lastPos;

    QAction _actClose;
    QAction _actFloat;
    QAction _actOverlay;
    QList<QAction*> _actions;

    QPointer<QWidget> _trackingWidget;
    OverlayTabWidget* _trackingOverlay = nullptr;

    bool updateStyle = false;
    QTime wheelDelay;
    QPoint wheelPos;

    std::map<QString, OverlayTabWidget*> _dockWidgetNameMap;
    bool compactRailTabOwnershipEnabled = _compactRailTabOwnershipEnabled;

    bool raising = false;

    OverlayManager::ReloadMode curReloadMode = OverlayManager::ReloadMode::ReloadPending;

    Private(OverlayManager* host, QWidget* parent)
        : _left(parent, "OverlayLeft", Qt::LeftDockWidgetArea, _overlayMap)
        , _right(parent, "OverlayRight", Qt::RightDockWidgetArea, _overlayMap)
        , _top(parent, "OverlayTop", Qt::TopDockWidgetArea, _overlayMap)
        , _bottom(parent, "OverlayBottom", Qt::BottomDockWidgetArea, _overlayMap)
        , _overlayInfos({&_left, &_right, &_top, &_bottom})
        , _actions({&_actOverlay, &_actFloat, &_actClose})
    {
        _Overlays = {
            OverlayTabWidget::_LeftOverlay,
            OverlayTabWidget::_RightOverlay,
            OverlayTabWidget::_TopOverlay,
            OverlayTabWidget::_BottomOverlay
        };

        connect(&_timer, &QTimer::timeout, [this]() { onTimer(); });
        _timer.setSingleShot(true);

        _reloadTimer.setSingleShot(true);
        connect(&_reloadTimer, &QTimer::timeout, [this]() {
            for (auto& o : _overlayInfos) {
                o->tabWidget->restore(nullptr);  // prevent saving setting first
                o->removeWidget();
            }
            for (auto& o : _overlayInfos) {
                o->restore();
            }
            refresh();
        });

        connect(qApp, &QApplication::focusChanged, host, &OverlayManager::onFocusChanged);

        Application::Instance->signalActivateView.connect([this](const MDIView*) { refresh(); });
        Application::Instance->signalInEdit.connect([this](const ViewProviderDocumentObject&) {
            refresh();
        });
        Application::Instance->signalResetEdit.connect([this](const ViewProviderDocumentObject&) {
            refresh();
        });
        Application::Instance->signalActivateWorkbench.connect([this](const char*) { refresh(); });

        _actOverlay.setData(QStringLiteral("OBTN Overlay"));
        _actFloat.setData(QStringLiteral("OBTN Float"));
        _actClose.setData(QStringLiteral("OBTN Close"));

        retranslate();
        refreshIcons();
        applyCompactRailTabOwnership();

        for (auto action : _actions) {
            QObject::connect(action, &QAction::triggered, host, &OverlayManager::onAction);
        }
        for (auto o : _overlayInfos) {
            for (auto action : o->tabWidget->actions()) {
                QObject::connect(action, &QAction::triggered, host, &OverlayManager::onAction);
            }
            o->tabWidget->setTitleBar(createTitleBar(o->tabWidget));
        }

        QIcon px = BitmapFactory().pixmap("cursor-through");
        _cursor = QCursor(px.pixmap(32, 32), 10, 9);
    }

    void refreshIcons()
    {
        _actFloat.setIcon(BitmapFactory().pixmap("qss:overlay/icons/float.svg"));
        _actOverlay.setIcon(BitmapFactory().pixmap("qss:overlay/icons/overlay.svg"));
        _actClose.setIcon(BitmapFactory().pixmap("qss:overlay/icons/close.svg"));
        for (OverlayTabWidget* tabWidget : _Overlays) {
            tabWidget->refreshIcons();
            for (auto handle : tabWidget->findChildren<OverlaySplitterHandle*>()) {
                handle->refreshIcons();
            }
        }
    }

    void interceptEvent(QWidget*, QEvent*);

    void setMouseTransparent(bool enabled)
    {
        if (mouseTransparent == enabled) {
            return;
        }
        mouseTransparent = enabled;
        for (OverlayTabWidget* tabWidget : _Overlays) {
            tabWidget->setAttribute(Qt::WA_TransparentForMouseEvents, enabled);
            tabWidget->touch();
        }
        refresh();
        if (!enabled) {
            qApp->restoreOverrideCursor();
        }
        else {
            qApp->setOverrideCursor(_cursor);
        }
    }

    OverlayInfo* overlayInfoForArea(int dockPos)
    {
        switch (dockPos) {
            case Qt::LeftDockWidgetArea:
                return &_left;
            case Qt::RightDockWidgetArea:
                return &_right;
            case Qt::TopDockWidgetArea:
                return &_top;
            case Qt::BottomDockWidgetArea:
                return &_bottom;
            default:
                return nullptr;
        }
    }

    bool canResolveOverlayInfo(QDockWidget* dock, int& dockPos)
    {
        if (!dock || !getMainWindow()) {
            return false;
        }
        if (dockPos == Qt::NoDockWidgetArea) {
            dockPos = getMainWindow()->dockWidgetArea(dock);
        }
        return overlayInfoForArea(dockPos) != nullptr;
    }

    Qt::DockWidgetArea dockWidgetOverlayArea(QDockWidget* dock) const
    {
        auto it = _overlayMap.find(dock);
        if (it == _overlayMap.end() || !it->second) {
            return Qt::NoDockWidgetArea;
        }

        return it->second->dockArea;
    }

    bool toggleOverlay(
        QDockWidget* dock,
        ToggleMode toggle,
        int dockPos = Qt::NoDockWidgetArea,
        bool forced = true
    )
    {
        if (!dock) {
            return false;
        }

        auto it = _overlayMap.find(dock);
        if (it != _overlayMap.end()) {
            auto o = it->second;
            switch (toggle) {
                case ToggleMode::Transparent:
                    o->tabWidget->setTransparent(!o->tabWidget->isTransparent());
                    break;
                case ToggleMode::Unset:
                case ToggleMode::Toggle:
                    _overlayMap.erase(it);
                    o->tabWidget->removeWidget(dock);
                    return false;
                default:
                    break;
            }
            return true;
        }

        if (toggle == ToggleMode::Unset) {
            return false;
        }

        if (dockPos == Qt::NoDockWidgetArea) {
            dockPos = getMainWindow()->dockWidgetArea(dock);
        }
        OverlayInfo* o = overlayInfoForArea(dockPos);
        if (!o) {
            return false;
        }
        if (toggle == ToggleMode::Check && !o->tabWidget->count()) {
            return false;
        }
        if (o->addWidget(dock, forced)) {
            if (toggle == ToggleMode::Transparent) {
                o->tabWidget->setTransparent(true);
            }
        }
        refresh();
        return true;
    }

    void refresh(QWidget* widget = nullptr, bool refreshStyle = false)
    {
        if (refreshStyle) {
            OverlayStyleSheet::instance()->update();
            updateStyle = true;
        }

        if (widget) {
            auto tabWidget = findTabWidget(widget);
            if (tabWidget && tabWidget->count()) {
                for (auto o : _overlayInfos) {
                    if (tabWidget == o->tabWidget) {
                        tabWidget->touch();
                        break;
                    }
                }
            }
        }
        _timer.start(OverlayParams::getDockOverlayDelay());
    }

    void save()
    {
        _left.save();
        _right.save();
        _top.save();
        _bottom.save();
    }

    void restore()
    {
        _left.restore();
        _right.restore();
        _top.restore();
        _bottom.restore();
        refresh();
    }

    void onTimer()
    {
        auto mdi = getMainWindow() ? getMainWindow()->getMdiArea() : nullptr;
        if (!mdi) {
            return;
        }

        auto focus = findTabWidget(qApp->focusWidget());
        if (focus && !focus->getSplitter()->isVisible()) {
            focus = nullptr;
        }
        auto active = findTabWidget(qApp->widgetAt(QCursor::pos()));
        if (active && !active->getSplitter()->isVisible()) {
            active = nullptr;
        }
        OverlayTabWidget* reveal = nullptr;

        bool updateFocus = false;
        bool updateActive = false;

        for (auto o : _overlayInfos) {
            if (o->tabWidget->isTouched() || updateStyle) {
                if (o->tabWidget == focus) {
                    updateFocus = true;
                }
                else if (o->tabWidget == active) {
                    updateActive = true;
                }
                else {
                    o->tabWidget->setOverlayMode(true);
                }
            }
            if (!o->tabWidget->getRevealTime().isNull()) {
                if (o->tabWidget->getRevealTime() <= QTime::currentTime()) {
                    o->tabWidget->setRevealTime(QTime());
                }
                else {
                    reveal = o->tabWidget;
                }
            }
        }
        updateStyle = false;

        if (focus) {
            if (focus->isOverlaid(OverlayTabWidget::QueryOption::TransparencyChanged) || updateFocus) {
                focus->setOverlayMode(false);
                focus->raise();
                if (reveal == focus) {
                    reveal = nullptr;
                }
            }
            else {
                focus->updateSplitterHandles();
            }
        }

        if (active) {
            if (active != focus
                && (active->isOverlaid(OverlayTabWidget::QueryOption::TransparencyChanged)
                    || updateActive)) {
                active->setOverlayMode(false);
            }
            active->raise();
            if (reveal == active) {
                reveal = nullptr;
            }
        }

        if (reveal && !reveal->splitter->isVisible()) {
            reveal->setOverlayMode(false);
            reveal->raise();
        }

        for (auto o : _overlayInfos) {
            if (o->tabWidget != focus && o->tabWidget != active && o->tabWidget != reveal
                && o->tabWidget->count()
                && !o->tabWidget->isOverlaid(OverlayTabWidget::QueryOption::TransparencyNotChanged)) {
                o->tabWidget->setOverlayMode(true);
            }
        }

        int w = mdi->geometry().width();
        int h = mdi->geometry().height();
        auto tabbar = mdi->findChild<QTabBar*>();
        if (tabbar) {
            h -= tabbar->height();
        }

        int naviCubeSize = NaviCube::getNaviCubeSize();
        int naviCorner = OverlayParams::getDockOverlayCheckNaviCube()
            ? App::GetApplication()
                  .GetParameterGroupByPath("User parameter:BaseApp/Preferences/NaviCube")
                  ->GetInt("CornerNaviCube", 1)
            : -1;

        QRect rect;
        QRect rectBottom(0, 0, 0, 0);

        rect = _bottom.tabWidget->getRect();

        QSize ofs = _bottom.tabWidget->getOffset();
        int delta = _bottom.tabWidget->getSizeDelta();
        h -= ofs.height();

        const int paddedCubeSize = naviCubeSize + 10;
        if (naviCorner == 2) {
            ofs.setWidth(ofs.width() + paddedCubeSize);
        }
        int bw = w - 10 - ofs.width() - delta;
        if (naviCorner == 3) {
            bw -= paddedCubeSize;
        }
        if (bw < 10) {
            bw = 10;
        }

        // Bottom width is maintain the same to reduce QTextEdit re-layout
        // which may be expensive if there are lots of text, e.g. for
        // ReportView or PythonConsole.
        _bottom.tabWidget->setRect(QRect(ofs.width(), h - rect.height(), bw, rect.height()));

        if (_bottom.tabWidget->count() && _bottom.tabWidget->isVisible()
            && _bottom.tabWidget->getState() <= OverlayTabWidget::State::Normal) {
            rectBottom = _bottom.tabWidget->getRect();
        }

        QRect rectLeft(0, 0, 0, 0);
        rect = _left.tabWidget->getRect();

        ofs = _left.tabWidget->getOffset();
        if (naviCorner == 0) {
            ofs.setWidth(ofs.width() + paddedCubeSize);
        }
        delta = _left.tabWidget->getSizeDelta() + rectBottom.height();
        if (naviCorner == 2 && paddedCubeSize > rectBottom.height()) {
            delta += paddedCubeSize - rectBottom.height();
        }
        int lh = std::max(h - ofs.width() - delta, 10);

        _left.tabWidget->setRect(QRect(ofs.height(), ofs.width(), rect.width(), lh));

        if (_left.tabWidget->count() && _left.tabWidget->isVisible()
            && _left.tabWidget->getState() <= OverlayTabWidget::State::Normal) {
            rectLeft = _left.tabWidget->getRect();
        }

        QRect rectRight(0, 0, 0, 0);
        rect = _right.tabWidget->getRect();

        ofs = _right.tabWidget->getOffset();
        if (naviCorner == 1) {
            ofs.setWidth(ofs.width() + paddedCubeSize);
        }
        delta = _right.tabWidget->getSizeDelta() + rectBottom.height();
        if (naviCorner == 3 && paddedCubeSize > rectBottom.height()) {
            delta += paddedCubeSize - rectBottom.height();
        }
        int rh = std::max(h - ofs.width() - delta, 10);
        w -= ofs.height();

        _right.tabWidget->setRect(QRect(w - rect.width(), ofs.width(), rect.width(), rh));

        if (_right.tabWidget->count() && _right.tabWidget->isVisible()
            && _right.tabWidget->getState() <= OverlayTabWidget::State::Normal) {
            rectRight = _right.tabWidget->getRect();
        }

        rect = _top.tabWidget->getRect();

        ofs = _top.tabWidget->getOffset();
        delta = _top.tabWidget->getSizeDelta();
        if (naviCorner == 0) {
            rectLeft.setWidth(std::max(rectLeft.width(), paddedCubeSize));
        }
        else if (naviCorner == 1) {
            rectRight.setWidth(std::max(rectRight.width(), paddedCubeSize));
        }
        int tw = w - rectLeft.width() - rectRight.width() - ofs.width() - delta;

        _top.tabWidget->setRect(QRect(rectLeft.width() - ofs.width(), ofs.height(), tw, rect.height()));
    }

    void setOverlayMode(OverlayMode mode)
    {
        switch (mode) {
            case OverlayManager::OverlayMode::DisableAll:
            case OverlayManager::OverlayMode::EnableAll: {
                auto docks = getMainWindow()->findChildren<QDockWidget*>();
                // put visible dock widget first
                std::sort(docks.begin(), docks.end(), [](const QDockWidget* a, const QDockWidget* b) {
                    return !a->visibleRegion().isEmpty() && b->visibleRegion().isEmpty();
                });
                for (auto dock : docks) {
                    if (mode == OverlayManager::OverlayMode::DisableAll) {
                        toggleOverlay(dock, ToggleMode::Unset);
                    }
                    else {
                        toggleOverlay(dock, ToggleMode::Set);
                    }
                }
                return;
            }
            case OverlayManager::OverlayMode::ToggleAll:
                for (auto o : _overlayInfos) {
                    if (o->tabWidget->count()) {
                        setOverlayMode(OverlayManager::OverlayMode::DisableAll);
                        return;
                    }
                }
                setOverlayMode(OverlayManager::OverlayMode::EnableAll);
                return;
            case OverlayManager::OverlayMode::TransparentAll: {
                bool found = false;
                for (auto o : _overlayInfos) {
                    if (o->tabWidget->count()) {
                        found = true;
                    }
                }
                if (!found) {
                    setOverlayMode(OverlayManager::OverlayMode::EnableAll);
                }
            }
            // fall through
            case OverlayManager::OverlayMode::TransparentNone:
                for (auto o : _overlayInfos) {
                    o->tabWidget->setTransparent(mode == OverlayManager::OverlayMode::TransparentAll);
                }
                refresh();
                return;
            case OverlayManager::OverlayMode::ToggleTransparentAll:
                for (auto o : _overlayInfos) {
                    if (o->tabWidget->count() && o->tabWidget->isTransparent()) {
                        setOverlayMode(OverlayManager::OverlayMode::TransparentNone);
                        return;
                    }
                }
                setOverlayMode(OverlayManager::OverlayMode::TransparentAll);
                return;
            case OverlayManager::OverlayMode::ToggleLeft:
                if (OverlayTabWidget::_LeftOverlay->isVisible()) {
                    OverlayTabWidget::_LeftOverlay->setState(OverlayTabWidget::State::Hidden);
                }
                else {
                    OverlayTabWidget::_LeftOverlay->setState(OverlayTabWidget::State::Showing);
                }
                break;
            case OverlayManager::OverlayMode::ToggleRight:
                if (OverlayTabWidget::_RightOverlay->isVisible()) {
                    OverlayTabWidget::_RightOverlay->setState(OverlayTabWidget::State::Hidden);
                }
                else {
                    OverlayTabWidget::_RightOverlay->setState(OverlayTabWidget::State::Showing);
                }
                break;
            case OverlayManager::OverlayMode::ToggleTop:
                if (OverlayTabWidget::_TopOverlay->isVisible()) {
                    OverlayTabWidget::_TopOverlay->setState(OverlayTabWidget::State::Hidden);
                }
                else {
                    OverlayTabWidget::_TopOverlay->setState(OverlayTabWidget::State::Showing);
                }
                break;
            case OverlayManager::OverlayMode::ToggleBottom:
                if (OverlayTabWidget::_BottomOverlay->isVisible()) {
                    OverlayTabWidget::_BottomOverlay->setState(OverlayTabWidget::State::Hidden);
                }
                else {
                    OverlayTabWidget::_BottomOverlay->setState(OverlayTabWidget::State::Showing);
                }
                break;
            default:
                break;
        }

        ToggleMode m;
        QDockWidget* dock = nullptr;
        for (auto w = qApp->widgetAt(QCursor::pos()); w; w = w->parentWidget()) {
            dock = qobject_cast<QDockWidget*>(w);
            if (dock) {
                break;
            }
            auto tabWidget = qobject_cast<OverlayTabWidget*>(w);
            if (tabWidget) {
                dock = tabWidget->currentDockWidget();
                if (dock) {
                    break;
                }
            }
        }
        if (!dock) {
            for (auto w = qApp->focusWidget(); w; w = w->parentWidget()) {
                dock = qobject_cast<QDockWidget*>(w);
                if (dock) {
                    break;
                }
            }
        }

        switch (mode) {
            case OverlayManager::OverlayMode::ToggleActive:
                m = ToggleMode::Toggle;
                break;
            case OverlayManager::OverlayMode::ToggleTransparent:
                m = ToggleMode::Transparent;
                break;
            case OverlayManager::OverlayMode::EnableActive:
                m = ToggleMode::Set;
                break;
            case OverlayManager::OverlayMode::DisableActive:
                m = ToggleMode::Unset;
                break;
            default:
                return;
        }
        toggleOverlay(dock, m);
    }

    void onToggleDockWidget(QDockWidget* dock, int checked)
    {
        if (!dock) {
            return;
        }

        auto it = _overlayMap.find(dock);
        if (it == _overlayMap.end()) {
            return;
        }

        OverlayTabWidget* tabWidget = it->second->tabWidget;
        int index = tabWidget->dockWidgetIndex(dock);
        if (index < 0) {
            return;
        }
        auto sizes = tabWidget->getSplitter()->sizes();
        while (index >= sizes.size()) {
            sizes.append(0);
        }

        if (checked < -1) {
            checked = 0;
        }
        else if (checked == 3) {
            checked = 1;
            sizes[index] = 0;  // force expand the tab in full
        }
        else if (checked <= 1) {
            if (sizes[index] != 0 && tabWidget->isHidden()) {
                checked = 1;
            }
            else {
                // child widget inside splitter by right shouldn't been hidden, so
                // we ignore the given toggle bit, but rely on its splitter size to
                // decide.
                checked = sizes[index] == 0 ? 1 : 0;
            }
        }
        if (sizes[index] == 0) {
            if (!checked) {
                return;
            }
            tabWidget->setCurrent(dock);
            tabWidget->onCurrentChanged(tabWidget->dockWidgetIndex(dock));
        }
        else if (!checked) {
            if (sizes[index] > 0 && sizes.size() == 1) {
                // A one-panel overlay still has to honour the shared dock toggle action. The
                // legacy code only collapsed splitters with peers, leaving the sole panel
                // permanently open and its rail/View action unable to toggle it back.
                auto newsizes = sizes;
                newsizes[index] = 0;
                tabWidget->splitter->setSizes(newsizes);
                tabWidget->setState(OverlayTabWidget::State::Hidden);
                tabWidget->saveTabs();
            }
            else if (sizes[index] > 0 && sizes.size() > 1) {
                int newtotal = 0;
                auto newsizes = sizes;
                newsizes[index] = 0;
                for (int i = 0; i < sizes.size(); ++i) {
                    if (i != index) {
                        auto d = tabWidget->dockWidget(i);
                        auto it = tabWidget->_sizemap.find(d);
                        if (it == tabWidget->_sizemap.end()) {
                            newsizes[i] = 0;
                        }
                        else {
                            if (newtotal == 0) {
                                tabWidget->setCurrent(d);
                            }
                            newsizes[i] = it->second;
                            newtotal += it->second;
                        }
                    }
                }
                if (!newtotal) {
                    int expand = 0;
                    for (int i = 0; i < sizes.size(); ++i) {
                        if (i != index && sizes[i] > 0) {
                            ++expand;
                            break;
                        }
                    }
                    if (expand) {
                        int expansion = sizes[index];
                        int step = expansion / expand;
                        for (int i = 0; i < sizes.size(); ++i) {
                            if (i == index) {
                                newsizes[i] = 0;
                            }
                            else if (--expand) {
                                expansion -= step;
                                newsizes[i] += step;
                            }
                            else {
                                newsizes[i] += expansion;
                                break;
                            }
                        }
                    }
                    newsizes[index] = 0;
                }
                tabWidget->splitter->setSizes(newsizes);
                tabWidget->saveTabs();
            }
        }
        for (int i = 0; i < sizes.size(); ++i) {
            if (sizes[i]) {
                tabWidget->_sizemap[tabWidget->dockWidget(i)] = sizes[i];
            }
        }
        if (checked) {
            // QDockWidget's own toggle action hides the child on close. Reopening an overlay must
            // remap that same dock after the splitter state is restored; otherwise the action is
            // checked and the rail looks active while the panel surface remains absent.
            dock->show();
            tabWidget->startShow();
            tabWidget->setRevealTime(
                QTime::currentTime().addMSecs(OverlayParams::getDockOverlayRevealDelay())
            );
        }
        refresh();
    }

    void onFocusChanged(QWidget*, QWidget*)
    {
        refresh();
    }

    bool usesCanonicalHeaderOwnership() const
    {
        return compactRailTabOwnershipEnabled;
    }

    static bool hasTitleRole(const QWidget* widget, const char* role)
    {
        return widget && widget->property(OverlayTitleRoleProperty).toByteArray() == role;
    }

    static void setHeaderOwnerProperty(QDockWidget* dock, bool ownedByOverlay)
    {
        if (!dock) {
            return;
        }
        dock->setProperty(
            OverlayHeaderOwnerProperty,
            ownedByOverlay ? OverlayHeaderOwnerOverlay : OverlayHeaderOwnerDock
        );
    }

    static bool isHeaderOwnedByOverlay(const QDockWidget* dock)
    {
        return dock
            && dock->property(OverlayHeaderOwnerProperty).toByteArray() == OverlayHeaderOwnerOverlay;
    }

    QWidget* createTitlePlaceholder(QDockWidget* dock)
    {
        auto* widget = new QWidget(dock);
        widget->setObjectName(QStringLiteral("OverlayTitle"));
        widget->setProperty(OverlayTitleRoleProperty, OverlayTitleRolePlaceholder);
        widget->hide();
        return widget;
    }

    QDockWidget* titleDockForParent(QWidget* parent) const
    {
        if (auto dockWidget = qobject_cast<QDockWidget*>(parent)) {
            return dockWidget;
        }
        auto* tabWidget = qobject_cast<OverlayTabWidget*>(parent);
        if (!tabWidget) {
            return nullptr;
        }
        if (auto* currentDock = tabWidget->currentDockWidget()) {
            return currentDock;
        }
        return tabWidget->dockWidget(0);
    }

    QList<QAction*> titleActionsForDock(
        QDockWidget* dockWidget,
        bool includeOverlayToggle,
        bool forceFloatAction = false
    )
    {
        QList<QAction*> actions;
        if (!dockWidget) {
            return actions;
        }
        const QDockWidget::DockWidgetFeatures features = dockWidget->features();
        for (auto action : dockWidget->actions()) {
            if (action->property("DockTitleBarAction").toBool()) {
                actions.append(action);
            }
        }
        if (includeOverlayToggle) {
            actions.append(&_actOverlay);
        }
        if (forceFloatAction || features.testFlag(QDockWidget::DockWidgetFloatable)) {
            actions.append(&_actFloat);
        }
        if (features.testFlag(QDockWidget::DockWidgetClosable)) {
            actions.append(&_actClose);
        }
        return actions;
    }

    void setupTitleBar(QDockWidget* dock)
    {
        auto* oldWidget = dock->titleBarWidget();
        if (usesCanonicalHeaderOwnership()) {
            if (isHeaderOwnedByOverlay(dock)) {
                dock->setTitleBarWidget(createTitlePlaceholder(dock));
            }
            else {
                dock->setTitleBarWidget(createTitleBar(dock));
            }
        }
        else {
            dock->setTitleBarWidget(createTitleBar(dock));
        }
        if (oldWidget) {
            oldWidget->deleteLater();
        }
    }

    QWidget* createTitleBar(QWidget* parent)
    {
        OverlayTitleBar* widget = new OverlayTitleBar(parent);
        widget->setObjectName(QStringLiteral("OverlayTitle"));
        widget->setProperty(
            OverlayTitleRoleProperty,
            qobject_cast<OverlayTabWidget*>(parent) ? OverlayTitleRoleOverlayHost : OverlayTitleRoleDock
        );
        if (usesCanonicalHeaderOwnership()) {
            if (auto* tabWidget = qobject_cast<OverlayTabWidget*>(parent); tabWidget
                && (tabWidget->getDockArea() == Qt::TopDockWidgetArea
                    || tabWidget->getDockArea() == Qt::BottomDockWidgetArea)) {
                widget->setFixedWidth(CompactTitleBarStyle::panelHeaderHeight());
            }
            else {
                widget->setFixedHeight(CompactTitleBarStyle::panelHeaderHeight());
            }
        }

        QList<QAction*> actions;
        if (auto tabWidget = qobject_cast<OverlayTabWidget*>(parent)) {
            if (usesCanonicalHeaderOwnership()) {
                actions = titleActionsForDock(titleDockForParent(tabWidget), true, true);
            }
            else {
                actions = tabWidget->actions();
            }
        }
        else if (auto dockWidget = qobject_cast<QDockWidget*>(parent)) {
            actions = titleActionsForDock(dockWidget, true);
        }
        else {
            actions = _actions;
        }

        widget->setTitleItem(
            OverlayTabWidget::prepareTitleWidget(widget, actions, usesCanonicalHeaderOwnership() ? 3 : -1)
        );
        return widget;
    }

    void refreshOverlayTitleBar(OverlayTabWidget* tabWidget)
    {
        if (!tabWidget || !usesCanonicalHeaderOwnership()) {
            return;
        }

        QWidget* oldWidget = tabWidget->getTitleBar();
        QWidget* newWidget = createTitleBar(tabWidget);
        tabWidget->setTitleBar(newWidget);
        if (oldWidget) {
            oldWidget->deleteLater();
        }
        tabWidget->refreshTitleBarLayout();
        newWidget->update();
    }

    void onAction(QAction* action)
    {
        if (action == &_actOverlay) {
            OverlayManager::instance()->setOverlayMode(OverlayManager::OverlayMode::ToggleActive);
        }
        else if (action == &_actFloat || action == &_actClose) {
            for (auto w = qApp->widgetAt(QCursor::pos()); w; w = w->parentWidget()) {
                auto dock = qobject_cast<QDockWidget*>(w);
                if (!dock) {
                    continue;
                }
                setFocusView();
                if (action == &_actClose) {
                    dock->toggleViewAction()->activate(QAction::Trigger);
                }
                else {
                    auto it = _overlayMap.find(dock);
                    if (it != _overlayMap.end()) {
                        it->second->tabWidget->removeWidget(dock);
                        getMainWindow()->addDockWidget(it->second->dockArea, dock);
                        _overlayMap.erase(it);
                        dock->show();
                        dock->setFloating(true);
                        refresh();
                    }
                    else {
                        dock->setFloating(!dock->isFloating());
                    }
                }
                return;
            }
        }
        else {
            auto tabWidget = qobject_cast<OverlayTabWidget*>(action->parent());
            if (tabWidget) {
                tabWidget->onAction(action);
            }
        }
    }

    void retranslate()
    {
        _actOverlay.setToolTip(QObject::tr("Toggle overlay"));
        _actFloat.setToolTip(QObject::tr("Toggle floating window"));
        _actClose.setToolTip(QObject::tr("Close dock window"));
    }

    void floatDockWidget(QDockWidget* dock)
    {
        setFocusView();
        auto it = _overlayMap.find(dock);
        if (it != _overlayMap.end()) {
            it->second->tabWidget->removeWidget(dock);
            _overlayMap.erase(it);
        }
        dock->setFloating(true);
        dock->show();
    }

    // Warning, the caller may be deleted during the call. So do not pass
    // parameter using reference, pass by value instead.
    void dragDockWidget(QPoint pos, QWidget* srcWidget, QPoint dragOffset, QSize dragSize, bool drop)
    {
        if (!getMainWindow()) {
            return;
        }
        auto mdi = getMainWindow()->getMdiArea();
        if (!mdi) {
            return;
        }

        auto dock = qobject_cast<QDockWidget*>(srcWidget);
        if (dock && dock->isFloating()) {
            dock->move(pos - dragOffset);
        }

        OverlayTabWidget* src = nullptr;
        int srcIndex = -1;
        if (dock) {
            auto it = _overlayMap.find(dock);
            if (it != _overlayMap.end()) {
                src = it->second->tabWidget;
                srcIndex = src->dockWidgetIndex(dock);
            }
        }
        else {
            src = qobject_cast<OverlayTabWidget*>(srcWidget);
            if (!src) {
                return;
            }
            for (int size : src->getSplitter()->sizes()) {
                ++srcIndex;
                if (size) {
                    dock = src->dockWidget(srcIndex);
                    break;
                }
            }
            if (!dock) {
                return;
            }
        }

        OverlayTabWidget* tabWidget = nullptr;
        int resizeOffset = 0;
        int index = -1;
        QRect rect;
        QRect rectMain(getMainWindow()->mapToGlobal(QPoint()), getMainWindow()->size());
        QRect rectMdi(mdi->mapToGlobal(QPoint()), mdi->size());

        for (OverlayTabWidget* overlay : _Overlays) {
            rect = QRect(mdi->mapToGlobal(overlay->rectOverlay.topLeft()), overlay->rectOverlay.size());

            QSize size(rect.width() * 3 / 4, rect.height() * 3 / 4);
            QSize sideSize(rect.width() / 4, rect.height() / 4);

            int dockArea = overlay->getDockArea();

            if (dockArea == Qt::BottomDockWidgetArea) {
                if (pos.y() < rect.bottom() && rect.bottom() - pos.y() < sideSize.height()) {
                    rect.setTop(rect.bottom() - OverlayParams::getDockOverlayMinimumSize());
                    rect.setBottom(rectMain.bottom());
                    rect.setLeft(rectMdi.left());
                    rect.setRight(rectMdi.right());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }
            if (dockArea == Qt::LeftDockWidgetArea) {
                if (pos.x() >= rect.left() && pos.x() - rect.left() < sideSize.width()) {
                    rect.setRight(rect.left() + OverlayParams::getDockOverlayMinimumSize());
                    rect.setLeft(rectMain.left());
                    rect.setTop(rectMdi.top());
                    rect.setBottom(rectMdi.bottom());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }
            else if (dockArea == Qt::RightDockWidgetArea) {
                if (pos.x() < rect.right() && rect.right() - pos.x() < sideSize.width()) {
                    rect.setLeft(rect.right() - OverlayParams::getDockOverlayMinimumSize());
                    rect.setRight(rectMain.right());
                    rect.setTop(rectMdi.top());
                    rect.setBottom(rectMdi.bottom());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }
            else if (dockArea == Qt::TopDockWidgetArea) {
                if (pos.y() >= rect.top() && pos.y() - rect.top() < sideSize.height()) {
                    rect.setBottom(rect.top() + OverlayParams::getDockOverlayMinimumSize());
                    rect.setTop(rectMain.top());
                    rect.setLeft(rectMdi.left());
                    rect.setRight(rectMdi.right());
                    tabWidget = overlay;
                    index = -2;
                    break;
                }
            }

            switch (dockArea) {
                case Qt::LeftDockWidgetArea:
                    rect.setWidth(size.width());
                    break;
                case Qt::RightDockWidgetArea:
                    rect.setLeft(rect.right() - size.width());
                    break;
                case Qt::TopDockWidgetArea:
                    rect.setHeight(size.height());
                    break;
                default:
                    rect.setTop(rect.bottom() - size.height());
                    break;
            }

            if (!rect.contains(pos)) {
                continue;
            }

            tabWidget = overlay;
            index = -1;
            int i = -1;

            for (int size : overlay->getSplitter()->sizes()) {
                ++i;
                auto handle = overlay->getSplitter()->handle(i);
                QWidget* w = overlay->dockWidget(i);
                if (!handle || !w) {
                    continue;
                }
                if (handle->rect().contains(handle->mapFromGlobal(pos))) {
                    QPoint pt = handle->mapToGlobal(QPoint());
                    QSize s = handle->size();
                    if (!size) {
                        size = OverlayParams::getDockOverlayMinimumSize();
                    }
                    if (tabWidget != src) {
                        size /= 2;
                    }
                    if (overlay->getSplitter()->orientation() == Qt::Vertical) {
                        s.setHeight(s.height() + size);
                    }
                    else {
                        s.setWidth(s.width() + size);
                    }
                    rect = QRect(pt, s);
                    index = i;
                    break;
                }
                if (!size) {
                    continue;
                }
                if (w->rect().contains(w->mapFromGlobal(pos))) {
                    QPoint pt = overlay->getSplitter()->mapToGlobal(w->pos());
                    rect = QRect(pt, w->size());
                    if (tabWidget != src) {
                        if (overlay->getSplitter()->orientation() == Qt::Vertical) {
                            if (pos.y() > pt.y() + size / 2) {
                                rect.setTop(rect.top() + size / 2);
                                resizeOffset = -1;
                                ++i;
                            }
                            else {
                                rect.setHeight(size / 2);
                            }
                        }
                        else if (pos.x() > pt.x() + size / 2) {
                            rect.setLeft(rect.left() + size / 2);
                            resizeOffset = -1;
                            ++i;
                        }
                        else {
                            rect.setWidth(size / 2);
                        }
                    }
                    index = i;
                    break;
                }
            }
            break;
        };

        OverlayTabWidget* dst = nullptr;
        int dstIndex = -1;
        QDockWidget* dstDock = nullptr;
        Qt::DockWidgetArea dstDockArea {};

        if (!tabWidget) {
            rect = QRect(pos - dragOffset, dragSize);
            if (rect.width() < 50) {
                rect.setWidth(50);
            }
            if (rect.height() < 50) {
                rect.setHeight(50);
            }

            for (auto dockWidget : getMainWindow()->findChildren<QDockWidget*>()) {
                if (dockWidget == dock || !dockWidget->isVisible() || dockWidget->isFloating()
                    || _overlayMap.contains(dockWidget)) {
                    continue;
                }
                if (dockWidget->rect().contains(dockWidget->mapFromGlobal(pos))) {
                    dstDock = dockWidget;
                    dstDockArea = getMainWindow()->dockWidgetArea(dstDock);
                    rect = QRect(dockWidget->mapToGlobal(QPoint()), dockWidget->size());
                    break;
                }
            }
        }
        else {
            dst = tabWidget;
            dstIndex = index;
            if (dstIndex == -1) {
                rect = QRect(
                    mdi->mapToGlobal(tabWidget->rectOverlay.topLeft()),
                    tabWidget->rectOverlay.size()
                );
            }
        }

        bool outside = false;
        if (!rectMain.contains(pos)) {
            outside = true;
            if (drop) {
                if (!dock->isFloating()) {
                    if (src) {
                        _overlayMap.erase(dock);
                        src->removeWidget(dock);
                    }
                    setFocusView();
                    dock->setFloating(true);
                    dock->move(pos - dragOffset);
                    dock->show();
                }
                if (OverlayTabWidget::_DragFloating) {
                    OverlayTabWidget::_DragFloating->hide();
                }
            }
            else if (!dock->isFloating()) {
                if (!OverlayTabWidget::_DragFloating) {
                    OverlayTabWidget::_DragFloating = new QDockWidget(getMainWindow());
                    OverlayTabWidget::_DragFloating->setFloating(true);
                }
                OverlayTabWidget::_DragFloating->resize(dock->size());
                OverlayTabWidget::_DragFloating->setWindowTitle(dock->windowTitle());
                OverlayTabWidget::_DragFloating->show();
                OverlayTabWidget::_DragFloating->move(pos - dragOffset);
            }
            if (OverlayTabWidget::_DragFrame) {
                OverlayTabWidget::_DragFrame->hide();
            }
            return;
        }
        else if (!drop && OverlayTabWidget::_DragFrame && !OverlayTabWidget::_DragFrame->isVisible()) {
            OverlayTabWidget::_DragFrame->raise();
            OverlayTabWidget::_DragFrame->show();
            if (OverlayTabWidget::_DragFloating) {
                OverlayTabWidget::_DragFloating->hide();
            }
        }

        int insertDock = 0;  // 0: tabify, -1: insert before, 1: insert after
        if (!dst && dstDock) {
            switch (dstDockArea) {
                case Qt::LeftDockWidgetArea:
                case Qt::RightDockWidgetArea:
                    if (pos.y() < rect.top() + rect.height() / 4) {
                        insertDock = -1;
                        rect.setBottom(rect.top() + rect.height() / 2);
                    }
                    else if (pos.y() > rect.bottom() - rect.height() / 4) {
                        insertDock = 1;
                        int height = rect.height();
                        rect.setTop(rect.bottom() - height / 4);
                        rect.setHeight(height / 2);
                    }
                    break;
                default:
                    if (pos.x() < rect.left() + rect.width() / 4) {
                        insertDock = -1;
                        rect.setRight(rect.left() + rect.width() / 2);
                    }
                    else if (pos.x() > rect.right() - rect.width() / 4) {
                        insertDock = 1;
                        int width = rect.width();
                        rect.setLeft(rect.right() - width / 4);
                        rect.setWidth(width / 2);
                    }
                    break;
            }
        }

        if (!drop) {
            if (!OverlayTabWidget::_DragFrame) {
                OverlayTabWidget::_DragFrame = new OverlayDragFrame(getMainWindow());
            }

            rect = QRect(getMainWindow()->mapFromGlobal(rect.topLeft()), rect.size());
            OverlayTabWidget::_DragFrame->setGeometry(rect);
            if (!outside && !OverlayTabWidget::_DragFrame->isVisible()) {
                OverlayTabWidget::_DragFrame->raise();
                OverlayTabWidget::_DragFrame->show();
            }
            return;
        }

        if (src && src == dst && dstIndex != -2) {
            auto splitter = src->getSplitter();
            if (dstIndex == -1) {
                src->tabBar()->moveTab(srcIndex, 0);
                src->setCurrentIndex(0);
                src->onCurrentChanged(0);
            }
            else if (srcIndex != dstIndex) {
                auto sizes = splitter->sizes();
                src->tabBar()->moveTab(srcIndex, dstIndex);
                splitter->setSizes(sizes);
                src->onSplitterResize(dstIndex);
                src->saveTabs();
            }
            return;
        }

        if (src) {
            _overlayMap.erase(dock);
            src->removeWidget(dock);
        }

        setFocusView();
        if (!dst) {
            if (dstDock) {
                dock->setFloating(false);
                if (insertDock == 0) {
                    getMainWindow()->tabifyDockWidget(dstDock, dock);
                }
                else {
                    std::map<int, QDockWidget*> docks;
                    for (auto dockWidget : getMainWindow()->findChildren<QDockWidget*>()) {
                        if (dockWidget == dock || !dockWidget->isVisible()
                            || getMainWindow()->dockWidgetArea(dockWidget) != dstDockArea) {
                            continue;
                        }
                        auto pos = dockWidget->mapToGlobal(QPoint(0, 0));
                        if (dstDockArea == Qt::LeftDockWidgetArea
                            || dstDockArea == Qt::RightDockWidgetArea) {
                            docks[pos.y()] = dockWidget;
                        }
                        else {
                            docks[pos.x()] = dockWidget;
                        }
                    }
                    auto it = docks.begin();
                    for (; it != docks.end(); ++it) {
                        if (it->second == dstDock) {
                            break;
                        }
                    }
                    if (insertDock > 0 && it != docks.end()) {
                        ++it;
                    }
                    for (auto iter = it; iter != docks.end(); ++iter) {
                        getMainWindow()->removeDockWidget(iter->second);
                    }
                    getMainWindow()->addDockWidget(dstDockArea, dock);
                    dock->show();
                    for (auto iter = it; iter != docks.end(); ++iter) {
                        getMainWindow()->addDockWidget(dstDockArea, iter->second);
                        iter->second->show();
                    }
                }
            }
            else {
                dock->setFloating(true);
                if (OverlayTabWidget::_DragFrame) {
                    dock->setGeometry(QRect(
                        OverlayTabWidget::_DragFrame->mapToGlobal(QPoint()),
                        OverlayTabWidget::_DragFrame->size()
                    ));
                }
            }
            dock->show();
        }
        else if (dstIndex == -2) {
            getMainWindow()->addDockWidget(dst->getDockArea(), dock);
            dock->setFloating(false);
        }
        else {
            auto sizes = dst->getSplitter()->sizes();
            for (auto o : _overlayInfos) {
                if (o->tabWidget == dst) {
                    o->addWidget(dock, false);
                    break;
                }
            }
            index = dst->dockWidgetIndex(dock);
            if (index >= 0) {
                if (dstIndex < 0) {
                    dst->tabBar()->moveTab(index, 0);
                    dst->setCurrentIndex(0);
                    dst->onCurrentChanged(0);
                }
                else {
                    dst->tabBar()->moveTab(index, dstIndex);
                    int size = sizes[dstIndex + resizeOffset];
                    if (size) {
                        size /= 2;
                        sizes[dstIndex + resizeOffset] = size;
                    }
                    else {
                        size = 50;
                    }
                    sizes.insert(dstIndex, size);
                    dst->setCurrentIndex(dstIndex);
                    dst->getSplitter()->setSizes(sizes);
                    dst->onSplitterResize(dstIndex);
                    dst->saveTabs();
                }
                dst->setRevealTime(
                    QTime::currentTime().addMSecs(OverlayParams::getDockOverlayRevealDelay())
                );
            }
        }

        refresh();
    }

    void raiseAll()
    {
        if (raising) {
            return;
        }
        Base::StateLocker guard(raising);
        for (OverlayTabWidget* tabWidget : _Overlays) {
            if (tabWidget->isVisible()) {
                tabWidget->raise();
            }
        }
    }

    void registerDockWidget(const QString& name, OverlayTabWidget* widget)
    {
        if (name.size()) {
            _dockWidgetNameMap[name] = widget;
        }
    }

    void unregisterDockWidget(const QString& name, OverlayTabWidget* widget)
    {
        auto it = _dockWidgetNameMap.find(name);
        if (it != _dockWidgetNameMap.end() && it->second == widget) {
            _dockWidgetNameMap.erase(it);
        }
    }

    void applyCompactRailTabOwnership()
    {
        for (auto* overlayInfo : _overlayInfos) {
            overlayInfo->tabWidget->setCompactRailTabOwnershipEnabled(compactRailTabOwnershipEnabled);
        }
    }

    void reload(OverlayManager::ReloadMode mode)
    {
        if (mode == OverlayManager::ReloadMode::ReloadResume) {
            curReloadMode = mode = OverlayManager::ReloadMode::ReloadPending;
        }
        if (mode == OverlayManager::ReloadMode::ReloadPending) {
            if (curReloadMode != OverlayManager::ReloadMode::ReloadPause) {
                FC_LOG("reload pending");
                _reloadTimer.start(100);
            }
        }
        curReloadMode = mode;
        if (mode == OverlayManager::ReloadMode::ReloadPause) {
            FC_LOG("reload paused");
            _reloadTimer.stop();
        }
    }
};


static OverlayManager* _instance;

OverlayManager* OverlayManager::instance()
{
    if (_instance == 0) {
        _instance = new OverlayManager;
    }
    return _instance;
}

void OverlayManager::destruct()
{
    delete _instance;
    _instance = 0;
}

OverlayManager::OverlayManager()
{
    d = new Private(this, getMainWindow());
    qApp->installEventFilter(this);
}

OverlayManager::~OverlayManager()
{
    delete d;
}

void OverlayManager::setOverlayMode(OverlayMode mode)
{
    d->setOverlayMode(mode);
}


void OverlayManager::initDockWidget(QDockWidget* dw)
{
    QObject::connect(dw->toggleViewAction(), &QAction::triggered, this, &OverlayManager::onToggleDockWidget);
    QObject::connect(dw, &QDockWidget::visibilityChanged, this, &OverlayManager::onDockVisibleChange);
    QObject::connect(dw, &QDockWidget::featuresChanged, this, &OverlayManager::onDockFeaturesChange);
    if (auto widget = dw->widget()) {
        QObject::connect(
            widget,
            &QWidget::windowTitleChanged,
            this,
            &OverlayManager::onDockWidgetTitleChange
        );
    }

    QString name = dw->objectName();
    if (name.size()) {
        auto it = d->_dockWidgetNameMap.find(dw->objectName());
        if (it != d->_dockWidgetNameMap.end()) {
            for (auto o : d->_overlayInfos) {
                if (o->tabWidget == it->second) {
                    o->addWidget(dw, true);
                    d->onToggleDockWidget(dw, 3);
                    break;
                }
            }
            d->refresh();
        }
    }
}

void OverlayManager::setupDockWidget(QDockWidget* dw, int dockArea)
{
    (void)dockArea;
    d->setupTitleBar(dw);
}

void OverlayManager::moveDockWidgetToOverlay(QDockWidget* dw, Qt::DockWidgetArea dockArea)
{
    if (!dw) {
        return;
    }

    d->toggleOverlay(dw, ToggleMode::Unset);
    d->toggleOverlay(dw, ToggleMode::Set, dockArea);
    d->setupTitleBar(dw);
}

bool OverlayManager::moveDockWidgetToOverlayOnly(QDockWidget* dw, Qt::DockWidgetArea dockArea)
{
    int resolvedDockArea = dockArea;
    if (!d->canResolveOverlayInfo(dw, resolvedDockArea)) {
        return false;
    }
    const Qt::DockWidgetArea previousArea = d->dockWidgetOverlayArea(dw);
    if (previousArea == resolvedDockArea) {
        d->setupTitleBar(dw);
        return true;
    }

    d->toggleOverlay(dw, ToggleMode::Unset);
    if (!d->toggleOverlay(dw, ToggleMode::Set, resolvedDockArea, false)) {
        if (previousArea != Qt::NoDockWidgetArea) {
            d->toggleOverlay(dw, ToggleMode::Set, previousArea, false);
        }
        return false;
    }

    d->setupTitleBar(dw);
    return true;
}

bool OverlayManager::moveDockWidgetInOverlay(QDockWidget* dock, int order)
{
    auto it = d->_overlayMap.find(dock);
    return it != d->_overlayMap.end() && it->second
        && it->second->tabWidget->moveDockWidgetToIndex(dock, order);
}

Qt::DockWidgetArea OverlayManager::dockWidgetOverlayArea(QDockWidget* dw) const
{
    return d->dockWidgetOverlayArea(dw);
}

bool OverlayManager::setDockWidgetOverlayExtent(QDockWidget* dock, int extent)
{
    auto it = d->_overlayMap.find(dock);
    if (it == d->_overlayMap.end() || !it->second || extent <= 0) {
        return false;
    }

    OverlayTabWidget* tabWidget = it->second->tabWidget;
    QRect rect = tabWidget->getRect();
    switch (it->second->dockArea) {
        case Qt::LeftDockWidgetArea:
            rect.setWidth(extent);
            break;
        case Qt::RightDockWidgetArea:
            rect.setLeft(rect.right() - extent + 1);
            break;
        case Qt::TopDockWidgetArea:
            rect.setHeight(extent);
            break;
        case Qt::BottomDockWidgetArea:
            rect.setTop(rect.bottom() - extent + 1);
            break;
        default:
            return false;
    }
    tabWidget->setRect(rect);
    d->refresh();
    return true;
}

int OverlayManager::dockWidgetOverlayExtent(QDockWidget* dock) const
{
    auto it = d->_overlayMap.find(dock);
    if (it == d->_overlayMap.end() || !it->second) {
        return 0;
    }
    const QRect rect = it->second->tabWidget->getRect();
    return it->second->dockArea == Qt::LeftDockWidgetArea
            || it->second->dockArea == Qt::RightDockWidgetArea
        ? rect.width()
        : rect.height();
}

bool OverlayManager::usesCanonicalHeaderOwnership() const
{
    return d->usesCanonicalHeaderOwnership();
}

void OverlayManager::setDockHeaderOwnedByOverlay(QDockWidget* dock, bool ownedByOverlay)
{
    if (!dock || !d->usesCanonicalHeaderOwnership()) {
        return;
    }

    Private::setHeaderOwnerProperty(dock, ownedByOverlay);
    d->setupTitleBar(dock);
}

bool OverlayManager::isDockHeaderOwnedByOverlay(QDockWidget* dock) const
{
    return d->usesCanonicalHeaderOwnership() && Private::isHeaderOwnedByOverlay(dock);
}

void OverlayManager::unsetupDockWidget(QDockWidget* dw)
{
    d->toggleOverlay(dw, ToggleMode::Unset);
}

void OverlayManager::onToggleDockWidget(bool checked)
{
    auto action = qobject_cast<QAction*>(sender());
    if (!action) {
        return;
    }
    d->onToggleDockWidget(qobject_cast<QDockWidget*>(action->parent()), checked);
}

void OverlayManager::onDockVisibleChange(bool visible)
{
    auto dock = qobject_cast<QDockWidget*>(sender());
    if (!dock) {
        return;
    }
    FC_TRACE(
        "dock " << dock->objectName().toUtf8().constData() << " visible change " << visible << ", "
                << dock->isVisible()
    );
}

void OverlayManager::onDockFeaturesChange(QDockWidget::DockWidgetFeatures features)
{
    Q_UNUSED(features);

    auto dw = qobject_cast<QDockWidget*>(sender());

    if (!dw) {
        return;
    }

    // Rebuild the title widget as it may have a different set of buttons shown.
    if (auto tw = dw->titleBarWidget(); !tw || qobject_cast<OverlayTitleBar*>(tw)) {
        setupTitleBar(dw);
    }
    auto it = d->_overlayMap.find(dw);
    if (it != d->_overlayMap.end() && it->second) {
        d->refreshOverlayTitleBar(it->second->tabWidget);
    }
}

void OverlayManager::onTaskViewUpdate()
{
    auto taskview = qobject_cast<TaskView::TaskView*>(sender());
    if (!taskview) {
        return;
    }
    QDockWidget* dock = nullptr;
    for (QWidget* w = taskview; w; w = w->parentWidget()) {
        if ((dock = qobject_cast<QDockWidget*>(w))) {
            break;
        }
    }
    if (dock) {
        auto it = d->_overlayMap.find(dock);
        if (it == d->_overlayMap.end() || it->second->tabWidget->count() < 2
            || it->second->tabWidget->getAutoMode() != OverlayTabWidget::AutoMode::TaskShow) {
            return;
        }
        d->onToggleDockWidget(dock, taskview->isEmpty(false) ? -2 : 2);
    }
}

void OverlayManager::onDockWidgetTitleChange(const QString& title)
{
    if (title.isEmpty()) {
        return;
    }
    auto widget = qobject_cast<QWidget*>(sender());
    QDockWidget* dock = nullptr;
    for (QWidget* w = widget; w; w = w->parentWidget()) {
        if ((dock = qobject_cast<QDockWidget*>(w))) {
            break;
        }
    }
    if (!dock) {
        return;
    }
    auto tabWidget = findTabWidget(dock);
    if (!tabWidget) {
        return;
    }
    int index = tabWidget->dockWidgetIndex(dock);
    if (index >= 0) {
        tabWidget->setTabText(index, title);
    }
    if (tabWidget->getTitleBar()) {
        tabWidget->getTitleBar()->update();
    }
}

void OverlayManager::retranslate()
{
    d->retranslate();
}

void OverlayManager::refreshIcons()
{
    d->refreshIcons();
}

void OverlayManager::reload(ReloadMode mode)
{
    d->reload(mode);
}

void OverlayManager::raiseAll()
{
    d->raiseAll();
}

static inline bool isNear(const QPoint& a, const QPoint& b, int tol = 16)
{
    QPoint d = a - b;
    return d.x() * d.x() + d.y() * d.y() < tol;
}

bool OverlayManager::eventFilter(QObject* o, QEvent* ev)
{
    if (d->intercepting || !getMainWindow() || !o->isWidgetType()) {
        return false;
    }
    auto mdi = getMainWindow()->getMdiArea();
    if (!mdi) {
        return false;
    }

    switch (ev->type()) {
        case QEvent::Enter:
            if (Selection().hasPreselection() && !qobject_cast<View3DInventorViewer*>(o)
                && !isUnderOverlay()) {
                Selection().rmvPreselect();
            }
            break;
        case QEvent::Leave:
            // Clear mirrored cursor when leaving overlay.
            if (auto tabWidget = qobject_cast<OverlayTabWidget*>(o)) {
                tabWidget->unsetCursor();
            }
            break;
        case QEvent::ZOrderChange: {
            if (!d->raising && getMainWindow() && o == mdi) {
                // On Windows, for some reason, it will raise mdi window on tab
                // change in any docked widget, which will then obscure any overlay
                // docked widget here.
                for (auto child : getMainWindow()->children()) {
                    if (child == mdi || qobject_cast<QDockWidget*>(child)) {
                        QMetaObject::invokeMethod(this, "raiseAll", Qt::QueuedConnection);
                        break;
                    }
                    if (qobject_cast<OverlayTabWidget*>(child)) {
                        break;
                    }
                }
            }
            break;
        }
        case QEvent::Resize: {
            if (getMainWindow() && o == mdi) {
                refresh();
            }
            return false;
        }
        case QEvent::KeyPress: {
            QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
            bool accepted = false;
            if (ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_Escape) {
                if (d->mouseTransparent) {
                    d->setMouseTransparent(false);
                    accepted = true;
                }
                else if (OverlayTabWidget::_Dragging && OverlayTabWidget::_Dragging != o) {
                    if (auto titleBar = qobject_cast<OverlayTitleBar*>(OverlayTabWidget::_Dragging)) {
                        titleBar->endDrag();
                    }
                    else if (
                        auto splitHandle
                        = qobject_cast<OverlaySplitterHandle*>(OverlayTabWidget::_Dragging)
                    ) {
                        splitHandle->endDrag();
                    }
                }
                else if (!OverlayTabWidget::_Dragging) {
                    for (OverlayTabWidget* tabWidget : _Overlays) {
                        if (tabWidget->onEscape()) {
                            accepted = true;
                        }
                    }
                }
            }
            if (accepted) {
                ke->accept();
                return true;
            }
            break;
        }
        case QEvent::Paint:
            if (auto widget = qobject_cast<QWidget*>(o)) {
                // QAbstractItemView optimize redraw using its item delegate's
                // visualRect(). However, if we are using QGraphicsEffects, the
                // effect may touch areas outside of visualRect(), so
                // OverlayTabWidget offers a timer for a delayed redraw.
                widget = qobject_cast<QAbstractItemView*>(widget->parentWidget());
                if (widget) {
                    auto tabWidget = findTabWidget(widget, true);
                    if (tabWidget) {
                        tabWidget->scheduleRepaint();
                    }
                }
            }
            break;
        // case QEvent::NativeGesture:
        case QEvent::ContextMenu: {
            auto cev = static_cast<QContextMenuEvent*>(ev);
            if (cev->reason() != QContextMenuEvent::Mouse) {
                return false;
            }
        }  // fall through
        case QEvent::Wheel:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove: {
            if (ev->type() == QEvent::Wheel && !OverlayParams::getDockOverlayWheelPassThrough()) {
                return false;
            }
            if (OverlayTabWidget::_Dragging && OverlayTabWidget::_Dragging != o) {
                if (auto titleBar = qobject_cast<OverlayTitleBar*>(OverlayTabWidget::_Dragging)) {
                    titleBar->endDrag();
                }
                else if (
                    auto splitHandle = qobject_cast<OverlaySplitterHandle*>(OverlayTabWidget::_Dragging)
                ) {
                    splitHandle->endDrag();
                }
            }
            QWidget* grabber = QWidget::mouseGrabber();
            d->lastIntercept = nullptr;
            if (d->mouseTransparent || (grabber && grabber != d->_trackingOverlay)) {
                return false;
            }
            if (qobject_cast<QAbstractButton*>(o)) {
                return false;
            }
            if (ev->type() != QEvent::Wheel) {
                if (qobject_cast<OverlayTitleBar*>(o)) {
                    return false;
                }
            }
            else if (qobject_cast<QScrollBar*>(o)) {
                return false;
            }

            if (d->_trackingWidget) {
                if (!isTreeViewDragging()) {
                    d->interceptEvent(d->_trackingWidget, ev);
                }
                if (isTreeViewDragging()
                    || (ev->type() == QEvent::MouseButtonRelease
                        && QApplication::mouseButtons() == Qt::NoButton)) {
                    d->_trackingWidget = nullptr;
                    if (d->_trackingOverlay == grabber && ev->type() == QEvent::MouseButtonRelease) {
                        d->_trackingOverlay = nullptr;
                        // Must not release mouse here, because otherwise the event
                        // will find its way to the actual widget under cursor.
                        // Instead, return false here to let OverlayTabWidget::event()
                        // release the mouse.
                        return false;
                    }
                    if (d->_trackingOverlay && grabber == d->_trackingOverlay) {
                        d->_trackingOverlay->releaseMouse();
                    }
                    d->_trackingOverlay = nullptr;
                }
                // Must return true here to filter the event, otherwise ContextMenu
                // event may be routed to the actual widget. Other types of event
                // probably do not matter.
                return true;
            }
            else if (
                ev->type() != QEvent::MouseButtonPress && ev->type() != QEvent::MouseButtonDblClick
                && QApplication::mouseButtons() != Qt::NoButton
            ) {
                return false;
            }

            if (isTreeViewDragging()) {
                return false;
            }

            OverlayTabWidget* activeTabWidget = nullptr;
            int hit = 0;
            QPoint pos = QCursor::pos();
            if (OverlayParams::getDockOverlayAutoMouseThrough() && ev->type() != QEvent::Wheel
                && pos == d->_lastPos) {
                hit = 1;
            }
            else if (
                ev->type() == QEvent::Wheel && !d->wheelDelay.isNull()
                && (isNear(pos, d->wheelPos) || d->wheelDelay > QTime::currentTime())
            ) {
                d->wheelDelay = QTime::currentTime().addMSecs(
                    OverlayParams::getDockOverlayWheelDelay()
                );
                d->wheelPos = pos;
                return false;
            }
            else {
                for (auto widget = qApp->widgetAt(pos); widget; widget = widget->parentWidget()) {
                    int type = widget->windowType();
                    if (type != Qt::Widget && type != Qt::Window) {
                        if (type != Qt::SubWindow) {
                            hit = -1;
                        }
                        break;
                    }
                    if (ev->type() == QEvent::Wheel) {
                        if (qobject_cast<OverlayTitleBar*>(widget)) {
                            activeTabWidget = qobject_cast<OverlayTabWidget*>(widget->parentWidget());
                        }
                        else if (qobject_cast<OverlaySplitterHandle*>(widget)) {
                            auto parent = widget->parentWidget();
                            if (parent) {
                                activeTabWidget = qobject_cast<OverlayTabWidget*>(
                                    parent->parentWidget()
                                );
                            }
                        }
                        if (activeTabWidget) {
                            break;
                        }
                    }
                    if (auto tabWidget = qobject_cast<OverlayTabWidget*>(widget)) {
                        if (tabWidget->testAlpha(pos, ev->type() == QEvent::Wheel ? 4 : 1) == 0) {
                            activeTabWidget = tabWidget;
                        }
                        break;
                    }
                }
                if (activeTabWidget) {
                    hit = OverlayParams::getDockOverlayAutoMouseThrough();
                    d->_lastPos = pos;
                }
            }

            for (OverlayTabWidget* tabWidget : _Overlays) {
                if (tabWidget->getProxyWidget()->hitTest(pos)
                    == OverlayProxyWidget::HitTest::HitInner) {
                    if ((ev->type() == QEvent::MouseButtonRelease
                         || ev->type() == QEvent::MouseButtonPress)
                        && static_cast<QMouseEvent*>(ev)->button() == Qt::LeftButton) {
                        if (ev->type() == QEvent::MouseButtonRelease) {
                            tabWidget->getProxyWidget()->onMousePress();
                        }
                        return true;
                    }
                }
            }

            if (hit <= 0) {
                d->_lastPos.setX(std::numeric_limits<int>::max());
                if (ev->type() == QEvent::Wheel) {
                    d->wheelDelay = QTime::currentTime().addMSecs(
                        OverlayParams::getDockOverlayWheelDelay()
                    );
                    d->wheelPos = pos;
                }
                return false;
            }

            auto hitWidget = mdi->childAt(mdi->mapFromGlobal(pos));
            if (!hitWidget) {
                return false;
            }

            if (!activeTabWidget) {
                activeTabWidget = findTabWidget(qApp->widgetAt(QCursor::pos()));
            }
            if (!activeTabWidget || !activeTabWidget->isTransparent()) {
                return false;
            }

            ev->accept();
            d->interceptEvent(hitWidget, ev);
            // Mirror underlying widget cursor onto transparent overlay.
            if (ev->type() == QEvent::MouseMove) {
                activeTabWidget->setCursor(hitWidget->cursor());
            }
            if (ev->isAccepted() && ev->type() == QEvent::MouseButtonPress) {
                hitWidget->setFocus();
                d->_trackingWidget = hitWidget;
                d->_trackingOverlay = activeTabWidget;
                // Wayland doesn't allow mouse grab
                if (QGuiApplication::platformName() != QLatin1String("wayland")) {
                    d->_trackingOverlay->grabMouse();
                }
            }
            return true;
        }
        default:
            break;
    }
    return false;
}

namespace
{
class MouseGrabberGuard
{
public:
    explicit MouseGrabberGuard(QWidget* grabber)
    {
        if (grabber && grabber == QWidget::mouseGrabber()) {
            _grabber = grabber;
            _grabber->releaseMouse();
        }
    }
    ~MouseGrabberGuard()
    {
        if (_grabber) {
            // Wayland doesn't allow mouse grab
            if (QGuiApplication::platformName() != QLatin1String("wayland")) {
                _grabber->grabMouse();
            }
        }
    }

    QPointer<QWidget> _grabber;
};
}  // anonymous namespace

void OverlayManager::Private::interceptEvent(QWidget* widget, QEvent* ev)
{
    Base::StateLocker guard(this->intercepting);
    MouseGrabberGuard grabberGuard(_trackingOverlay);

    lastIntercept = nullptr;
    auto getChildAt = [](QWidget* w, const QPoint& pos) {
        QWidget* res = w;
        for (; w; w = w->childAt(w->mapFromGlobal(pos))) {
            if (auto scrollArea = qobject_cast<QAbstractScrollArea*>(w)) {
                return scrollArea->viewport();
            }
            res = w;
        }
        return res;
    };

    switch (ev->type()) {
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonPress:
        case QEvent::MouseMove:
        case QEvent::MouseButtonDblClick: {
            auto me = static_cast<QMouseEvent*>(ev);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QPointF screenPos = me->screenPos();
            QPoint point = me->globalPos();
#else
            QPointF screenPos = me->globalPosition();
            QPoint point = screenPos.toPoint();
#endif
            lastIntercept = getChildAt(widget, point);
            QMouseEvent mouseEvent(
                ev->type(),
                lastIntercept->mapFromGlobal(point),
                screenPos,
                me->button(),
                me->buttons(),
                me->modifiers()
            );
            QApplication::sendEvent(lastIntercept, &mouseEvent);
            break;
        }
        case QEvent::Wheel: {
            auto we = static_cast<QWheelEvent*>(ev);
            QPoint globalPos = we->globalPosition().toPoint();
            lastIntercept = getChildAt(widget, globalPos);

            // For some reason in case of 3D View we have to target it directly instead of targeting
            // the viewport of QAbstractScrollArea like it works for all other widgets of that kind.
            // That's why for this event we have to traverse up the widget tree to find if it is
            // part of the 3D view.
            for (auto parent = lastIntercept; parent; parent = parent->parentWidget()) {
                if (qobject_cast<View3DInventorViewer*>(parent)) {
                    lastIntercept = parent;
                    break;
                }
            }

            QWheelEvent wheelEvent(
                lastIntercept->mapFromGlobal(globalPos),
                globalPos,
                we->pixelDelta(),
                we->angleDelta(),
                we->buttons(),
                we->modifiers(),
                we->phase(),
                we->inverted(),
                we->source()
            );
            QApplication::sendEvent(lastIntercept, &wheelEvent);
            break;
        }
        case QEvent::ContextMenu: {
            auto ce = static_cast<QContextMenuEvent*>(ev);
            lastIntercept = getChildAt(widget, ce->globalPos());
            QContextMenuEvent contextMenuEvent(
                ce->reason(),
                lastIntercept->mapFromGlobal(ce->globalPos()),
                ce->globalPos()
            );
            QApplication::sendEvent(lastIntercept, &contextMenuEvent);
            break;
        }
        default:
            break;
    }
}

void OverlayManager::refresh(QWidget* widget, bool refreshStyle)
{
    d->refresh(widget, refreshStyle);
}

void OverlayManager::setMouseTransparent(bool enabled)
{
    d->setMouseTransparent(enabled);
}

bool OverlayManager::isMouseTransparent() const
{
    return d->mouseTransparent;
}

bool OverlayManager::isUnderOverlay() const
{
    return OverlayParams::getDockOverlayAutoMouseThrough()
        && findTabWidget(qApp->widgetAt(QCursor::pos()), true);
}

void OverlayManager::save()
{
    d->save();
}

void OverlayManager::restore()
{
    d->restore();

    if (Control().taskPanel()) {
        connect(
            Control().taskPanel(),
            &TaskView::TaskView::taskUpdate,
            this,
            &OverlayManager::onTaskViewUpdate
        );
    }
}

void OverlayManager::setupTitleBar(QDockWidget* dock)
{
    d->setupTitleBar(dock);
}

void OverlayManager::onFocusChanged(QWidget* old, QWidget* now)
{
    d->onFocusChanged(old, now);
}

void OverlayManager::onAction()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        d->onAction(action);
    }
}

void OverlayManager::dragDockWidget(
    const QPoint& pos,
    QWidget* src,
    const QPoint& offset,
    const QSize& size,
    bool drop
)
{
    d->dragDockWidget(pos, src, offset, size, drop);
}

void OverlayManager::floatDockWidget(QDockWidget* dock)
{
    d->floatDockWidget(dock);
}

void OverlayManager::registerDockWidget(const QString& name, OverlayTabWidget* widget)
{
    d->registerDockWidget(name, widget);
}

void OverlayManager::unregisterDockWidget(const QString& name, OverlayTabWidget* widget)
{
    d->unregisterDockWidget(name, widget);
}

QWidget* OverlayManager::getLastMouseInterceptWidget() const
{
    return d->lastIntercept;
}

const QString& OverlayManager::getStyleSheet() const
{
    return OverlayStyleSheet::instance()->activeStyleSheet;
}

bool OverlayManager::getHideTab() const
{
    return OverlayStyleSheet::instance()->hideTab;
}

void OverlayManager::setCompactRailTabOwnershipEnabled(bool enabled)
{
    _compactRailTabOwnershipEnabled = enabled;
    d->compactRailTabOwnershipEnabled = enabled;
    d->applyCompactRailTabOwnership();
}

bool OverlayManager::isCompactRailTabOwnershipEnabled() const
{
    return _compactRailTabOwnershipEnabled;
}

bool OverlayManager::dockWidgetOverlayVisible(QDockWidget* dock) const
{
    if (dock && d->usesCanonicalHeaderOwnership()) {
        if (QAction* toggleAction = dock->toggleViewAction();
            toggleAction && toggleAction->isCheckable()) {
            // Compact placement defines QAction as the visibility source of truth so the View
            // menu, rail, and placement policy cannot disagree about an overlay's open state.
            return toggleAction->isChecked();
        }
    }

    auto it = d->_overlayMap.find(dock);
    if (it == d->_overlayMap.end() || !it->second) {
        return false;
    }

    OverlayTabWidget* tabWidget = it->second->tabWidget;
    const int index = tabWidget->dockWidgetIndex(dock);
    const QList<int> sizes = tabWidget->getSplitter()->sizes();
    if (index < 0 || index >= sizes.size() || sizes.at(index) <= 0) {
        return false;
    }

    // Logical openness is host state, not whether an ancestor/window is currently mapped. The
    // latter is false during startup, workbench changes, and headless tests even for an expanded
    // overlay, and previously made a collapsed overlay impossible to toggle back on reliably.
    return tabWidget->getState() != OverlayTabWidget::State::Hidden
        && tabWidget->getState() != OverlayTabWidget::State::HintHidden;
}

bool OverlayManager::compactRailTabOwnershipEnabled()
{
    return _compactRailTabOwnershipEnabled;
}

bool OverlayManager::setDockWidgetOverlayAutoHide(QDockWidget* dock, bool enabled)
{
    auto it = d->_overlayMap.find(dock);
    if (!d->usesCanonicalHeaderOwnership() || it == d->_overlayMap.end() || !it->second) {
        return false;
    }

    OverlayTabWidget* tabWidget = it->second->tabWidget;
    const ParameterGrp::handle persistedHandle = tabWidget->hGrp;
    tabWidget->hGrp = nullptr;
    tabWidget->setAutoMode(
        enabled ? OverlayTabWidget::AutoMode::AutoHide : OverlayTabWidget::AutoMode::NoAutoMode
    );
    tabWidget->hGrp = persistedHandle;
    return true;
}

bool OverlayManager::dockWidgetOverlayAutoHide(QDockWidget* dock) const
{
    auto it = d->_overlayMap.find(dock);
    if (!d->usesCanonicalHeaderOwnership() || it == d->_overlayMap.end() || !it->second) {
        return false;
    }
    return it->second->tabWidget->getAutoMode() == OverlayTabWidget::AutoMode::AutoHide;
}

void OverlayManager::refreshOverlayTitleBar(OverlayTabWidget* tabWidget)
{
    d->refreshOverlayTitleBar(tabWidget);
}

void OverlayManager::setFocusView()
{
    auto view = getMainWindow()->activeWindow();
    if (!view) {
        view = Application::Instance->activeView();
    }
    if (view) {
        view->setFocus();
    }
}

#include "moc_OverlayManager.cpp"
