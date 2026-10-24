// Copyright 2026 Dolphin Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DolphinQt/Home/HomeWidget.h"

#include <algorithm>

#include <QAbstractItemModel>
#include <QAction>
#include <QDir>
#include <QEasingCurve>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QImageReader>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLabel>
#include <QLinearGradient>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QStringList>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QVariantAnimation>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "DolphinQt/GameList/GameListModel.h"
#include "DolphinQt/Home/OrbitBackground.h"

namespace
{
constexpr int CARD_WIDTH = 192;
constexpr int CARD_HEIGHT = 168;

QLabel* MakeLabel(const QString& text, const char* name, QWidget* parent)
{
  auto* label = new QLabel(text, parent);
  label->setTextFormat(Qt::PlainText);
  label->setObjectName(QString::fromLatin1(name));
  return label;
}

QPushButton* MakeButton(const QString& text, const char* name, QWidget* parent)
{
  auto* button = new QPushButton(text, parent);
  button->setObjectName(QString::fromLatin1(name));
  button->setAutoDefault(true);
  button->setCursor(Qt::PointingHandCursor);
  return button;
}

void DrawArtwork(QPainter* painter, const QRectF& bounds, const QPixmap& pixmap)
{
  if (pixmap.isNull())
    return;

  const QSizeF size = QSizeF(pixmap.size()).scaled(bounds.size(), Qt::KeepAspectRatio);
  const QRectF target(bounds.x() + (bounds.width() - size.width()) / 2,
                      bounds.y() + (bounds.height() - size.height()) / 2, size.width(),
                      size.height());
  painter->drawPixmap(target, pixmap, QRectF(pixmap.rect()));
}

// Animations belong to the view, rather than to a widget or timer for every game.
class HomeGameView final : public QListView
{
public:
  explicit HomeGameView(QWidget* parent)
      : QListView(parent), m_selection_animation(this), m_hover_animation(this),
        m_scroll_animation(horizontalScrollBar(), "value", this)
  {
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    for (QVariantAnimation* animation : {&m_selection_animation, &m_hover_animation})
    {
      animation->setStartValue(0.0);
      animation->setEndValue(1.0);
      animation->setDuration(190);
      animation->setEasingCurve(QEasingCurve::OutCubic);
      connect(animation, &QVariantAnimation::valueChanged, viewport(),
              qOverload<>(&QWidget::update));
    }
    m_scroll_animation.setDuration(260);
    m_scroll_animation.setEasingCurve(QEasingCurve::OutCubic);
  }

  void ObserveSelection()
  {
    connect(selectionModel(), &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex& current, const QModelIndex& previous) {
              m_previous = previous;
              m_current = current;
              m_selection_animation.stop();
              if (m_motion_enabled)
                m_selection_animation.start();
              viewport()->update();
            });
  }

  void SetMotionEnabled(bool enabled)
  {
    m_motion_enabled = enabled;
    if (!enabled)
    {
      m_selection_animation.stop();
      m_hover_animation.stop();
      m_scroll_animation.stop();
      viewport()->update();
    }
  }

  qreal SelectionAmount(const QModelIndex& index, bool selected) const
  {
    const qreal amount = m_selection_animation.state() == QAbstractAnimation::Running ?
                             m_selection_animation.currentValue().toReal() :
                             1.0;
    if (index == m_current)
      return selected ? amount : 0.0;
    if (index == m_previous)
      return 1.0 - amount;
    return selected ? 1.0 : 0.0;
  }

  qreal HoverAmount(const QModelIndex& index) const
  {
    const qreal amount = m_hover_animation.state() == QAbstractAnimation::Running ?
                             m_hover_animation.currentValue().toReal() :
                             1.0;
    if (index == m_hovered)
      return amount;
    return index == m_previous_hovered ? 1.0 - amount : 0.0;
  }

  void SelectOffset(int offset)
  {
    if (model()->rowCount() == 0)
      return;
    const int row = std::clamp(currentIndex().row() + offset, 0, model()->rowCount() - 1);
    const QModelIndex index = model()->index(row, modelColumn());
    selectionModel()->setCurrentIndex(
        index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    scrollTo(index, QAbstractItemView::EnsureVisible);
  }

  void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override
  {
    const int start = horizontalScrollBar()->value();
    m_scroll_animation.stop();
    QListView::scrollTo(index, hint);
    const int target = horizontalScrollBar()->value();
    if (m_motion_enabled && isVisible() && start != target)
    {
      horizontalScrollBar()->setValue(start);
      AnimateScroll(target);
    }
  }

protected:
  void keyPressEvent(QKeyEvent* event) override
  {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
    {
      // GameList owns keyboard activation and its existing game-launch path.
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

  void mouseMoveEvent(QMouseEvent* event) override
  {
    SetHovered(indexAt(event->pos()));
    QListView::mouseMoveEvent(event);
  }

  void leaveEvent(QEvent* event) override
  {
    SetHovered({});
    QListView::leaveEvent(event);
  }

  void wheelEvent(QWheelEvent* event) override
  {
    int distance = event->pixelDelta().x();
    if (distance == 0)
      distance = event->pixelDelta().y();
    if (distance == 0)
      distance = event->angleDelta().x() != 0 ? event->angleDelta().x() : event->angleDelta().y();
    const int base = m_scroll_animation.state() == QAbstractAnimation::Running ?
                         m_scroll_animation.endValue().toInt() :
                         horizontalScrollBar()->value();
    AnimateScroll(base - distance);
    event->accept();
  }

private:
  void SetHovered(const QModelIndex& index)
  {
    if (m_hovered == index)
      return;
    m_previous_hovered = m_hovered;
    m_hovered = index;
    m_hover_animation.stop();
    if (m_motion_enabled)
      m_hover_animation.start();
    viewport()->update();
  }

  void AnimateScroll(int value)
  {
    m_scroll_animation.stop();
    value = std::clamp(value, horizontalScrollBar()->minimum(), horizontalScrollBar()->maximum());
    if (!m_motion_enabled)
    {
      horizontalScrollBar()->setValue(value);
      return;
    }
    m_scroll_animation.setStartValue(horizontalScrollBar()->value());
    m_scroll_animation.setEndValue(value);
    m_scroll_animation.start();
  }

  QPersistentModelIndex m_current;
  QPersistentModelIndex m_previous;
  QPersistentModelIndex m_hovered;
  QPersistentModelIndex m_previous_hovered;
  QVariantAnimation m_selection_animation;
  QVariantAnimation m_hover_animation;
  QPropertyAnimation m_scroll_animation;
  bool m_motion_enabled = true;
};

class HomeGameDelegate final : public QStyledItemDelegate
{
public:
  explicit HomeGameDelegate(HomeGameView* view) : QStyledItemDelegate(view), m_view(view) {}

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
    const bool selected = option.state.testFlag(QStyle::State_Selected);
    const bool focused = option.state.testFlag(QStyle::State_HasFocus);
    const qreal selection = m_view->SelectionAmount(index, selected);
    const qreal hover = m_view->HoverAmount(index);
    QRectF card = QRectF(option.rect).adjusted(6, 10, -6, -8);
    card.translate(0, -4 * selection - 2 * hover);

    // Soft animated glow stays inside the delegate's item rectangle.
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(94, 235, 219, static_cast<int>(22 * selection)));
    painter->drawRoundedRect(card.adjusted(-4, -4, 4, 4), 17, 17);
    painter->setBrush(
        QColor(20 + static_cast<int>(12 * hover), 31 + static_cast<int>(9 * hover), 47, 220));
    painter->drawRoundedRect(card, 13, 13);

    QPainterPath clip;
    clip.addRoundedRect(card, 13, 13);
    painter->save();
    painter->setClipPath(clip);
    const QRectF artwork(card.x(), card.y(), card.width(), card.height() - 30);
    DrawArtwork(painter, artwork, index.data(Qt::DecorationRole).value<QPixmap>());
    QLinearGradient shade(card.topLeft(), card.bottomLeft());
    shade.setColorAt(0.40, QColor(6, 13, 25, 0));
    shade.setColorAt(1.0, QColor(6, 13, 25, 245));
    painter->fillRect(card, shade);
    painter->restore();

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(QColor(224, 255, 250, static_cast<int>(28 + selection * 210)),
                         1.0 + selection));
    painter->drawRoundedRect(card, 13, 13);
    if (focused)
    {
      painter->setPen(QPen(QColor(QStringLiteral("#ffffff")), 1.5));
      painter->drawRoundedRect(card.adjusted(-3, -3, 3, 3), 16, 16);
    }

    QFont font = option.font;
    font.setPixelSize(13);
    font.setWeight(QFont::DemiBold);
    painter->setFont(font);
    painter->setPen(QColor(QStringLiteral("#f5f8ff")));
    const QRectF title(card.x() + 12, card.bottom() - 31, card.width() - 24, 24);
    painter->drawText(title, Qt::AlignLeft | Qt::AlignVCenter,
                      painter->fontMetrics().elidedText(index.data().toString(), Qt::ElideRight,
                                                       static_cast<int>(title.width())));
    if (selection > 0)
    {
      painter->setPen(Qt::NoPen);
      painter->setBrush(QColor(255, 161, 141, static_cast<int>(255 * selection)));
      painter->drawRoundedRect(QRectF(card.center().x() - 12, card.bottom() + 4, 24, 3), 1.5, 1.5);
    }
    painter->restore();
  }

private:
  HomeGameView* m_view;
};

class EmptyOrbit final : public QWidget
{
public:
  explicit EmptyOrbit(QWidget* parent) : QWidget(parent)
  {
    setFixedSize(190, 190);
    setAttribute(Qt::WA_TransparentForMouseEvents);
  }

protected:
  void paintEvent(QPaintEvent*) override
  {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(width() / 2.0, height() / 2.0);
    painter.rotate(-25);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(119, 224, 221, 42), 1));
    painter.drawEllipse(QRectF(-88, -88, 176, 176));
    painter.setPen(QPen(QColor(119, 224, 221, 130), 2));
    painter.drawEllipse(QRectF(-65, -65, 130, 130));
    painter.setPen(QPen(QColor(200, 240, 255, 55), 1));
    painter.drawEllipse(QRectF(-42, -42, 84, 84));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(QStringLiteral("#ffad95")));
    painter.drawEllipse(QPointF(0, -65), 9, 9);
    painter.setBrush(QColor(QStringLiteral("#c0faf0")));
    painter.drawEllipse(QPointF(0, 0), 5, 5);
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
    QWidget#OrbitHome QLabel { color: #f5f8ff; background: transparent; }
    QWidget#OrbitHome QLabel#OrbitEyebrow {
      color: #a4dcd7; font-size: 11px; font-weight: 600;
    }
    QWidget#OrbitHome QLabel#OrbitMetadata,
    QWidget#OrbitHome QLabel#OrbitDescription,
    QWidget#OrbitHome QLabel#OrbitCount,
    QWidget#OrbitHome QLabel#OrbitHint { color: #aebbd0; font-size: 12px; }
    QWidget#OrbitHome QLabel#OrbitTitle { font-weight: 700; }
    QWidget#OrbitHome QLabel#OrbitDescription { color: #c9d3e1; font-size: 14px; }
    QWidget#OrbitHome QLabel#OrbitSection { font-size: 14px; font-weight: 600; }
    QWidget#OrbitHome QLabel#OrbitEmptyTitle { font-size: 35px; font-weight: 700; }
    QWidget#OrbitHome QPushButton {
      color: #d9e4f4; background: rgba(18, 30, 48, 160);
      border: 1px solid rgba(174, 196, 221, 40); border-radius: 14px;
      padding: 7px 15px; font-size: 12px;
    }
    QWidget#OrbitHome QPushButton:hover {
      color: #ffffff; background: rgba(55, 78, 101, 190);
      border-color: rgba(200, 240, 238, 130);
    }
    QWidget#OrbitHome QPushButton:focus {
      border: 2px solid #b9fff0; padding: 6px 14px;
    }
    QWidget#OrbitHome QPushButton#OrbitPlay,
    QWidget#OrbitHome QPushButton#OrbitAddFolder {
      color: #0c1724; background: #c5fff0; border-color: #c5fff0;
      border-radius: 20px; padding: 12px 28px; font-size: 14px; font-weight: 700;
    }
    QWidget#OrbitHome QPushButton#OrbitPlay:hover,
    QWidget#OrbitHome QPushButton#OrbitAddFolder:hover {
      background: #f0fff9; border-color: #f0fff9;
    }
    QWidget#OrbitHome QPushButton#OrbitPlay:focus,
    QWidget#OrbitHome QPushButton#OrbitAddFolder:focus {
      border: 2px solid #ffad95; padding: 11px 27px;
    }
    QWidget#OrbitHome QPushButton:disabled {
      color: #77859a; background: rgba(26, 37, 54, 160); border-color: transparent;
    }
    QWidget#OrbitHome QPushButton#OrbitPrevious,
    QWidget#OrbitHome QPushButton#OrbitNext {
      border-radius: 14px; padding: 0; min-width: 28px; max-width: 28px;
      min-height: 28px; max-height: 28px; font-size: 20px;
    }
    QWidget#OrbitHome QPushButton#OrbitSettings {
      border-radius: 14px; padding: 0; min-width: 30px; max-width: 30px;
      min-height: 30px; max-height: 30px; font-size: 20px;
    }
    QWidget#OrbitHome QListView { background: transparent; border: none; outline: none; }
  )"));

  m_background = new OrbitBackground(this);
  m_background->setObjectName(QStringLiteral("OrbitBackground"));
  m_background->setGeometry(rect());
  m_background->lower();

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(34, 18, 34, 18);
  layout->setSpacing(10);
  auto* utilities = new QHBoxLayout;
  utilities->setSpacing(8);
  auto* eyebrow = MakeLabel(tr("TU ESPACIO  /  GAMECUBE + WII"), "OrbitEyebrow", this);
  QFont eyebrow_font = eyebrow->font();
  eyebrow_font.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
  eyebrow->setFont(eyebrow_font);
  utilities->addWidget(eyebrow);
  utilities->addStretch();
  m_customize = MakeButton(tr("Personalizar fondo"), "OrbitCustomizeBackground", this);
  m_customize->setToolTip(tr("Elige una imagen o GIF local para este juego."));
  utilities->addWidget(m_customize);
  auto* settings = MakeButton(QStringLiteral("⋯"), "OrbitSettings", this);
  settings->setAccessibleName(tr("Ajustes"));
  settings->setToolTip(tr("Ajustes generales"));
  utilities->addWidget(settings);
  layout->addLayout(utilities);
  connect(m_customize, &QPushButton::clicked, this, &HomeWidget::ShowBackgroundMenu);
  connect(settings, &QPushButton::clicked, this, &HomeWidget::OpenGeneralSettings);

  m_content = new QStackedWidget(this);
  m_content->setObjectName(QStringLiteral("OrbitContent"));
  layout->addWidget(m_content, 1);
  m_library = new QWidget(m_content);
  m_library->setObjectName(QStringLiteral("OrbitLibrary"));
  auto* library_layout = new QVBoxLayout(m_library);
  library_layout->setContentsMargins(0, 0, 0, 0);
  library_layout->setSpacing(7);

  m_hero = new QWidget(m_library);
  m_hero->setObjectName(QStringLiteral("OrbitHero"));
  auto* hero_layout = new QVBoxLayout(m_hero);
  hero_layout->setContentsMargins(0, 8, 0, 12);
  hero_layout->setSpacing(8);
  hero_layout->addStretch(1);
  m_title = MakeLabel(QString(), "OrbitTitle", m_hero);
  m_title->setWordWrap(true);
  m_title->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  hero_layout->addWidget(m_title);
  m_metadata = MakeLabel(QString(), "OrbitMetadata", m_hero);
  m_metadata->setWordWrap(true);
  m_metadata->setMaximumHeight(34);
  hero_layout->addWidget(m_metadata);
  m_description = MakeLabel(QString(), "OrbitDescription", m_hero);
  m_description->setWordWrap(true);
  m_description->setMaximumHeight(44);
  hero_layout->addWidget(m_description);
  auto* hero_actions = new QHBoxLayout;
  hero_actions->setContentsMargins(0, 8, 0, 0);
  hero_actions->setSpacing(15);
  m_play = MakeButton(tr("Jugar"), "OrbitPlay", m_hero);
  m_play->setMinimumWidth(150);
  m_play->setEnabled(false);
  hero_actions->addWidget(m_play, 0, Qt::AlignLeft);
  hero_actions->addWidget(
      MakeLabel(tr("A tu manera. A tu ritmo."), "OrbitHint", m_hero), 0, Qt::AlignLeft);
  hero_actions->addStretch();
  hero_layout->addLayout(hero_actions);
  hero_layout->addStretch(1);
  library_layout->addWidget(m_hero, 1);
  m_hero_opacity = new QGraphicsOpacityEffect(m_hero);
  m_hero_opacity->setOpacity(1.0);
  m_hero->setGraphicsEffect(m_hero_opacity);
  m_hero_animation = new QPropertyAnimation(m_hero_opacity, "opacity", this);
  m_hero_animation->setDuration(240);
  m_hero_animation->setEasingCurve(QEasingCurve::OutCubic);
  connect(m_play, &QPushButton::clicked, this, [this] {
    if (HasSelectedGame())
      emit GameSelected();
  });

  auto* library_header = new QHBoxLayout;
  library_header->setSpacing(9);
  library_header->addWidget(MakeLabel(tr("Tu colección"), "OrbitSection", m_library));
  m_count = MakeLabel(QString(), "OrbitCount", m_library);
  library_header->addWidget(m_count);
  library_header->addStretch();
  auto* previous = MakeButton(QStringLiteral("‹"), "OrbitPrevious", m_library);
  previous->setAccessibleName(tr("Juego anterior"));
  auto* next = MakeButton(QStringLiteral("›"), "OrbitNext", m_library);
  next->setAccessibleName(tr("Juego siguiente"));
  library_header->addWidget(previous);
  library_header->addWidget(next);
  library_layout->addLayout(library_header);

  auto* game_view = new HomeGameView(m_library);
  m_view = game_view;
  m_view->setObjectName(QStringLiteral("OrbitGameCarousel"));
  m_view->setAccessibleName(tr("Biblioteca de juegos"));
  m_view->setAccessibleDescription(tr("Usa las flechas para elegir un juego e Intro para jugar."));
  m_view->setModel(m_model);
  m_view->setModelColumn(0);
  m_view->setItemDelegate(new HomeGameDelegate(game_view));
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
  m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_view->setFixedHeight(CARD_HEIGHT + 8);
  game_view->ObserveSelection();
  library_layout->addWidget(m_view);
  connect(previous, &QPushButton::clicked, this, [game_view] { game_view->SelectOffset(-1); });
  connect(next, &QPushButton::clicked, this, [game_view] { game_view->SelectOffset(1); });
  library_layout->addWidget(
      MakeLabel(tr("←  →  Elegir                       ↵  Jugar"), "OrbitHint", m_library));
  m_content->addWidget(m_library);
  setFocusProxy(m_view);

  auto* empty = new QWidget(m_content);
  empty->setObjectName(QStringLiteral("OrbitEmpty"));
  auto* empty_layout = new QHBoxLayout(empty);
  empty_layout->setSpacing(42);
  empty_layout->addStretch();
  m_empty_emblem = new EmptyOrbit(empty);
  empty_layout->addWidget(m_empty_emblem);
  auto* empty_details = new QVBoxLayout;
  empty_details->setSpacing(18);
  empty_details->addStretch();
  empty_details->addWidget(MakeLabel(tr("UN UNIVERSO POR DESCUBRIR"), "OrbitEyebrow", empty));
  m_empty_title = MakeLabel(QString(), "OrbitEmptyTitle", empty);
  m_empty_title->setWordWrap(true);
  m_empty_title->setMinimumWidth(360);
  m_empty_title->setMaximumWidth(440);
  empty_details->addWidget(m_empty_title);
  m_empty_description = MakeLabel(QString(), "OrbitDescription", empty);
  m_empty_description->setWordWrap(true);
  m_empty_description->setMinimumWidth(360);
  m_empty_description->setMaximumWidth(420);
  empty_details->addWidget(m_empty_description);
  auto* add_folder = MakeButton(tr("Añadir carpeta"), "OrbitAddFolder", empty);
  empty_details->addWidget(add_folder, 0, Qt::AlignLeft);
  empty_details->addStretch();
  empty_layout->addLayout(empty_details);
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

void HomeWidget::SetBackgroundSettings(QSettings* settings)
{
  m_background->SetSettings(settings);
  UpdateMotionPreference();
  UpdateSelectedGame();
}

void HomeWidget::SetBackgroundPlaybackEnabled(bool enabled)
{
  m_background->SetPlaybackEnabled(enabled);
}

void HomeWidget::SetGameCountVisible(bool visible)
{
  m_count->setVisible(visible);
}

void HomeWidget::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  m_background->setGeometry(rect());
  m_background->lower();
  const bool compact = height() < 600;
  QFont title_font = m_title->font();
  title_font.setPixelSize(std::clamp(width() / 19, 40, 64));
  title_font.setWeight(QFont::Bold);
  m_title->setFont(title_font);
  m_title->setMaximumWidth(std::max(420, width() * 3 / 4));
  m_title->setMaximumHeight(compact ? 112 : 164);
  m_description->setVisible(!compact && !m_description->text().isEmpty());
  m_empty_emblem->setVisible(width() >= 800);
  const int empty_text_width = width() < 600 ? std::max(240, width() - 80) : 360;
  m_empty_title->setMinimumWidth(empty_text_width);
  m_empty_description->setMinimumWidth(empty_text_width);
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
  m_count->setText(count == 1 ? tr("·  1 juego") : tr("·  %1 juegos").arg(count));
  if (count == 0)
  {
    const bool has_games = m_model->sourceModel() && m_model->sourceModel()->rowCount() != 0;
    m_empty_title->setText(has_games ? tr("No hay coincidencias") :
                                      tr("Tu próxima aventura empieza aquí."));
    m_empty_description->setText(
        has_games ? tr("Cambia la búsqueda o los filtros para volver a tu colección.") :
                    tr("Añade una carpeta con tus juegos de GameCube o Wii. Nosotros ponemos "
                       "el espacio; tú eliges la aventura."));
    m_content->setCurrentIndex(1);
  }
  else
  {
    m_content->setCurrentWidget(m_library);
    if (!HasSelectedGame())
    {
      // Choosing an index must not take focus from the shell or customization controls.
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
  m_customize->setEnabled(has_game);
  if (!has_game)
  {
    m_title->setText(tr("Elige tu próxima aventura"));
    m_metadata->clear();
    m_description->clear();
    m_current_game_key.clear();
    m_background->SetGame({}, {});
    return;
  }

  const QModelIndex current = m_view->currentIndex();
  const QModelIndex source = m_model->mapToSource(current);
  const auto text = [this, &source](GameListModel::Column column) {
    return m_model->sourceModel()->index(source.row(), static_cast<int>(column)).data().toString();
  };
  const QString filename = text(GameListModel::Column::FileName);
  QString game_key =
      filename.isEmpty() ? text(GameListModel::Column::ID) :
                           QDir(text(GameListModel::Column::FilePath)).filePath(filename);
  if (game_key.isEmpty())
    game_key = current.data().toString();
  const bool changed = m_current_game_key != game_key;
  m_current_game_key = game_key;
  m_title->setText(current.data().toString());
  QStringList metadata;
  for (const auto column : {GameListModel::Column::Maker, GameListModel::Column::FileFormat,
                            GameListModel::Column::Size})
  {
    const QString value = text(column);
    if (!value.isEmpty())
      metadata.append(value);
  }
  m_metadata->setText(metadata.join(QStringLiteral("   ·   ")));
  m_description->setText(text(GameListModel::Column::Description));
  m_description->setVisible(height() >= 600 && !m_description->text().isEmpty());
  m_background->SetGame(game_key, current.data(Qt::DecorationRole).value<QPixmap>());
  if (changed && isVisible() && m_background->MotionEnabled())
  {
    m_hero_animation->stop();
    m_hero_animation->setStartValue(0.45);
    m_hero_animation->setEndValue(1.0);
    m_hero_animation->start();
  }
}

void HomeWidget::ShowBackgroundMenu()
{
  if (!HasSelectedGame())
    return;
  QMenu menu(this);
  menu.setObjectName(QStringLiteral("OrbitBackgroundMenu"));
  menu.setStyleSheet(QStringLiteral(R"(
    QMenu { color: #edf5ff; background: #152238; border: 1px solid #334a61;
            border-radius: 12px; padding: 7px; }
    QMenu::item { padding: 9px 24px 9px 12px; border-radius: 6px; }
    QMenu::item:selected { background: #2b4560; }
    QMenu::item:disabled { color: #6e8098; }
    QMenu::separator { height: 1px; background: #2c4057; margin: 5px 8px; }
  )"));
  QAction* choose = menu.addAction(tr("Elegir imagen o GIF…"));
  choose->setObjectName(QStringLiteral("OrbitChooseBackground"));
  QAction* remove = menu.addAction(tr("Quitar fondo personalizado"));
  remove->setObjectName(QStringLiteral("OrbitRemoveBackground"));
  remove->setEnabled(m_background->HasCustomBackground());
  menu.addSeparator();
  QAction* motion = menu.addAction(tr("Movimiento y transiciones"));
  motion->setObjectName(QStringLiteral("OrbitBackgroundMotion"));
  motion->setCheckable(true);
  motion->setChecked(m_background->MotionEnabled());
  QAction* action = menu.exec(m_customize->mapToGlobal(QPoint(0, m_customize->height() + 8)));
  if (action == choose)
  {
    QStringList patterns;
    const auto supported = QImageReader::supportedImageFormats();
    for (const QByteArray format : {QByteArray("gif"), QByteArray("png"), QByteArray("jpeg"),
                                  QByteArray("webp"), QByteArray("bmp")})
    {
      if (supported.contains(format))
      {
        patterns.append(QStringLiteral("*.%1").arg(QString::fromLatin1(format)));
        if (format == "jpeg")
          patterns.append(QStringLiteral("*.jpg"));
      }
    }
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Elige el fondo de este juego"), QString(),
        tr("Imágenes y GIF (%1)").arg(patterns.join(QLatin1Char(' '))));
    if (path.isEmpty())
      return;
    QString error;
    if (!m_background->SetBackgroundFile(path, &error))
      QMessageBox::warning(this, tr("No se pudo cargar el fondo"), error);
  }
  else if (action == remove)
  {
    m_background->ClearBackground();
  }
  else if (action == motion)
  {
    m_background->SetMotionEnabled(motion->isChecked());
    UpdateMotionPreference();
  }
}

void HomeWidget::UpdateMotionPreference()
{
  const bool enabled = m_background->MotionEnabled();
  static_cast<HomeGameView*>(m_view)->SetMotionEnabled(enabled);
  if (!enabled)
  {
    m_hero_animation->stop();
    m_hero_opacity->setOpacity(1.0);
  }
}
