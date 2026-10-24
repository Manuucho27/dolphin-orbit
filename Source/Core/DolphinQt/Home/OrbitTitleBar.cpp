// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Home/OrbitTitleBar.h"

#include <QAction>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QShortcut>
#include <QStyleOptionToolButton>
#include <QStylePainter>
#include <QToolBar>
#include <QToolButton>
#include <QWindow>

namespace
{
enum class WindowGlyph
{
  Minimize,
  Maximize,
  Restore,
  Close,
};

class OrbitCommandButton final : public QToolButton
{
public:
  explicit OrbitCommandButton(QWidget* parent) : QToolButton(parent) {}

protected:
  void paintEvent(QPaintEvent*) override
  {
    QStylePainter painter(this);
    QStyleOptionToolButton option;
    initStyleOption(&option);
    QPixmap icon = option.icon.pixmap(option.iconSize, devicePixelRatioF());
    if (!icon.isNull())
    {
      QPainter tint(&icon);
      tint.setCompositionMode(QPainter::CompositionMode_SourceIn);
      tint.fillRect(icon.rect(), isEnabled() ? QColor(QStringLiteral("#dce7f7")) :
                                              QColor(QStringLiteral("#67758b")));
      tint.end();
      option.icon = QIcon(icon);
    }
    painter.drawComplexControl(QStyle::CC_ToolButton, option);
  }
};

QIcon WindowIcon(WindowGlyph glyph)
{
  QPixmap pixmap(16, 16);
  pixmap.fill(Qt::transparent);
  QPainter painter(&pixmap);
  painter.setPen(QPen(QColor(QStringLiteral("#ecf0f8")), 1.2));
  switch (glyph)
  {
  case WindowGlyph::Minimize:
    painter.drawLine(3, 10, 13, 10);
    break;
  case WindowGlyph::Maximize:
    painter.drawRect(3, 3, 10, 10);
    break;
  case WindowGlyph::Restore:
    painter.drawRect(5, 2, 8, 8);
    painter.fillRect(QRect(2, 5, 8, 8), QColor(QStringLiteral("#101a29")));
    painter.drawRect(2, 5, 8, 8);
    break;
  case WindowGlyph::Close:
    painter.drawLine(3, 3, 13, 13);
    painter.drawLine(13, 3, 3, 13);
    break;
  }
  return QIcon(pixmap);
}

const QString POPUP_STYLE = QStringLiteral(R"(
  QMenu {
    color: #eaf0fa; background: #111d2e; border: 1px solid #314059;
    border-radius: 9px; padding: 7px;
  }
  QMenu::item { padding: 7px 28px 7px 12px; border-radius: 5px; }
  QMenu::item:selected { color: #ffffff; background: #2b4159; }
  QMenu::item:disabled { color: #69798f; }
  QMenu::separator { height: 1px; background: #29384d; margin: 6px 8px; }
)");
}  // namespace

OrbitTitleBar::OrbitTitleBar(QWidget* window, QMenuBar* menu_bar, QToolBar* tool_bar)
    : QWidget(window), m_window(window), m_source_menu_bar(menu_bar)
{
  setObjectName(QStringLiteral("OrbitTitleBar"));
  setAttribute(Qt::WA_StyledBackground);
  setFixedHeight(52);
  setStyleSheet(QStringLiteral(R"(
    QWidget#OrbitTitleBar { background: #0c1422; border-bottom: 1px solid #233047; }
    QWidget#OrbitTitleBar QLabel#OrbitWindowBrand {
      color: #70ece4; font-size: 18px; font-weight: 800; letter-spacing: 3px;
      background: transparent; border: none;
    }
    QWidget#OrbitTitleBar QToolButton {
      color: #dce5f5; background: transparent; border: 1px solid transparent;
      border-radius: 7px; padding: 3px; font-size: 12px;
    }
    QWidget#OrbitTitleBar QToolButton:hover { background: #26364b; }
    QWidget#OrbitTitleBar QToolButton:pressed { background: #314962; }
    QWidget#OrbitTitleBar QToolButton:focus { border-color: #70ece4; }
    QWidget#OrbitTitleBar QToolButton:disabled { color: #65738b; }
    QWidget#OrbitTitleBar QToolButton#OrbitLibrary { color: #8deee8; background: #152e3a; }
    QWidget#OrbitTitleBar QToolButton#OrbitClose:hover { background: #a2384d; }
    QWidget#OrbitTitleBar QToolButton::menu-indicator { image: none; }
  )"));

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(18, 5, 5, 5);
  layout->setSpacing(5);
  auto* brand = new QLabel(QStringLiteral("ORBIT"), this);
  brand->setObjectName(QStringLiteral("OrbitWindowBrand"));
  brand->setAttribute(Qt::WA_TransparentForMouseEvents);
  brand->setContentsMargins(0, 0, 10, 0);
  layout->addWidget(brand);

  m_library = new QToolButton(this);
  m_library->setObjectName(QStringLiteral("OrbitLibrary"));
  m_library->setText(tr("Biblioteca"));
  m_library->setToolTip(tr("Volver a la biblioteca"));
  m_library->setFixedSize(76, 32);
  layout->addWidget(m_library);
  connect(m_library, &QToolButton::clicked, this, &OrbitTitleBar::LibraryRequested);

  const auto add_command = [this, layout](QAction* action, bool show_text = false) {
    auto* button = new OrbitCommandButton(this);
    button->setDefaultAction(action);
    button->setToolButtonStyle(show_text ? Qt::ToolButtonTextBesideIcon : Qt::ToolButtonIconOnly);
    button->setIconSize(QSize(18, 18));
    button->setFixedSize(show_text ? 80 : 32, 32);
    layout->addWidget(button);
    return button;
  };

  // ToolBar::MakeActions owns this order. Reuse those QAction instances, including Play/Pause's
  // changing trigger and all enabled/visible states; never duplicate emulation commands.
  const auto commands = tool_bar->actions();
  Q_ASSERT(commands.size() == 17 && commands[8]->isSeparator() && commands[13]->isSeparator());
  if (commands.size() == 17 && commands[8]->isSeparator() && commands[13]->isSeparator())
  {
    add_command(commands[6]);   // Open
    add_command(commands[7]);   // Refresh
    layout->addStretch(1);      // Empty space remains a native window drag target.
    m_emulation_buttons.push_back(add_command(commands[9], true));
    for (int index : {10, 11})  // Stop, fullscreen
      m_emulation_buttons.push_back(add_command(commands[index]));
    m_screenshot = add_command(commands[12]);
    for (int index : {14, 16, 15})  // Settings, controllers, graphics
      m_extra_buttons.push_back(add_command(commands[index]));
  }
  else
  {
    layout->addStretch(1);
  }

  m_more_menu = new QMenu(this);
  m_more_menu->setStyleSheet(POPUP_STYLE);
  m_more = new QToolButton(this);
  m_more->setObjectName(QStringLiteral("OrbitMore"));
  m_more->setText(tr("Más"));
  m_more->setToolTip(tr("Todos los comandos de Dolphin"));
  m_more->setFixedSize(56, 32);
  m_more->setPopupMode(QToolButton::InstantPopup);
  m_more->setMenu(m_more_menu);
  layout->addWidget(m_more);
  m_menu_separator = m_more_menu->addSeparator();
  m_debug_menu = m_more_menu->addMenu(tr("Controles de depuración"));
  m_debug_menu->setStyleSheet(POPUP_STYLE);
  if (commands.size() == 17)
  {
    for (int index = 0; index < 6; ++index)
      m_debug_menu->addAction(commands[index]);
  }

  const auto add_window_button = [this, layout](WindowGlyph glyph, const QString& description) {
    auto* button = new QToolButton(this);
    button->setIcon(WindowIcon(glyph));
    button->setIconSize(QSize(16, 16));
    button->setFixedSize(36, 36);
    button->setToolTip(description);
    button->setAccessibleName(description);
    layout->addWidget(button);
    return button;
  };
  auto* minimize = add_window_button(WindowGlyph::Minimize, tr("Minimizar"));
  minimize->setObjectName(QStringLiteral("OrbitMinimize"));
  m_maximize = add_window_button(WindowGlyph::Maximize, tr("Maximizar"));
  m_maximize->setObjectName(QStringLiteral("OrbitMaximize"));
  auto* close = add_window_button(WindowGlyph::Close, tr("Cerrar"));
  close->setObjectName(QStringLiteral("OrbitClose"));
  connect(minimize, &QToolButton::clicked, m_window, &QWidget::showMinimized);
  connect(m_maximize, &QToolButton::clicked, this, [this] {
    if (m_window->isMaximized() || m_window->isFullScreen())
      m_window->showNormal();
    else
      m_window->showMaximized();
  });
  // Keep MainWindow's close event and its existing emulation-stop confirmation in charge.
  connect(close, &QToolButton::clicked, m_window, &QWidget::close);
  m_source_menu_bar->installEventFilter(this);
  m_window->installEventFilter(this);
  connect(m_window, &QWidget::windowTitleChanged, brand, &QWidget::setToolTip);
  SyncMenus();
  UpdateWindowButton();
  UpdateResponsiveActions();
}

void OrbitTitleBar::SyncMenus()
{
  const auto roots = m_source_menu_bar->actions();
  for (QAction* previous : m_root_menu_actions)
  {
    if (!roots.contains(previous))
      m_more_menu->removeAction(previous);
  }
  for (QAction* action : roots)
    m_more_menu->insertAction(m_menu_separator, action);
  m_root_menu_actions = roots;

  QSet<QAction*> desired;
  const auto collect = [&desired](const auto& self, QAction* action) -> void {
    if (desired.contains(action))
      return;
    desired.insert(action);
    if (QMenu* menu = action->menu())
    {
      for (QAction* child : menu->actions())
        self(self, child);
    }
  };
  for (QAction* action : roots)
    collect(collect, action);

  const auto obsolete = m_registered_actions - desired;
  for (QAction* action : obsolete)
  {
    m_window->removeAction(action);
    if (QMenu* menu = action->menu())
      menu->removeEventFilter(this);
    m_registered_actions.remove(action);
  }
  for (auto it = m_menu_shortcuts.begin(); it != m_menu_shortcuts.end();)
  {
    if (!roots.contains(it.key()))
    {
      delete it.value();
      it = m_menu_shortcuts.erase(it);
    }
    else
    {
      ++it;
    }
  }
  for (QAction* action : desired)
  {
    if (!m_registered_actions.contains(action))
    {
      if (!action->isSeparator())
        m_window->addAction(action);
      if (QMenu* menu = action->menu())
      {
        menu->setStyleSheet(POPUP_STYLE);
        menu->installEventFilter(this);
      }
      m_registered_actions.insert(action);
    }
    if (!m_observed_actions.contains(action))
    {
      m_observed_actions.insert(action);
      connect(action, &QObject::destroyed, this, [this, action] {
        m_registered_actions.remove(action);
        m_observed_actions.remove(action);
        m_root_menu_actions.removeAll(action);
        delete m_menu_shortcuts.take(action);
      });
    }
  }
  for (QAction* action : roots)
  {
    const auto mnemonic = QKeySequence::mnemonic(action->text());
    if (action->menu() && !mnemonic.isEmpty() && !m_menu_shortcuts.contains(action))
    {
      auto* shortcut = new QShortcut(mnemonic, m_window);
      m_menu_shortcuts.insert(action, shortcut);
      connect(shortcut, &QShortcut::activated, this, [this, action] {
        if (action->isVisible() && action->isEnabled())
          action->menu()->popup(m_more->mapToGlobal(QPoint(0, m_more->height())));
      });
      const auto update_shortcut = [action, shortcut] {
        shortcut->setEnabled(action->isVisible() && action->isEnabled());
        shortcut->setKey(QKeySequence::mnemonic(action->text()));
      };
      connect(action, &QAction::changed, shortcut, update_shortcut);
      update_shortcut();
    }
  }
}

bool OrbitTitleBar::eventFilter(QObject* object, QEvent* event)
{
  if (object == m_window && event->type() == QEvent::WindowStateChange)
    UpdateWindowButton();
  if ((object == m_source_menu_bar || qobject_cast<QMenu*>(object)) &&
      (event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionRemoved))
  {
    SyncMenus();
  }
  return QWidget::eventFilter(object, event);
}

void OrbitTitleBar::UpdateWindowButton()
{
  const bool restore = m_window->isMaximized() || m_window->isFullScreen();
  m_maximize->setIcon(WindowIcon(restore ? WindowGlyph::Restore : WindowGlyph::Maximize));
  m_maximize->setToolTip(restore ? tr("Restaurar") : tr("Maximizar"));
  m_maximize->setAccessibleName(m_maximize->toolTip());
}

void OrbitTitleBar::SetEmulationActive(bool active)
{
  m_emulation_active = active;
  UpdateResponsiveActions();
}

void OrbitTitleBar::SetDebugModeEnabled(bool enabled)
{
  m_debug_menu->menuAction()->setVisible(enabled);
}

void OrbitTitleBar::SetLibraryEnabled(bool enabled)
{
  m_library->setEnabled(enabled);
}

void OrbitTitleBar::UpdateResponsiveActions()
{
  for (QToolButton* button : m_emulation_buttons)
    button->setVisible(m_emulation_active);
  for (QToolButton* button : m_extra_buttons)
    button->setVisible(width() >= 1100);
  if (m_screenshot)
    m_screenshot->setVisible(m_emulation_active && width() >= 1100);
}

void OrbitTitleBar::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  UpdateResponsiveActions();
}

void OrbitTitleBar::mousePressEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton && m_window->windowHandle() &&
      m_window->windowHandle()->startSystemMove())
  {
    event->accept();
    return;
  }
  QWidget::mousePressEvent(event);
}

void OrbitTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    m_maximize->click();
    event->accept();
    return;
  }
  QWidget::mouseDoubleClickEvent(event);
}
