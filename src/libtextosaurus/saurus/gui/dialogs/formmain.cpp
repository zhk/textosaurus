// This file is distributed under GNU GPLv3 license. For full license text, see <project-git-repository-root-folder>/LICENSE.md.

#include "saurus/gui/dialogs/formmain.h"

#include "common/gui/messagebox.h"
#include "common/gui/plaintoolbutton.h"
#include "common/gui/systemtrayicon.h"
#include "common/gui/toolbar.h"
#include "common/miscellaneous/iconfactory.h"
#include "common/miscellaneous/settings.h"
#include "common/miscellaneous/systemfactory.h"
#include "common/network-web/webfactory.h"
#include "definitions/definitions.h"
#include "saurus/gui/dialogs/formabout.h"
#include "saurus/gui/dialogs/formfindreplace.h"
#include "saurus/gui/dialogs/formsettings.h"
#include "saurus/gui/dialogs/formupdate.h"
#include "saurus/gui/statusbar.h"
#include "saurus/gui/tabbar.h"
#include "saurus/gui/tabwidget.h"
#include "saurus/miscellaneous/application.h"
#include "saurus/miscellaneous/textapplication.h"

#include <QCloseEvent>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMetaObject>
#include <QRect>
#include <QScopedPointer>
#include <QThread>
#include <QTimer>

FormMain::FormMain(QWidget* parent) : QMainWindow(parent), m_statusBar() {
  m_ui.setupUi(this);
  qApp->setMainForm(this);

  setWindowTitle(APP_LONG_NAME);
  setCentralWidget(m_tabEditors = new TabWidget(this));
  setStatusBar(m_statusBar = new StatusBar(this));
  addToolBar(m_toolBar = new ToolBar(tr("Main Toolbar"), this));

  m_statusBar->setObjectName(QSL("m_statusBar"));
  m_tabEditors->setObjectName(QSL("m_tabEditors"));
  m_toolBar->setObjectName(QSL("m_toolBar"));

  prepareMenus();
  createConnections();
  setupIcons();

  setStyleSheet(QSL("QStatusBar::item { border: none; } "
                    "QSplitter::handle:horizontal, QSplitter::handle:vertical { width: 1px; }"));

  qDebug().nospace().noquote() << QSL("Creating main application form in thread: \'") << QThread::currentThreadId() << "\'.";
}

TabWidget* FormMain::tabWidget() const {
  return m_tabEditors;
}

ToolBar* FormMain::toolBar() const {
  return m_toolBar;
}

StatusBar* FormMain::statusBar() const {
  return m_statusBar;
}

QList<QAction*> FormMain::allActions() const {
  QList<QAction*> actions;

  // Add basic actions.
  actions << m_ui.m_actionSettings;
  actions << m_ui.m_actionRestart;
  actions << m_ui.m_actionQuit;
  actions << m_ui.m_actionFileNew;
  actions << m_ui.m_actionFileOpen;
  actions << m_ui.m_actionFileSave;
  actions << m_ui.m_actionFileSaveAll;
  actions << m_ui.m_actionFileSaveAs;
  actions << m_ui.m_actionFileReload;
  actions << m_ui.m_actionFileEncryption;

#if !defined(Q_OS_MAC)
  actions << m_ui.m_actionFullscreen;
#endif

  actions << m_ui.m_actionEditBack;
  actions << m_ui.m_actionEditForward;
  actions << m_ui.m_actionEolConvertMac;
  actions << m_ui.m_actionEolConvertUnix;
  actions << m_ui.m_actionEolConvertWindows;
  actions << m_ui.m_actionEolMac;
  actions << m_ui.m_actionEolUnix;
  actions << m_ui.m_actionEolWindows;
  actions << m_ui.m_actionFindReplace;
  actions << m_ui.m_actionCodeFolding;
  actions << m_ui.m_actionLineNumbers;
  actions << m_ui.m_actionWordWrap;
  actions << m_ui.m_actionStayOnTop;
  actions << m_ui.m_actionViewEols;
  actions << m_ui.m_actionViewWhitespaces;
  actions << m_ui.m_actionContextAwareHighlighting;
  actions << m_ui.m_actionAutoIndentEnabled;

  actions << m_ui.m_actionAboutGuard;
  actions << m_ui.m_actionSwitchMainWindow;
  actions << m_ui.m_actionSwitchStatusBar;
  actions << m_ui.m_actionTabsNext;
  actions << m_ui.m_actionTabsPrevious;
  actions << m_ui.m_actionTabsCloseAll;
  actions << m_ui.m_actionTabsCloseCurrent;
  actions << m_ui.m_actionTabsCloseAllExceptCurrent;
  actions << m_ui.m_actionTabsCloseAllUnmodified;

  return actions;
}

void FormMain::prepareMenus() {
#if defined(Q_OS_MAC)
  m_ui.m_actionFullscreen->setVisible(false);
#endif
}

void FormMain::switchFullscreenMode() {
  if (!isFullScreen()) {
    qApp->settings()->setValue(GROUP(GUI), GUI::IsMainWindowMaximizedBeforeFullscreen, isMaximized());
    showFullScreen();

    toolBar()->hide();
    statusBar()->hide();
  }
  else {
    if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::IsMainWindowMaximizedBeforeFullscreen)).toBool()) {
      setWindowState((windowState() & ~Qt::WindowFullScreen) | Qt::WindowMaximized);
    }
    else {
      showNormal();
    }

    toolBar()->resetActiveState();
    statusBar()->resetActiveState();
  }
}

void FormMain::changeEvent(QEvent* event) {
  switch (event->type()) {
    case QEvent::WindowStateChange: {
      if ((windowState() & Qt::WindowMinimized) == Qt::WindowMinimized &&
          SystemTrayIcon::isSystemTrayActivated() &&
          qApp->settings()->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenMinimized)).toBool()) {
        event->ignore();
        QTimer::singleShot(CHANGE_EVENT_DELAY, this, [this]() {
          switchVisibility();
        });
      }

      break;
    }

    default:
      break;
  }

  QMainWindow::changeEvent(event);
}

void FormMain::dragEnterEvent(QDragEnterEvent* event) {
  event->accept();
}

void FormMain::closeEvent(QCloseEvent* event) {
  // 1) "Quit" is triggered or main window is closed via "X" in non-tray mode.
  // 2) Main window is closed via "X" in tray mode and user wants to quit app when this happens.
  bool quiting_or_non_tray = qApp->isQuitting() || qApp->quitOnLastWindowClosed();
  bool quit_on_close_tray = !qApp->settings()->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenClosed)).toBool();

  if (quiting_or_non_tray || quit_on_close_tray) { /* 2) */
    bool should_stop = true;
    emit closeRequested(&should_stop);

    if (should_stop) {
      if (quit_on_close_tray) {
        qApp->setQuitOnLastWindowClosed(true);
      }

      event->accept();
    }
    else {
      event->ignore();
    }
  }
}

void FormMain::switchVisibility(bool force_hide) {
  if (force_hide || isVisible()) {
    if (isMinimized()) {
      display();
    }
    else if (SystemTrayIcon::isSystemTrayActivated()) {
      hide();
    }
    else {
      // Window gets minimized in single-window mode.
      showMinimized();
    }
  }
  else {
    display();
  }
}

void FormMain::switchStayOnTop() {
  bool enable = (windowFlags() & Qt::WindowStaysOnTopHint) == 0;
  bool was_maximized = isMaximized();
  QRect geom;

  if (was_maximized) {
    showNormal();
    qApp->processEvents();

    // We store unmaximized geometry.
    geom = geometry();
  }

  setWindowFlag(Qt::WindowStaysOnTopHint, enable);
  show();

  if (was_maximized) {
    setGeometry(geom);
    qApp->processEvents();
    showMaximized();
  }

  m_ui.m_actionStayOnTop->setChecked(enable);
  qApp->settings()->setValue(GROUP(GUI), GUI::StayOnTop, enable);
}

void FormMain::display() {
  setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
  show();
  activateWindow();
  raise();

  Application::alert(this);
}

void FormMain::setupIcons() {
  IconFactory* icon_theme_factory = qApp->icons();

  // Setup icons of this main window.
  m_ui.m_actionSettings->setIcon(icon_theme_factory->fromTheme(QSL("document-properties")));
  m_ui.m_actionQuit->setIcon(icon_theme_factory->fromTheme(QSL("application-exit")));
  m_ui.m_actionRestart->setIcon(icon_theme_factory->fromTheme(QSL("view-refresh")));
  m_ui.m_actionAboutGuard->setIcon(icon_theme_factory->fromTheme(QSL("help-about")));
  m_ui.m_actionCheckForUpdates->setIcon(icon_theme_factory->fromTheme(QSL("system-upgrade")));
  m_ui.m_actionReportBug->setIcon(icon_theme_factory->fromTheme(QSL("call-start")));
  m_ui.m_actionDonate->setIcon(icon_theme_factory->fromTheme(QSL("applications-office")));
  m_ui.m_actionDisplayWiki->setIcon(icon_theme_factory->fromTheme(QSL("applications-science")));

  m_ui.m_actionFileNew->setIcon(icon_theme_factory->fromTheme(QSL("document-new")));
  m_ui.m_actionFileOpen->setIcon(icon_theme_factory->fromTheme(QSL("document-open")));
  m_ui.m_menuFileOpenWithEncoding->setIcon(icon_theme_factory->fromTheme(QSL("document-open")));
  m_ui.m_menuFileReopenWithEncoding->setIcon(icon_theme_factory->fromTheme(QSL("document-open")));
  m_ui.m_actionFileSave->setIcon(icon_theme_factory->fromTheme(QSL("document-save")));
  m_ui.m_actionFileSaveAs->setIcon(icon_theme_factory->fromTheme(QSL("document-save-as")));
  m_ui.m_actionFileSaveAll->setIcon(icon_theme_factory->fromTheme(QSL("document-save")));
  m_ui.m_menuFileSaveWithEncoding->setIcon(icon_theme_factory->fromTheme(QSL("document-save-as")));
  m_ui.m_actionFileEncryption->setIcon(icon_theme_factory->fromTheme(QSL("multipart-encrypted")));
  m_ui.m_actionPrint->setIcon(icon_theme_factory->fromTheme(QSL("gtk-print")));
  m_ui.m_actionPrintPreview->setIcon(icon_theme_factory->fromTheme(QSL("gtk-print-preview")));
  m_ui.m_actionPrintPreviewBlackWhite->setIcon(icon_theme_factory->fromTheme(QSL("gtk-print-preview")));

  // Edit.
  m_ui.m_actionFindReplace->setIcon(icon_theme_factory->fromTheme(QSL("edit-find")));
  m_ui.m_actionEditBack->setIcon(icon_theme_factory->fromTheme(QSL("edit-undo")));
  m_ui.m_actionEditForward->setIcon(icon_theme_factory->fromTheme(QSL("edit-redo")));

  // View.
  m_ui.m_actionSwitchMainWindow->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui.m_actionFullscreen->setIcon(icon_theme_factory->fromTheme(QSL("view-fullscreen")));
  m_ui.m_actionSwitchStatusBar->setIcon(icon_theme_factory->fromTheme(QSL("gtk-dialog-info")));
  m_ui.m_actionSwitchToolBar->setIcon(icon_theme_factory->fromTheme(QSL("configure-toolbars")));

  // Tabs & web browser.
  m_ui.m_actionTabsCloseAll->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui.m_actionTabsCloseAllExceptCurrent->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui.m_actionTabsCloseAllUnmodified->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui.m_actionTabsCloseCurrent->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui.m_actionTabsNext->setIcon(icon_theme_factory->fromTheme(QSL("go-next")));
  m_ui.m_actionTabsPrevious->setIcon(icon_theme_factory->fromTheme(QSL("go-previous")));
}

void FormMain::loadSize() {
  const Settings* settings = qApp->settings();

  toolBar()->setIsActive(settings->value(GROUP(GUI), SETTING(GUI::ToolbarsVisible)).toBool());
  statusBar()->setIsActive(settings->value(GROUP(GUI), SETTING(GUI::StatusBarVisible)).toBool());

  restoreGeometry(settings->value(GROUP(GUI), GUI::MainWindowGeometry).toByteArray());
  restoreState(settings->value(GROUP(GUI), GUI::MainWindowState).toByteArray());

  if (settings->value(GROUP(GUI), SETTING(GUI::StayOnTop)).toBool()) {
    switchStayOnTop();
  }
}

void FormMain::saveSize() {
  Settings* settings = qApp->settings();

  if (isFullScreen()) {
    switchFullscreenMode();

    // We (process events to really) un-fullscreen, so that we can determine if window is really maximized.
    qApp->processEvents();
  }

  settings->setValue(GROUP(GUI), GUI::ToolbarsVisible, toolBar()->isActive());
  settings->setValue(GROUP(GUI), GUI::StatusBarVisible, statusBar()->isActive());
  settings->setValue(GROUP(GUI), GUI::MainWindowGeometry, saveGeometry());
  settings->setValue(GROUP(GUI), GUI::MainWindowState, saveState());
}

void FormMain::createConnections() {
  // Menu "File" connections.
  connect(m_ui.m_actionQuit, &QAction::triggered, qApp, &Application::quitApplication);
  connect(m_ui.m_actionRestart, &QAction::triggered, this, [this]() {
    if (close()) {
      qApp->restart();
    }
  });

  // Menu "View" connections.
  connect(m_ui.m_menuShowHide, &QMenu::aboutToShow, [this]() {
    m_ui.m_actionFullscreen->setChecked(isFullScreen());
    m_ui.m_actionSwitchStatusBar->setChecked(statusBar()->isVisible());
    m_ui.m_actionSwitchToolBar->setChecked(toolBar()->isVisible());
  });
  connect(m_ui.m_actionFullscreen, &QAction::triggered, this, &FormMain::switchFullscreenMode);
  connect(m_ui.m_actionSwitchMainWindow, &QAction::triggered, this, &FormMain::switchVisibility);
  connect(m_ui.m_actionSwitchToolBar, &QAction::triggered, toolBar(), &ToolBar::setIsActive);
  connect(m_ui.m_actionSwitchStatusBar, &QAction::triggered, statusBar(), &StatusBar::setIsActive);
  connect(m_ui.m_actionStayOnTop, &QAction::triggered, this, &FormMain::switchStayOnTop);

  // Menu "Tools" connections.
  connect(m_ui.m_actionSettings, &QAction::triggered, this, [this]() {
    FormSettings(*this).exec();
  });

  // Menu "Help" connections.
  connect(m_ui.m_actionAboutGuard, &QAction::triggered, this, [this]() {
    FormAbout(this).exec();
  });
  connect(m_ui.m_actionCheckForUpdates, &QAction::triggered, this, [this]() {
    FormUpdate(this).exec();
  });
  connect(m_ui.m_actionReportBug, &QAction::triggered, this, &FormMain::reportABug);
  connect(m_ui.m_actionDonate, &QAction::triggered, this, &FormMain::donate);
  connect(m_ui.m_actionDisplayWiki, &QAction::triggered, this, &FormMain::showWiki);

  // Tab widget connections.
  connect(m_ui.m_actionTabsNext, &QAction::triggered, m_tabEditors, &TabWidget::gotoNextTab);
  connect(m_ui.m_actionTabsPrevious, &QAction::triggered, m_tabEditors, &TabWidget::gotoPreviousTab);
  connect(m_ui.m_actionTabsCloseAllExceptCurrent, &QAction::triggered, m_tabEditors, &TabWidget::closeAllTabsExceptCurrent);
  connect(m_ui.m_actionTabsCloseAll, &QAction::triggered, m_tabEditors, &TabWidget::closeAllTabs);
  connect(m_ui.m_actionTabsCloseCurrent, &QAction::triggered, m_tabEditors, &TabWidget::closeCurrentTab);
}

void FormMain::showWiki() {
  if (!qApp->web()->openUrlInExternalBrowser(APP_URL_WIKI)) {
    qApp->showGuiMessage(tr("Cannot open external browser. Navigate to application website manually."),
                         QMessageBox::Warning);
  }
}

void FormMain::reportABug() {
  if (!qApp->web()->openUrlInExternalBrowser(QSL(APP_URL_ISSUES_NEW))) {
    qApp->showGuiMessage(tr("Cannot open external browser. Navigate to application website manually."),
                         QMessageBox::Warning);
  }
}

void FormMain::donate() {
  if (!qApp->web()->openUrlInExternalBrowser(QSL(APP_DONATE_URL))) {
    qApp->showGuiMessage(tr("Cannot open external browser. Navigate to application website manually."),
                         QMessageBox::Warning);
  }
}
