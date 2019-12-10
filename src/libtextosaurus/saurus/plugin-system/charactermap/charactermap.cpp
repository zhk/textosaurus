// This file is distributed under GNU GPLv3 license. For full license text, see <project-git-repository-root-folder>/LICENSE.md.

#include "saurus/plugin-system/charactermap/charactermap.h"

#include "definitions/definitions.h"

#include <QApplication>
#include <QFontDatabase>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>
#include <QToolTip>

CharacterMap::CharacterMap(QWidget* parent)
  : QWidget(parent), m_columns(4), m_squareSize(0), m_characters(QList<CharacterInfo>()),
  m_selectedCharacter(-1) {
  setMouseTracking(true);
  setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

  m_font = qApp->font();
}

void CharacterMap::calculateSquareSize() {
  m_squareSize = width() / m_columns;
}

QSize CharacterMap::sizeHint() const {
  return QSize(1, (qCeil(m_characters.size() / (m_columns * 1.01)) + 1) * m_squareSize);
}

void CharacterMap::loadCharacters(const QList<CharacterInfo>& list) {
  m_characters = list;
  m_selectedCharacter = -1;

  adjustSize();
  update();
}

void CharacterMap::mouseMoveEvent(QMouseEvent* event) {
  QPoint widgetPosition = mapFromGlobal(event->globalPos());
  int idx = indexFromPoint(widgetPosition);

  if (idx >= 0 && idx < m_characters.size()) {
    CharacterInfo nfo = m_characters.at(idx);
    QString text = tr("<center><h1>%1</h1></center>"
                      "<p>HEX: %3 (UTF-16)</p>").arg(QString(nfo.m_character), nfo.m_description.toHtmlEscaped(),
                                                     QString::number(nfo.m_character.unicode(), 16));

    QToolTip::showText(event->globalPos(), text, this);
  }
}

void CharacterMap::mouseDoubleClickEvent(QMouseEvent* event) {
  if (event->button() == Qt::MouseButton::LeftButton) {
    if (isSelectedValidCharacter()) {
      emit characterSelected(m_characters.at(m_selectedCharacter).m_character);
    }
  }
  else {
    QWidget::mouseDoubleClickEvent(event);
  }
}

void CharacterMap::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::MouseButton::LeftButton) {
    m_selectedCharacter = indexFromPoint(event->pos());
    update();
  }
  else {
    QWidget::mousePressEvent(event);
  }
}

void CharacterMap::paintEvent(QPaintEvent* event) {
  Q_UNUSED(event)

  QPainter painter(this);

  m_font.setPixelSize(int(m_squareSize * 0.5));
  painter.setFont(m_font);

  int start_idx = indexFromPoint(event->region().boundingRect().topLeft());
  int stop_idx = indexFromPoint(event->region().boundingRect().bottomRight());
  int row = start_idx / m_columns;
  int column = start_idx % m_columns;

  if (stop_idx >= m_characters.size()) {
    stop_idx = m_characters.size() - 1;
  }

  for (int i = start_idx; i <= stop_idx; i++) {
    // Draw next character.
    const CharacterInfo& chr_info = m_characters.at(i);
    QRect cell_rect(column* m_squareSize, row* m_squareSize, m_squareSize, m_squareSize);

    // Draw cell background and border.
    painter.fillRect(cell_rect, m_selectedCharacter == i ? Qt::GlobalColor::lightGray : Qt::GlobalColor::white);
    painter.drawRect(cell_rect);

    QFontMetrics fnt_metrics(m_font);
    QRect char_rect = fnt_metrics.boundingRect(chr_info.m_character);

    // Draw character.
    painter.drawText(cell_rect.topLeft() + QPoint(int((m_squareSize - char_rect.width()) / 2.0),
                                                  char_rect.height() + int((m_squareSize - char_rect.height()) / 2.0)),
                     chr_info.m_character);

    if (cell_rect.width() > 50) {
      // Draw description, because cell is big enough.
    }

    if (column++ == m_columns - 1) {
      // End of row.
      row++;
      column = 0;
    }
  }
}

void CharacterMap::resizeEvent(QResizeEvent* event) {
  Q_UNUSED(event)

  calculateSquareSize();
}

bool CharacterMap::isSelectedValidCharacter() const {
  return m_selectedCharacter >=0 && m_selectedCharacter < m_characters.size();
}

int CharacterMap::indexFromPoint(const QPoint& pt) const {
  int row = pt.y() / m_squareSize;
  int col = pt.x() / m_squareSize;

  return (row * m_columns) + col;
}