// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Home/OrbitBackground.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHideEvent>
#include <QImageReader>
#include <QLinearGradient>
#include <QMovie>
#include <QPainter>
#include <QResizeEvent>
#include <QRadialGradient>
#include <QSaveFile>
#include <QSettings>
#include <QShowEvent>
#include <QVariantAnimation>

namespace
{
constexpr qint64 MAX_FILE_BYTES = 32 * 1024 * 1024;
constexpr int MAX_IMAGE_SIDE = 4096;
constexpr qint64 MAX_IMAGE_PIXELS = 16 * 1024 * 1024;
constexpr auto MOTION_KEY = "Orbit/BackgroundMotion";

bool ReadImage(QIODevice* device, QImage* image, QByteArray* format, QString* error)
{
  QImageReader reader(device);
  reader.setDecideFormatFromContent(true);
  reader.setAutoTransform(true);
  const auto fail = [error](const QString& message) {
    if (error)
      *error = message;
    return false;
  };
  if (!reader.canRead())
    return fail(OrbitBackground::tr("The file is not a supported image, or its image plugin is missing."));

  *format = reader.format().toLower();
  if (*format != "gif" && *format != "png" && *format != "jpeg" && *format != "webp" &&
      *format != "bmp")
    return fail(OrbitBackground::tr("Choose a GIF, PNG, JPEG, WebP or BMP image."));

  const QSize dimensions = reader.size();
  if (!dimensions.isValid() || dimensions.width() > MAX_IMAGE_SIDE ||
      dimensions.height() > MAX_IMAGE_SIDE ||
      qint64{dimensions.width()} * dimensions.height() > MAX_IMAGE_PIXELS)
    return fail(OrbitBackground::tr("The image must be at most 4096 pixels per side and 16 megapixels."));

  *image = reader.read();
  if (image->isNull())
    return fail(OrbitBackground::tr("The image could not be decoded: %1").arg(reader.errorString()));
  return true;
}

bool ReadFile(const QString& path, QByteArray* bytes, QImage* image, QByteArray* format,
              QString* error)
{
  if (!QFileInfo(path).isFile())
  {
    if (error)
      *error = OrbitBackground::tr("Choose an existing image file.");
    return false;
  }
  QFile file(path);
  if (!file.open(QIODevice::ReadOnly))
  {
    if (error)
      *error = OrbitBackground::tr("The background file could not be opened.");
    return false;
  }
  if (file.size() <= 0 || file.size() > MAX_FILE_BYTES)
  {
    if (error)
      *error = OrbitBackground::tr("Choose a non-empty image no larger than 32 MiB.");
    return false;
  }
  // Bound reads even if the source file grows while it is being imported.
  *bytes = file.read(MAX_FILE_BYTES + 1);
  if (bytes->size() > MAX_FILE_BYTES || file.error() != QFileDevice::NoError)
  {
    if (error)
      *error = OrbitBackground::tr("The background file could not be read within the 32 MiB limit.");
    return false;
  }
  QBuffer buffer(bytes);
  buffer.open(QIODevice::ReadOnly);
  return ReadImage(&buffer, image, format, error);
}
}  // namespace

OrbitBackground::OrbitBackground(QWidget* parent) : QWidget(parent)
{
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setFocusPolicy(Qt::NoFocus);

  m_transition = new QVariantAnimation(this);
  m_transition->setDuration(350);
  m_transition->setStartValue(0.0);
  m_transition->setEndValue(1.0);
  m_transition->setEasingCurve(QEasingCurve::InOutCubic);
  connect(m_transition, &QVariantAnimation::valueChanged, this, [this](const QVariant& value) {
    m_progress = value.toReal();
    update();
  });
  connect(m_transition, &QVariantAnimation::finished, this, [this] {
    m_previous = {};
    m_progress = 1.0;
    update();
  });
  connect(qGuiApp, &QGuiApplication::applicationStateChanged, this,
          [this](Qt::ApplicationState) { UpdatePlayback(); });
}

void OrbitBackground::SetSettings(QSettings* settings)
{
  if (m_settings == settings)
    return;
  BeginTransition();
  m_settings = settings;
  m_motion_enabled = settings ? settings->value(QString::fromLatin1(MOTION_KEY), true).toBool() : true;
  Reload();
  StartTransition();
  emit BackgroundChanged();
}

QString OrbitBackground::SettingsKey() const
{
  return QStringLiteral("Orbit/Backgrounds/%1")
      .arg(QString::fromLatin1(QCryptographicHash::hash(m_game_key.toUtf8(), QCryptographicHash::Sha256)
                                  .toHex()));
}

QString OrbitBackground::StorageDirectory() const
{
  if (!m_settings || m_settings->format() != QSettings::IniFormat ||
      !QDir::isAbsolutePath(m_settings->fileName()))
    return {};
  return QFileInfo(m_settings->fileName()).absoluteDir().filePath(QStringLiteral("Orbit/Backgrounds"));
}

void OrbitBackground::SetGame(const QString& key, const QPixmap& fallback)
{
  if (m_game_key == key && m_fallback.cacheKey() == fallback.cacheKey())
    return;
  // Model metadata/artwork refreshes must not restart the selected game's animation.
  if (m_game_key == key && HasCustomBackground())
  {
    m_fallback = fallback;
    return;
  }
  BeginTransition();
  m_game_key = key;
  m_fallback = fallback;
  if (!fallback.isNull())
  {
    // Disc banners are tiny. Use their palette for ambient light instead of stretching text.
    const QImage sample = fallback.toImage().scaled(8, 8, Qt::IgnoreAspectRatio,
                                                   Qt::SmoothTransformation);
    int red = 0, green = 0, blue = 0;
    for (int y = 0; y < sample.height(); ++y)
      for (int x = 0; x < sample.width(); ++x)
      {
        const QColor color = sample.pixelColor(x, y);
        red += color.red();
        green += color.green();
        blue += color.blue();
      }
    m_ambient_color = QColor(red / 64, green / 64, blue / 64, 170);
  }
  Reload();
  StartTransition();
  emit BackgroundChanged();
}

void OrbitBackground::StopMovie()
{
  if (m_movie)
  {
    m_movie->stop();
    delete m_movie;
    m_movie = nullptr;
  }
}

void OrbitBackground::Reload()
{
  StopMovie();
  m_custom_path.clear();
  m_frame = m_fallback;
  const QString directory = StorageDirectory();
  if (m_settings && !m_game_key.isEmpty() && !directory.isEmpty())
  {
    const QString name = m_settings->value(SettingsKey()).toString();
    // Settings only contain a basename; a hand-edited path cannot escape the profile directory.
    if (!name.isEmpty() && name == QFileInfo(name).fileName() && name != QStringLiteral(".") &&
        name != QStringLiteral(".."))
    {
      const QString path = QDir(directory).filePath(name);
      QByteArray bytes;
      QByteArray format;
      QImage image;
      if (!QFileInfo(path).isSymLink() && ReadFile(path, &bytes, &image, &format, nullptr))
      {
        m_custom_path = path;
        m_frame = QPixmap::fromImage(image);
        if (format == "gif")
        {
          m_movie = new QMovie(path, format, this);
          m_movie->setCacheMode(QMovie::CacheNone);
          connect(m_movie, &QMovie::frameChanged, this, [this, movie = m_movie](int) {
            if (m_movie != movie)
              return;
            m_frame = movie->currentPixmap();
            UpdateScaledFrame();
            update();
          });
          connect(m_movie, &QMovie::error, this,
                  [this, movie = m_movie](QImageReader::ImageReaderError) {
            if (m_movie != movie)
              return;
            // Defer deletion so QMovie is not destroyed while emitting one of its own signals.
            QMovie* failed = m_movie;
            m_movie = nullptr;
            if (failed)
            {
              failed->stop();
              failed->deleteLater();
            }
            m_custom_path.clear();
            m_frame = m_fallback;
            UpdateScaledFrame();
            update();
            emit BackgroundChanged();
          });
        }
      }
    }
  }
  UpdateScaledFrame();
  UpdatePlayback();
  update();
}

bool OrbitBackground::SetBackgroundFile(const QString& path, QString* error)
{
  if (error)
    error->clear();
  const auto fail = [error](const QString& message) {
    if (error)
      *error = message;
    return false;
  };
  if (!m_settings || m_game_key.isEmpty())
    return fail(tr("Select a game and an active profile before importing a background."));

  QByteArray bytes;
  QByteArray format;
  QImage image;
  if (!ReadFile(path, &bytes, &image, &format, error))
    return false;

  const QString directory = StorageDirectory();
  const QFileInfo orbit_directory(QFileInfo(directory).absolutePath());
  if (directory.isEmpty() || orbit_directory.isSymLink() || QFileInfo(directory).isSymLink() ||
      !QDir().mkpath(directory))
    return fail(tr("The profile background directory could not be created safely."));

  const QString name = QStringLiteral("%1.%2")
                           .arg(QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex()),
                                QString::fromLatin1(format));
  const QString destination = QDir(directory).filePath(name);
  if (QFileInfo(destination).isSymLink())
    return fail(tr("The destination background must not be a symbolic link."));
  QSaveFile output(destination);
  output.setDirectWriteFallback(false);
  if (!output.open(QIODevice::WriteOnly) || output.write(bytes) != bytes.size() || !output.commit())
    return fail(tr("The background could not be copied into the profile."));

  const QVariant previous = m_settings->value(SettingsKey());
  m_settings->setValue(SettingsKey(), name);
  m_settings->sync();
  if (m_settings->status() != QSettings::NoError)
  {
    if (previous.isValid())
      m_settings->setValue(SettingsKey(), previous);
    else
      m_settings->remove(SettingsKey());
    return fail(tr("The background preference could not be saved."));
  }
  BeginTransition();
  Reload();
  StartTransition();
  emit BackgroundChanged();
  if (!HasCustomBackground())
    return fail(tr("The imported background could not be loaded; the default background is shown."));
  return true;
}

void OrbitBackground::ClearBackground()
{
  if (!m_settings || m_game_key.isEmpty())
    return;
  BeginTransition();
  m_settings->remove(SettingsKey());
  m_settings->sync();
  // Keep imported files: another game can use the same content, and external files are never deleted.
  Reload();
  StartTransition();
  emit BackgroundChanged();
}

bool OrbitBackground::HasCustomBackground() const
{
  return !m_custom_path.isEmpty();
}

void OrbitBackground::SetMotionEnabled(bool enabled)
{
  m_motion_enabled = enabled;
  if (m_settings)
  {
    m_settings->setValue(QString::fromLatin1(MOTION_KEY), enabled);
    m_settings->sync();
  }
  if (!enabled)
  {
    m_transition->stop();
    m_previous = {};
    m_progress = 1.0;
  }
  UpdatePlayback();
  update();
}

bool OrbitBackground::MotionEnabled() const
{
  return m_motion_enabled;
}

void OrbitBackground::SetPlaybackEnabled(bool enabled)
{
  m_playback_enabled = enabled;
  UpdatePlayback();
}

void OrbitBackground::UpdatePlayback()
{
  const bool active = m_motion_enabled && m_playback_enabled && isVisible() &&
                      !window()->isMinimized() &&
                      QGuiApplication::applicationState() == Qt::ApplicationActive;
  if (m_movie)
  {
    if (active && m_movie->state() == QMovie::NotRunning)
      m_movie->start();
    else if (m_movie->state() != QMovie::NotRunning)
      m_movie->setPaused(!active);
  }
  if (active && m_transition->state() == QAbstractAnimation::Paused)
    m_transition->resume();
  else if (!active && m_transition->state() == QAbstractAnimation::Running)
    m_transition->pause();
}

void OrbitBackground::UpdateScaledFrame()
{
  if (m_custom_path.isEmpty() || m_frame.isNull() || size().isEmpty())
  {
    m_scaled_frame = {};
    return;
  }
  // Scaling happens once per image/frame or resize, never on every crossfade repaint.
  const qreal ratio = devicePixelRatioF();
  m_scaled_frame = m_frame.scaled(size() * ratio, Qt::KeepAspectRatioByExpanding,
                                  Qt::SmoothTransformation);
  m_scaled_frame.setDevicePixelRatio(ratio);
}

void OrbitBackground::BeginTransition()
{
  m_transition->stop();
  if (isVisible() && !size().isEmpty() && m_motion_enabled && m_playback_enabled)
  {
    QPixmap snapshot(size() * devicePixelRatioF());
    snapshot.setDevicePixelRatio(devicePixelRatioF());
    QPainter painter(&snapshot);
    PaintBase(painter);
    painter.end();
    m_previous = snapshot;
  }
  else
  {
    m_previous = {};
  }
}

void OrbitBackground::StartTransition()
{
  if (!m_previous.isNull() && m_motion_enabled && m_playback_enabled)
  {
    m_progress = 0.0;
    m_transition->start();
    UpdatePlayback();
  }
  else
  {
    m_previous = {};
    m_progress = 1.0;
  }
  update();
}

void OrbitBackground::PaintBase(QPainter& painter) const
{
  QLinearGradient base(rect().topLeft(), rect().bottomRight());
  base.setColorAt(0.0, QColor("#102841"));
  base.setColorAt(0.55, QColor("#101c33"));
  base.setColorAt(1.0, QColor("#291d38"));
  painter.fillRect(rect(), base);
  QRadialGradient ambient(QPointF(width() * 0.75, height() * 0.28), width() * 0.7);
  ambient.setColorAt(0.0, m_ambient_color);
  ambient.setColorAt(1.0, Qt::transparent);
  painter.fillRect(rect(), ambient);
  if (!m_scaled_frame.isNull())
  {
    const QSizeF logical = m_scaled_frame.deviceIndependentSize();
    painter.drawPixmap(QPointF((width() - logical.width()) / 2.0,
                              (height() - logical.height()) / 2.0),
                       m_scaled_frame);
  }
  if (!m_previous.isNull() && m_progress < 1.0)
  {
    painter.save();
    painter.setOpacity(1.0 - m_progress);
    painter.drawPixmap(rect(), m_previous);
    painter.restore();
  }
}

void OrbitBackground::paintEvent(QPaintEvent*)
{
  QPainter painter(this);
  PaintBase(painter);

  // Strong left and lower shades keep the hero text and game carousel readable over any image.
  QLinearGradient horizontal(0, 0, width(), 0);
  horizontal.setColorAt(0.0, QColor(5, 12, 24, 241));
  horizontal.setColorAt(0.35, QColor(5, 12, 24, 204));
  horizontal.setColorAt(0.72, QColor(5, 12, 24, 48));
  horizontal.setColorAt(1.0, QColor(5, 12, 24, 22));
  painter.fillRect(rect(), horizontal);

  QLinearGradient vertical(0, 0, 0, height());
  vertical.setColorAt(0.0, QColor(5, 12, 24, 100));
  vertical.setColorAt(0.38, QColor(5, 12, 24, 8));
  vertical.setColorAt(0.72, QColor(5, 12, 24, 168));
  vertical.setColorAt(1.0, QColor(5, 12, 24, 252));
  painter.fillRect(rect(), vertical);
}

void OrbitBackground::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  UpdateScaledFrame();
}

void OrbitBackground::showEvent(QShowEvent* event)
{
  QWidget::showEvent(event);
  if (m_observed_window != window())
  {
    if (m_observed_window)
      m_observed_window->removeEventFilter(this);
    m_observed_window = window();
    m_observed_window->installEventFilter(this);
  }
  UpdatePlayback();
}

void OrbitBackground::hideEvent(QHideEvent* event)
{
  QWidget::hideEvent(event);
  UpdatePlayback();
}

bool OrbitBackground::eventFilter(QObject* watched, QEvent* event)
{
  if (watched == m_observed_window &&
      (event->type() == QEvent::WindowStateChange || event->type() == QEvent::Show ||
       event->type() == QEvent::Hide))
    UpdatePlayback();
  return QWidget::eventFilter(watched, event);
}
