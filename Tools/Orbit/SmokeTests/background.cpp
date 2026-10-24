// Tests of the production background store and real Qt GIF decoder using synthetic artwork.
#include <stdexcept>

#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImageReader>
#include <QMovie>
#include <QSettings>
#include <QTemporaryDir>
#include <QTimer>

#include "DolphinQt/Home/OrbitBackground.h"

namespace
{
void Require(bool condition, const char* message)
{
  if (!condition)
    throw std::runtime_error(message);
}

void WaitForFrames(int milliseconds)
{
  QEventLoop loop;
  QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
  loop.exec();
}
}  // namespace

void RunBackgroundTests()
{
  Require(QImageReader::supportedImageFormats().contains("gif"),
          "The deployed Qt GIF decoder is missing");
  QTemporaryDir directory;
  Require(directory.isValid(), "Cannot create temporary profile");
  QSettings settings(directory.filePath(QStringLiteral("Qt.ini")), QSettings::IniFormat);
  const QString original = directory.filePath(QStringLiteral("original.gif"));
  Require(QFile::copy(QString::fromUtf8(ORBIT_FIXTURE_PATH), original), "Cannot copy GIF fixture");
  const QString first_game = directory.filePath(QStringLiteral("games/first.rvz"));
  const QString second_game = directory.filePath(QStringLiteral("games/second.rvz"));
  QPixmap fallback(64, 48);
  fallback.fill(Qt::darkBlue);

  OrbitBackground background;
  background.resize(640, 360);
  background.SetSettings(&settings);
  background.SetGame(first_game, fallback);
  background.show();
  background.activateWindow();
  WaitForFrames(30);
  QString error;
  Require(background.SetBackgroundFile(original, &error), "Cannot import a valid animated GIF");
  Require(background.HasCustomBackground(), "Imported background was not selected");
  auto* movie = background.findChild<QMovie*>();
  Require(movie && movie->isValid(), "A valid GIF did not create a QMovie");
  int frame_events = 0;
  QColor previous_color;
  bool pixels_changed = false;
  QObject::connect(movie, &QMovie::frameChanged, &background, [&] {
    ++frame_events;
    const QColor color = movie->currentImage().pixelColor(0, 0);
    if (previous_color.isValid() && previous_color != color)
      pixels_changed = true;
    previous_color = color;
  });
  WaitForFrames(420);
  Require(frame_events >= 2 && pixels_changed, "GIF frames did not actually animate");
  Require(movie->cacheMode() == QMovie::CacheNone, "GIF retains all decoded frames in memory");

  background.SetPlaybackEnabled(false);
  Require(movie->state() != QMovie::Running, "Emulation suspension left the GIF running");
  background.SetPlaybackEnabled(true);
  WaitForFrames(100);
  Require(movie->state() == QMovie::Running, "GIF did not resume after emulation");
  background.hide();
  WaitForFrames(30);
  Require(movie->state() != QMovie::Running, "Hidden Home still decodes GIF frames");
  background.show();
  background.activateWindow();
  background.SetMotionEnabled(false);
  WaitForFrames(30);
  Require(movie->state() != QMovie::Running, "Reduced motion left GIF playback enabled");
  background.SetMotionEnabled(true);

  const QString invalid = directory.filePath(QStringLiteral("invalid.gif"));
  QFile invalid_file(invalid);
  Require(invalid_file.open(QIODevice::WriteOnly), "Cannot write invalid fixture");
  invalid_file.write("not an image");
  invalid_file.close();
  Require(!background.SetBackgroundFile(invalid, &error) && !error.isEmpty(),
          "Corrupt background was accepted without an error");
  Require(background.HasCustomBackground(), "Failed import discarded the existing background");
  background.SetGame(second_game, fallback);
  Require(!background.HasCustomBackground(), "A game inherited another game's background");
  Require(QFile::remove(original), "Cannot remove the original test file");
  background.SetGame(first_game, fallback);
  Require(background.HasCustomBackground(), "Moving the original file broke the imported copy");

  settings.sync();
  QSettings restored_settings(settings.fileName(), QSettings::IniFormat);
  OrbitBackground restored;
  restored.SetSettings(&restored_settings);
  restored.SetGame(first_game, fallback);
  Require(restored.HasCustomBackground(), "Per-game background was not persisted across sessions");
  restored.SetMotionEnabled(false);
  restored_settings.sync();
  QSettings motion_settings(settings.fileName(), QSettings::IniFormat);
  OrbitBackground motion_restored;
  motion_restored.SetSettings(&motion_settings);
  Require(!motion_restored.MotionEnabled(), "Motion preference was not persisted");
  restored.ClearBackground();
  Require(!restored.HasCustomBackground(), "Clear did not restore the default background");
  restored.SetGame(QString(), fallback);
  Require(!restored.SetBackgroundFile(QString::fromUtf8(ORBIT_FIXTURE_PATH), &error),
          "Background import was allowed without a game");
}
