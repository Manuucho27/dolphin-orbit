// Standalone smoke test of the real HomeWidget; every library entry below is synthetic.
#include <iostream>
#include <stdexcept>

#include <QApplication>
#include <QCoreApplication>
#include <QEventLoop>
#include <QImage>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLinearGradient>
#include <QListView>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QVBoxLayout>

#include "DolphinQt/GameList/GameListModel.h"
#include "DolphinQt/Home/HomeWidget.h"

namespace
{
using Column = GameListModel::Column;

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

class SyntheticGridProxy final : public QSortFilterProxyModel
{
public:
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
  {
    if (!index.isValid())
      return {};
    if (role == Qt::DisplayRole || role == Qt::AccessibleTextRole)
    {
      return sourceModel()
          ->index(mapToSource(index).row(), static_cast<int>(Column::Title))
          .data();
    }
    if (role == Qt::DecorationRole)
    {
      // Generated artwork makes the preview self-contained and contains no game assets.
      QPixmap pixmap(320, 220);
      pixmap.fill(Qt::transparent);
      QPainter painter(&pixmap);
      painter.setRenderHint(QPainter::Antialiasing);
      const QColor colors[] = {QColor("#369bb0"), QColor("#e98872"), QColor("#8692d7"),
                               QColor("#65b99c")};
      const QColor color = colors[mapToSource(index).row() % 4];
      QLinearGradient gradient(0, 0, 320, 220);
      gradient.setColorAt(0, color);
      gradient.setColorAt(1, QColor("#15253e"));
      painter.fillRect(pixmap.rect(), gradient);
      painter.setPen(QPen(QColor(255, 255, 255, 100), 2));
      painter.setBrush(Qt::NoBrush);
      painter.drawEllipse(QPoint(230, 70), 115, 115);
      painter.drawEllipse(QPoint(230, 70), 86, 86);
      painter.drawEllipse(QPoint(230, 70), 55, 55);
      painter.setPen(Qt::white);
      QFont font = painter.font();
      font.setPixelSize(25);
      font.setBold(true);
      painter.setFont(font);
      painter.drawText(QRect(22, 125, 280, 65), Qt::AlignLeft | Qt::TextWordWrap,
                       data(index, Qt::DisplayRole).toString());
      font.setPixelSize(12);
      font.setBold(false);
      painter.setFont(font);
      painter.drawText(22, 207, QStringLiteral("SYNTHETIC UI PREVIEW"));
      return pixmap;
    }
    return {};
  }
};

class HostWidget final : public QWidget
{
public:
  int forwarded_enters = 0;

protected:
  void keyPressEvent(QKeyEvent* event) override
  {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
      ++forwarded_enters;
      event->accept();
      return;
    }
    QWidget::keyPressEvent(event);
  }
};

void AppendGame(QStandardItemModel* source, const QString& title)
{
  QList<QStandardItem*> row;
  for (int i = 0; i < static_cast<int>(Column::Count); ++i)
    row.append(new QStandardItem);
  row[static_cast<int>(Column::Title)]->setText(title);
  row[static_cast<int>(Column::Maker)]->setText(QStringLiteral("ORBIT UI preview"));
  row[static_cast<int>(Column::FileFormat)]->setText(QStringLiteral("Synthetic demo"));
  row[static_cast<int>(Column::Size)]->setText(QStringLiteral("No game file"));
  row[static_cast<int>(Column::Description)]->setText(
      QStringLiteral("An original sample entry for reviewing the native DolphinQt interface."));
  source->appendRow(row);
  Pump();
}

QPushButton* FindButton(HomeWidget* home, const QString& text)
{
  for (QPushButton* button : home->findChildren<QPushButton*>())
  {
    if (button->text() == text)
      return button;
  }
  throw std::runtime_error("Expected button was not found");
}

void PressEnter(QWidget* target)
{
  QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
  QApplication::sendEvent(target, &press);
  QKeyEvent release(QEvent::KeyRelease, Qt::Key_Return, Qt::NoModifier);
  QApplication::sendEvent(target, &release);
  Pump();
}
}  // namespace

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  QPalette light_palette = app.palette();
  light_palette.setColor(QPalette::Window, QColor(QStringLiteral("#ffffff")));
  app.setPalette(light_palette);
  QStandardItemModel source(0, static_cast<int>(Column::Count));
  SyntheticGridProxy proxy;
  proxy.setSourceModel(&source);
  proxy.setFilterKeyColumn(static_cast<int>(Column::Title));
  HostWidget host;
  auto* layout = new QVBoxLayout(&host);
  layout->setContentsMargins(0, 0, 0, 0);
  auto* home = new HomeWidget(&proxy, &host);
  layout->addWidget(home);
  host.resize(1100, 720);
  host.show();
  host.activateWindow();
  Pump();

  try
  {
    Require(home->grab().toImage().pixelColor(2, 2) == QColor(QStringLiteral("#0a1323")),
            "Home background did not remain dark with a light application palette");
    auto* view = home->GetView();
    auto* play = FindButton(home, QStringLiteral("Play"));
    auto* settings = FindButton(home, QStringLiteral("Settings"));
    auto* add_folder = FindButton(home, QStringLiteral("Add game folder"));
    auto* title = home->findChild<QLabel*>(QStringLiteral("OrbitTitle"));
    auto* metadata = home->findChild<QLabel*>(QStringLiteral("OrbitMetadata"));
    auto* description = home->findChild<QLabel*>(QStringLiteral("OrbitDescription"));
    int game_events = 0;
    int settings_events = 0;
    int folder_events = 0;
    QObject::connect(home, &HomeWidget::GameSelected, [&] { ++game_events; });
    QObject::connect(home, &HomeWidget::OpenGeneralSettings, [&] { ++settings_events; });
    QObject::connect(home, &HomeWidget::OpenFolderRequested, [&] { ++folder_events; });

    Require(!play->isEnabled(), "Empty library enabled Play");
    Require(add_folder->isVisible(), "Empty state does not offer Add game folder");
    Require(!view->currentIndex().isValid(), "Empty library has a current game");
    Require(home->grab().save(QString::fromUtf8(ORBIT_EMPTY_PREVIEW_PATH)),
            "Failed to save empty preview");
    add_folder->setFocus();
    Pump();
    PressEnter(add_folder);
    Require(folder_events == 1, "Enter did not activate Add game folder exactly once");
    Require(host.forwarded_enters == 0, "Add game folder leaked Enter to the host");

    AppendGame(&source, QStringLiteral("Nebula Circuit"));
    AppendGame(&source, QStringLiteral("Coral Coast"));
    Require(play->isEnabled(), "Inserted game did not enable Play");
    Require(view->currentIndex().row() == 0, "First inserted game was not selected");
    Require(view->selectionMode() == QAbstractItemView::SingleSelection,
            "Home view is not single selection");

    view->selectionModel()->setCurrentIndex(proxy.index(1, 0),
                                            QItemSelectionModel::ClearAndSelect |
                                                QItemSelectionModel::Rows);
    Pump();
    Require(title->text() == QStringLiteral("Coral Coast"), "Hero did not follow selection");
    play->click();
    Require(game_events == 1, "Play did not emit GameSelected exactly once");
    play->setFocus();
    Pump();
    PressEnter(play);
    Require(game_events == 2, "Enter did not activate Play exactly once");
    Require(host.forwarded_enters == 0, "Play leaked Enter to the host");

    settings->setFocus();
    Pump();
    Require(settings->hasFocus(), "Settings could not receive focus");
    PressEnter(settings);
    Require(settings_events == 1, "Enter did not activate Settings exactly once");
    Require(game_events == 2, "Enter in Settings activated a game");
    Require(host.forwarded_enters == 0, "Settings leaked Enter to the host");

    source.setData(source.index(1, static_cast<int>(Column::Title)),
                   QStringLiteral("<b>Literal title</b>"));
    source.setData(source.index(1, static_cast<int>(Column::Maker)),
                   QStringLiteral("<img src='literal'>"));
    source.setData(source.index(1, static_cast<int>(Column::Description)),
                   QStringLiteral("<a href='literal'>Literal description</a>"));
    Pump();
    Require(title->textFormat() == Qt::PlainText && metadata->textFormat() == Qt::PlainText &&
                description->textFormat() == Qt::PlainText,
            "Game metadata is interpreted as rich text");
    Require(title->text() == QStringLiteral("<b>Literal title</b>"),
            "Model dataChanged did not refresh the hero");
    Require(settings->hasFocus(), "Metadata refresh stole focus from Settings");
    AppendGame(&source, QStringLiteral("Lunar Grove"));
    Require(settings->hasFocus(), "Row insertion stole focus from Settings");
    source.sort(static_cast<int>(Column::Title), Qt::DescendingOrder);
    Pump();
    Require(settings->hasFocus(), "Layout change stole focus from Settings");

    proxy.setFilterFixedString(QStringLiteral("no synthetic title matches this filter"));
    Pump();
    Require(proxy.rowCount() == 0 && !play->isEnabled(), "Filter-to-zero did not disable Play");
    Require(home->findChild<QLabel*>(QStringLiteral("OrbitEmptyTitle"))->text() ==
                QStringLiteral("No matching games"),
            "Filtered library did not show the no-matches state");
    Require(settings->hasFocus(), "Filtering to zero stole focus from Settings");
    proxy.setFilterFixedString(QString());
    Pump();
    Require(view->currentIndex().isValid() && play->isEnabled(),
            "Clearing filter did not restore a usable selection");
    Require(settings->hasFocus(), "Restoring filtered rows stole focus from Settings");

    const int selected_row = proxy.mapToSource(view->currentIndex()).row();
    source.removeRow(selected_row);
    Pump();
    Require(view->currentIndex().isValid() && play->isEnabled(),
            "Removing selected row did not recover a valid selection");
    Require(settings->hasFocus(), "Removing a row stole focus from Settings");
    view->clearSelection();
    Pump();
    Require(!play->isEnabled(), "Clearing selection left Play enabled");
    play->click();
    Require(game_events == 2, "Disabled Play emitted GameSelected");

    source.clear();
    source.setColumnCount(static_cast<int>(Column::Count));
    Pump();
    Require(!play->isEnabled() && !view->currentIndex().isValid(),
            "Model reset left a stale playable selection");
    AppendGame(&source, QStringLiteral("Nebula Circuit"));
    AppendGame(&source, QStringLiteral("Coral Coast"));
    AppendGame(&source, QStringLiteral("Lunar Grove"));
    AppendGame(&source, QStringLiteral("Starline Rally"));
    Require(settings->hasFocus(), "Reset/repopulation stole focus from Settings");
    view->setFocus();
    Pump();
    PressEnter(view);
    Require(host.forwarded_enters == 1, "Home view did not delegate Enter to GameList's parent");
    Require(game_events == 2, "Home view duplicated parent keyboard activation");

    Pump();
    Require(home->grab().save(QString::fromUtf8(ORBIT_PREVIEW_PATH)),
            "Failed to save library preview");
    std::cout << "PASS: empty/insert/select/filter/remove/reset/plain-text/Play/Enter/focus; "
                 "saved synthetic previews\n";
    return 0;
  }
  catch (const std::exception& error)
  {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
