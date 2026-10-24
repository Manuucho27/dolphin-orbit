// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Home/HomeWidget.h"

#include <QAbstractItemModel>
#include <QFrame>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLinearGradient>
#include <QListView>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStringList>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

#include "DolphinQt/GameList/GameListModel.h"

namespace
{
constexpr int CARD_WIDTH = 176;
constexpr int CARD_HEIGHT = 184;

QLabel* MakeLabel(const QString& text, const char* name, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  label->setTextFormat(Qt::PlainText);
  label->setObjectName(QString::fromLatin1(name));
  return label;
}

void DrawArtwork(QPainter* painter, const QRect& bounds, const QPixmap& pixmap)
{
  if (pixmap.isNull())
    return;

  const QSize size = pixmap.size().scaled(bounds.size(), Qt::KeepAspectRatio);
  const QRect target(bounds.x() + (bounds.width() - size.width()) / 2,
                     bounds.y() + (bounds.height() - size.height()) / 2, size.width(),
                     size.height());
  painter->drawPixmap(target, pixmap);
}

class HomeGameDelegate final : public QStyledItemDelegate
{
public:
  explicit HomeGameDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

  QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
  {
    return QSize(CARD_WIDTH, CARD_HEIGHT);
  }

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override
  {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);

    const QRect card = option.rect.adjusted(5, 5, -5, -5);
    const bool selected = option.state.testFlag(QStyle::State_Selected);
    const bool focused = option.state.testFlag(QStyle::State_HasFocus);
    painter->setPen(QPen(selected ? QColor(QStringLiteral("#63e4e5")) :
                                   QColor(QStringLiteral("#26364e")),
                         selected ? 2 : 1));
    painter->setBrush(QColor(QStringLiteral("#142238")));
    painter->drawRoundedRect(card, 12, 12);

    const QRect artwork(card.x() + 12, card.y() + 12, card.width() - 24, 108);
    QLinearGradient gradient(artwork.topLeft(), artwork.bottomRight());
    gradient.setColorAt(0, QColor(QStringLiteral("#213d58")));
    gradient.setColorAt(1, QColor(QStringLiteral("#152036")));
    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawRoundedRect(artwork, 6, 6);
    DrawArtwork(painter, artwork.adjusted(4, 4, -4, -4),
                index.data(Qt::DecorationRole).value<QPixmap>());

    QFont font = option.font;
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(QColor(QStringLiteral("#f4f7fc")));
    const QRect title(card.x() + 12, artwork.bottom() + 10, card.width() - 24, 22);
    painter->drawText(title, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(index.data().toString(), Qt::ElideRight,
                                                       title.width()));

    if (selected)
    {
      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(QStringLiteral("#ff927f")));
      painter->drawRoundedRect(QRect(card.x() + 12, card.bottom() - 13, 28, 3), 1.5, 1.5);
    }
    if (focused)
    {
      painter->setPen(QPen(QColor(QStringLiteral("#f4f7fc")), 1, Qt::DashLine));
      painter->setBrush(Qt::NoBrush);
      painter->drawRoundedRect(card.adjusted(4, 4, -4, -4), 8, 8);
    }
    painter->restore();
  }
};

class HomeGameView final : public QListView
{
public:
  explicit HomeGameView(QWidget* parent) : QListView(parent) {}

protected:
  void keyPressEvent(QKeyEvent* event) override
  {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
      // Let GameList own keyboard activation, just as it owns the launch path for the other views.
      event->ignore();
      return;
    }
    QListView::keyPressEvent(event);
  }

  void mouseDoubleClickEvent(QMouseEvent* event) override
  {
    if (event->button() == Qt::LeftButton)
      QListView::mouseDoubleClickEvent(event);
  }
};
}  // namespace

HomeWidget::HomeWidget(QSortFilterProxyModel* model, QWidget* parent)
    : QWidget(parent), m_model(model)
{
  Q_ASSERT(m_model);
  setObjectName(QStringLiteral("OrbitHome"));
  setAttribute(Qt::WA_StyledBackground, true);
  setStyleSheet(QStringLiteral(R"(
    QWidget#OrbitHome { background: #0a1323; }
    QWidget#OrbitHome QLabel { color: #f4f7fc; background: transparent; }
    QWidget#OrbitHome QLabel#OrbitBrand { color: #63e4e5; font-size: 22px; font-weight: 700; }
    QWidget#OrbitHome QLabel#OrbitSubtitle,
    QWidget#OrbitHome QLabel#OrbitMetadata,
    QWidget#OrbitHome QLabel#OrbitDescription,
    QWidget#OrbitHome QLabel#OrbitCount,
    QWidget#OrbitHome QLabel#OrbitHint { color: #acbad0; }
    QWidget#OrbitHome QLabel#OrbitTitle { font-size: 26px; font-weight: 700; }
    QWidget#OrbitHome QLabel#OrbitSection,
    QWidget#OrbitHome QLabel#OrbitEmptyTitle { font-size: 18px; font-weight: 600; }
    QWidget#OrbitHome QFrame#OrbitHero {
      border: 1px solid #30465e; border-radius: 18px;
      background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1,
                                  stop: 0 #1c3550, stop: 1 #122037);
    }
    QWidget#OrbitHome QPushButton {
      color: #f4f7fc; background: #17263d; border: 1px solid #364960;
      border-radius: 9px; padding: 8px 14px;
    }
    QWidget#OrbitHome QPushButton:hover { background: #263d59; }
    QWidget#OrbitHome QPushButton:focus { border: 2px solid #63e4e5; padding: 7px 13px; }
    QWidget#OrbitHome QPushButton#OrbitPlay {
      color: #0a1323; background: #63e4e5; border-color: #63e4e5; font-weight: 700;
    }
    QWidget#OrbitHome QPushButton#OrbitPlay:hover { background: #a0f0ed; }
    QWidget#OrbitHome QPushButton#OrbitPlay:disabled {
      color: #8090a8; background: #26364e; border-color: #26364e;
    }
    QWidget#OrbitHome QListView { background: transparent; border: none; outline: none; }
    QWidget#OrbitHome QScrollBar:horizontal {
      background: #142238; height: 10px; border-radius: 5px;
    }
    QWidget#OrbitHome QScrollBar::handle:horizontal {
      background: #456481; min-width: 32px; border-radius: 5px;
    }
    QWidget#OrbitHome QScrollBar::add-line:horizontal,
    QWidget#OrbitHome QScrollBar::sub-line:horizontal { width: 0; }
  )"));

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(24, 16, 24, 16);
  layout->setSpacing(12);

  auto* header = new QHBoxLayout;
  header->setSpacing(10);
  header->addWidget(MakeLabel(QStringLiteral("ORBIT"), "OrbitBrand", this));
  header->addWidget(MakeLabel(tr("Dolphin / Home"), "OrbitSubtitle", this));
  header->addStretch();
  auto* controllers = new QPushButton(tr("Controllers"), this);
  auto* graphics = new QPushButton(tr("Graphics"), this);
  auto* settings = new QPushButton(tr("Settings"), this);
  for (QPushButton* button : {controllers, graphics, settings})
    button->setAutoDefault(true);
  header->addWidget(controllers);
  header->addWidget(graphics);
  header->addWidget(settings);
  layout->addLayout(header);
  connect(controllers, &QPushButton::clicked, this, &HomeWidget::OpenControllerSettings);
  connect(graphics, &QPushButton::clicked, this, &HomeWidget::OpenGraphicsSettings);
  connect(settings, &QPushButton::clicked, this, &HomeWidget::OpenGeneralSettings);

  m_content = new QStackedWidget(this);
  layout->addWidget(m_content, 1);
  m_library = new QWidget(m_content);
  auto* library_layout = new QVBoxLayout(m_library);
  library_layout->setContentsMargins(0, 0, 0, 0);
  library_layout->setSpacing(10);

  auto* hero = new QFrame(m_library);
  hero->setObjectName(QStringLiteral("OrbitHero"));
  hero->setMinimumHeight(150);
  auto* hero_layout = new QHBoxLayout(hero);
  hero_layout->setContentsMargins(22, 16, 22, 16);
  hero_layout->setSpacing(24);
  auto* details = new QVBoxLayout;
  details->setSpacing(6);
  details->addWidget(MakeLabel(tr("READY WHEN YOU ARE"), "OrbitSubtitle", hero));
  m_title = MakeLabel(QString(), "OrbitTitle", hero);
  m_title->setWordWrap(true);
  m_title->setMaximumHeight(72);
  details->addWidget(m_title);
  m_metadata = MakeLabel(QString(), "OrbitMetadata", hero);
  m_metadata->setWordWrap(true);
  details->addWidget(m_metadata);
  m_description = MakeLabel(QString(), "OrbitDescription", hero);
  m_description->setWordWrap(true);
  m_description->setMaximumHeight(40);
  details->addWidget(m_description);
  details->addStretch();
  m_play = new QPushButton(tr("Play"), hero);
  m_play->setAutoDefault(true);
  m_play->setObjectName(QStringLiteral("OrbitPlay"));
  m_play->setMinimumWidth(120);
  m_play->setEnabled(false);
  details->addWidget(m_play, 0, Qt::AlignLeft);
  hero_layout->addLayout(details, 1);
  m_artwork = MakeLabel(QString(), "OrbitArtwork", hero);
  m_artwork->setAlignment(Qt::AlignCenter);
  m_artwork->setFixedSize(160, 112);
  hero_layout->addWidget(m_artwork);
  library_layout->addWidget(hero, 1);
  connect(m_play, &QPushButton::clicked, this, [this] {
    if (HasSelectedGame())
      emit GameSelected();
  });

  auto* library_header = new QHBoxLayout;
  library_header->addWidget(MakeLabel(tr("Your library"), "OrbitSection", m_library));
  library_header->addStretch();
  m_count = MakeLabel(QString(), "OrbitCount", m_library);
  library_header->addWidget(m_count);
  library_layout->addLayout(library_header);

  m_view = new HomeGameView(m_library);
  m_view->setAccessibleName(tr("Game library"));
  m_view->setAccessibleDescription(tr("Use the arrow keys to choose a game and Enter to play."));
  m_view->setModel(m_model);
  m_view->setModelColumn(0);
  m_view->setItemDelegate(new HomeGameDelegate(m_view));
  m_view->setViewMode(QListView::IconMode);
  m_view->setFlow(QListView::LeftToRight);
  m_view->setWrapping(false);
  m_view->setMovement(QListView::Static);
  m_view->setResizeMode(QListView::Adjust);
  m_view->setGridSize(QSize(CARD_WIDTH, CARD_HEIGHT));
  m_view->setUniformItemSizes(true);
  m_view->setSelectionMode(QAbstractItemView::SingleSelection);
  m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_view->setTabKeyNavigation(false);
  m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_view->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_view->setFixedHeight(CARD_HEIGHT + 20);
  library_layout->addWidget(m_view);
  library_layout->addWidget(
      MakeLabel(tr("Arrow keys to browse  /  Enter to play"), "OrbitHint", m_library));
  m_content->addWidget(m_library);
  setFocusProxy(m_view);

  auto* empty = new QWidget(m_content);
  auto* empty_layout = new QVBoxLayout(empty);
  empty_layout->setSpacing(16);
  empty_layout->addStretch();
  m_empty_title = MakeLabel(QString(), "OrbitEmptyTitle", empty);
  m_empty_title->setAlignment(Qt::AlignCenter);
  empty_layout->addWidget(m_empty_title);
  m_empty_description = MakeLabel(QString(), "OrbitDescription", empty);
  m_empty_description->setWordWrap(true);
  m_empty_description->setAlignment(Qt::AlignCenter);
  empty_layout->addWidget(m_empty_description);
  auto* add_folder = new QPushButton(tr("Add game folder"), empty);
  add_folder->setAutoDefault(true);
  empty_layout->addWidget(add_folder, 0, Qt::AlignHCenter);
  empty_layout->addStretch();
  m_content->addWidget(empty);
  connect(add_folder, &QPushButton::clicked, this, &HomeWidget::OpenFolderRequested);

  connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged, this,
          &HomeWidget::UpdateSelectedGame);
  connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged, this,
          &HomeWidget::UpdateSelectedGame);
  connect(m_view, &QListView::doubleClicked, this, [this] {
    if (HasSelectedGame())
      emit GameSelected();
  });
  connect(m_model, &QAbstractItemModel::modelReset, this, &HomeWidget::RefreshLibrary);
  connect(m_model, &QAbstractItemModel::rowsInserted, this, &HomeWidget::RefreshLibrary);
  connect(m_model, &QAbstractItemModel::rowsRemoved, this, &HomeWidget::RefreshLibrary);
  connect(m_model, &QAbstractItemModel::layoutChanged, this, &HomeWidget::RefreshLibrary);
  connect(m_model, &QAbstractItemModel::dataChanged, this, &HomeWidget::UpdateSelectedGame);
  RefreshLibrary();
}

bool HomeWidget::HasSelectedGame() const
{
  const QModelIndex index = m_view->currentIndex();
  return index.isValid() && index.model() == m_model &&
         m_view->selectionModel()->isSelected(index);
}

void HomeWidget::RefreshLibrary()
{
  const int count = m_model->rowCount();
  m_count->setText(tr("%n game(s)", nullptr, count));
  if (count == 0)
  {
    const bool has_games = m_model->sourceModel() && m_model->sourceModel()->rowCount() != 0;
    m_empty_title->setText(has_games ? tr("No matching games") :
                                      tr("Your next adventure starts here"));
    m_empty_description->setText(
        has_games ? tr("Change the search or filters to see your games.") :
                    tr("Add a folder containing your GameCube or Wii games to build your "
                       "library."));
    m_content->setCurrentIndex(1);
  }
  else
  {
    m_content->setCurrentWidget(m_library);
    if (!HasSelectedGame())
    {
      // Setting a current index must not take keyboard focus from the header's settings buttons.
      const QModelIndex current = m_view->currentIndex();
      m_view->selectionModel()->setCurrentIndex(
          current.isValid() ? current : m_model->index(0, 0),
          QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    }
  }
  UpdateSelectedGame();
}

void HomeWidget::UpdateSelectedGame()
{
  const bool has_game = HasSelectedGame();
  m_play->setEnabled(has_game);
  if (!has_game)
  {
    m_title->setText(tr("Choose a game"));
    m_metadata->clear();
    m_description->clear();
    m_artwork->clear();
    return;
  }

  const QModelIndex current = m_view->currentIndex();
  const QModelIndex source = m_model->mapToSource(current);
  const auto text = [this, &source](GameListModel::Column column) {
    return m_model->sourceModel()->index(source.row(), static_cast<int>(column)).data().toString();
  };
  m_title->setText(current.data().toString());
  QStringList metadata;
  for (const auto column : {GameListModel::Column::Maker, GameListModel::Column::FileFormat,
                            GameListModel::Column::Size})
  {
    const QString value = text(column);
    if (!value.isEmpty())
      metadata.append(value);
  }
  m_metadata->setText(metadata.join(QStringLiteral(" / ")));
  m_description->setText(text(GameListModel::Column::Description));
  const QPixmap artwork = current.data(Qt::DecorationRole).value<QPixmap>();
  const qreal ratio = m_artwork->devicePixelRatioF();
  QPixmap scaled = artwork.scaled(m_artwork->size() * ratio, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
  scaled.setDevicePixelRatio(ratio);
  m_artwork->setPixmap(scaled);
}
