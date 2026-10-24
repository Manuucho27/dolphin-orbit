// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>
#include <QList>
#include <QSet>
#include <QWidget>

#include <vector>

class QAction;
class QMenu;
class QMenuBar;
class QMouseEvent;
class QResizeEvent;
class QShortcut;
class QToolBar;
class QToolButton;

// Window chrome only: all Dolphin commands remain owned by the existing menu and toolbar.
class OrbitTitleBar final : public QWidget
{
  Q_OBJECT

public:
  OrbitTitleBar(QWidget* window, QMenuBar* menu_bar, QToolBar* tool_bar);

  void SetEmulationActive(bool active);
  void SetDebugModeEnabled(bool enabled);
  void SetLibraryEnabled(bool enabled);

signals:
  void LibraryRequested();

protected:
  bool eventFilter(QObject* object, QEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;

private:
  void SyncMenus();
  void UpdateWindowButton();
  void UpdateResponsiveActions();

  QWidget* m_window;
  QMenuBar* m_source_menu_bar;
  QMenu* m_more_menu;
  QMenu* m_debug_menu;
  QAction* m_menu_separator;
  QList<QAction*> m_root_menu_actions;
  QSet<QAction*> m_registered_actions;
  QSet<QAction*> m_observed_actions;
  QHash<QAction*, QShortcut*> m_menu_shortcuts;
  QToolButton* m_library;
  QToolButton* m_more;
  QToolButton* m_maximize;
  std::vector<QToolButton*> m_emulation_buttons;
  std::vector<QToolButton*> m_extra_buttons;
  QToolButton* m_screenshot = nullptr;
  bool m_emulation_active = false;
};
