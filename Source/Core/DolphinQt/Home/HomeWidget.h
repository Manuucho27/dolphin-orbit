// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QString>
#include <QWidget>

class OrbitBackground;
class QGraphicsOpacityEffect;
class QLabel;
class QListView;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QSettings;
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
  void SetBackgroundSettings(QSettings* settings);
  void SetBackgroundPlaybackEnabled(bool enabled);
  void SetGameCountVisible(bool visible);

signals:
  void GameSelected();
  void OpenGeneralSettings();
  void OpenGraphicsSettings();
  void OpenControllerSettings();
  void OpenFolderRequested();

protected:
  void resizeEvent(QResizeEvent* event) override;

private:
  void RefreshLibrary();
  void UpdateSelectedGame();
  bool HasSelectedGame() const;
  void ShowBackgroundMenu();
  void UpdateMotionPreference();

  QSortFilterProxyModel* m_model;
  OrbitBackground* m_background;
  QListView* m_view;
  QStackedWidget* m_content;
  QWidget* m_library;
  QWidget* m_hero;
  QWidget* m_empty_emblem;
  QLabel* m_title;
  QLabel* m_metadata;
  QLabel* m_description;
  QLabel* m_count;
  QLabel* m_empty_title;
  QLabel* m_empty_description;
  QPushButton* m_play;
  QPushButton* m_customize;
  QGraphicsOpacityEffect* m_hero_opacity;
  QPropertyAnimation* m_hero_animation;
  QString m_current_game_key;
};
