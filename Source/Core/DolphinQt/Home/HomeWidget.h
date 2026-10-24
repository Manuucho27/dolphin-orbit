// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QWidget>

class QLabel;
class QListView;
class QPushButton;
class QSortFilterProxyModel;
class QStackedWidget;

// A presentation of the existing game-list proxy. GameList remains responsible for launching games,
// context menus and emulation state.
class HomeWidget final : public QWidget
{
  Q_OBJECT

public:
  explicit HomeWidget(QSortFilterProxyModel* model, QWidget* parent = nullptr);

  QListView* GetView() const { return m_view; }

signals:
  void GameSelected();
  void OpenGeneralSettings();
  void OpenGraphicsSettings();
  void OpenControllerSettings();
  void OpenFolderRequested();

private:
  void RefreshLibrary();
  void UpdateSelectedGame();
  bool HasSelectedGame() const;

  QSortFilterProxyModel* m_model;
  QListView* m_view;
  QStackedWidget* m_content;
  QWidget* m_library;
  QLabel* m_title;
  QLabel* m_metadata;
  QLabel* m_description;
  QLabel* m_artwork;
  QLabel* m_count;
  QLabel* m_empty_title;
  QLabel* m_empty_description;
  QPushButton* m_play;
};
