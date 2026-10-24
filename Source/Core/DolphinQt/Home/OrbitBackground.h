// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QColor>
#include <QPixmap>
#include <QPointer>
#include <QString>
#include <QWidget>

class QHideEvent;
class QMovie;
class QPaintEvent;
class QPainter;
class QResizeEvent;
class QSettings;
class QShowEvent;
class QVariantAnimation;

// Backgrounds belong to the active Qt.ini profile and never reference imported source files.
class OrbitBackground final : public QWidget
{
  Q_OBJECT

public:
  explicit OrbitBackground(QWidget* parent = nullptr);

  void SetSettings(QSettings* settings);
  void SetGame(const QString& key, const QPixmap& fallback);
  bool SetBackgroundFile(const QString& path, QString* error = nullptr);
  void ClearBackground();
  bool HasCustomBackground() const;
  void SetMotionEnabled(bool enabled);
  bool MotionEnabled() const;
  void SetPlaybackEnabled(bool enabled);

signals:
  void BackgroundChanged();

protected:
  void paintEvent(QPaintEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void showEvent(QShowEvent* event) override;
  void hideEvent(QHideEvent* event) override;
  bool eventFilter(QObject* watched, QEvent* event) override;

private:
  QString SettingsKey() const;
  QString StorageDirectory() const;
  void Reload();
  void StopMovie();
  void UpdatePlayback();
  void UpdateScaledFrame();
  void BeginTransition();
  void StartTransition();
  void PaintBase(QPainter& painter) const;

  QPointer<QSettings> m_settings;
  QPointer<QWidget> m_observed_window;
  QString m_game_key;
  QString m_custom_path;
  QPixmap m_fallback;
  QPixmap m_frame;
  QPixmap m_scaled_frame;
  QPixmap m_previous;
  QColor m_ambient_color = QColor(60, 151, 159, 160);
  QMovie* m_movie = nullptr;
  QVariantAnimation* m_transition = nullptr;
  qreal m_progress = 1.0;
  bool m_motion_enabled = true;
  bool m_playback_enabled = true;
};
