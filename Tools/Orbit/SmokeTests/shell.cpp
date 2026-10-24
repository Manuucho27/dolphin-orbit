// Exercise the production chrome without requiring the emulation core.
#include <stdexcept>

#include <QAction>
#include <QApplication>
#include <QEventLoop>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QShortcut>
#include <QToolBar>
#include <QToolButton>

#include "DolphinQt/Home/OrbitTitleBar.h"

namespace
{
void Require(bool condition, const char* message)
{
  if (!condition)
    throw std::runtime_error(message);
}
void Pump()
{
  QCoreApplication::sendPostedEvents();
  QCoreApplication::processEvents(QEventLoop::AllEvents);
}
}  // namespace

void RunShellTests()
{
  QMainWindow window;
  window.setWindowFlag(Qt::FramelessWindowHint);
  auto* menus = new QMenuBar(&window);
  auto* file = menus->addMenu(QStringLiteral("&Archivo"));
  auto* initial_action = file->addAction(QStringLiteral("Inicial"));
  auto* tools = new QToolBar(&window);
  for (int i = 0; i < 17; ++i)
  {
    if (i == 8 || i == 13)
      tools->addSeparator();
    else
      tools->addAction(QStringLiteral("Command %1").arg(i));
  }
  auto* chrome = new OrbitTitleBar(&window, menus, tools);
  window.setMenuWidget(chrome);
  window.setCentralWidget(new QWidget);
  window.resize(900, 600);
  window.show();
  window.activateWindow();
  Pump();
  auto* more = chrome->findChild<QToolButton*>(QStringLiteral("OrbitMore"));
  Require(more && more->menu(), "The integrated menu is missing");
  Require(more->menu()->actions().contains(file->menuAction()), "Original menus were not retained");
  Require(window.actions().contains(initial_action), "Original action shortcut was not registered");
  for (auto* shortcut : window.findChildren<QShortcut*>())
    Require(shortcut->key() != QKeySequence(Qt::Key_F10), "More shadows Dolphin's F10 Play/Pause hotkey");

  auto* jit = menus->addMenu(QStringLiteral("&JIT"));
  auto* dynamic = jit->addAction(QStringLiteral("Dynamic"));
  dynamic->setShortcut(QKeySequence(QStringLiteral("Ctrl+Shift+9")));
  Pump();
  Require(more->menu()->actions().contains(jit->menuAction()), "A newly added debugger menu is missing");
  Require(window.actions().contains(dynamic), "A dynamically added shortcut was not registered");
  menus->removeAction(jit->menuAction());
  Pump();
  Require(!more->menu()->actions().contains(jit->menuAction()), "A removed debugger menu remains visible");
  Require(!window.actions().contains(dynamic), "A removed menu left a hidden shortcut active");
  menus->addAction(jit->menuAction());
  Pump();
  Require(more->menu()->actions().contains(jit->menuAction()), "A debugger menu could not be re-added");
  Require(window.actions().count(dynamic) == 1, "Re-added menu duplicated shortcut registrations");

  QAction* open = tools->actions()[6];
  QToolButton* open_button = nullptr;
  for (auto* button : chrome->findChildren<QToolButton*>())
  {
    if (button->defaultAction() == open)
      open_button = button;
  }
  Require(open_button && open_button->isVisible(), "Open is missing from the integrated header");
  int opens = 0;
  QObject::connect(open, &QAction::triggered, &window, [&] { ++opens; });
  open_button->click();
  Require(opens == 1, "An integrated command did not invoke the original action exactly once");
  open->setEnabled(false);
  Require(!open_button->isEnabled(), "Integrated command ignored the original enabled state");
  chrome->SetEmulationActive(true);
  Pump();
  Require(more->isVisible() && chrome->rect().contains(more->geometry()),
          "The integrated menu is clipped at 900px during emulation");
  auto* maximize = chrome->findChild<QToolButton*>(QStringLiteral("OrbitMaximize"));
  Require(maximize, "The integrated maximize control is missing");
  maximize->click();
  Pump();
  Require(window.isMaximized(), "Integrated maximize did not maximize the window");
  maximize->click();
  Pump();
  Require(!window.isMaximized(), "Integrated restore did not restore the window");
}
