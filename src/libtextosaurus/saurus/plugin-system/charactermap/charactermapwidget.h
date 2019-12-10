// This file is distributed under GNU GPLv3 license. For full license text, see <project-git-repository-root-folder>/LICENSE.md.

#ifndef CHARACTERMAPWIDGET_H
#define CHARACTERMAPWIDGET_H

#include <QWidget>

#include "saurus/plugin-system/charactermap/charactermap.h"

class CharacterMap;
class QComboBox;
class QLineEdit;
class QScrollArea;

struct CharacterCategory {
  int m_from;
  int m_to;

  CharacterCategory() = default;
  CharacterCategory(int from, int to) : m_from(from), m_to(to) {}

};

Q_DECLARE_METATYPE(CharacterCategory)

class CharacterMapWidget : public QWidget {
  Q_OBJECT

  public:
    explicit CharacterMapWidget(QWidget* parent = nullptr);
    virtual ~CharacterMapWidget() = default;

    CharacterMap* map() const;

  private slots:
    void updateVisibleCharacters();

  private:
    QList<CharacterInfo> charactersForCategory(const CharacterCategory& cat) const;
    void setupUi();
    void loadCategories();
    void loadCharacters();

  private:
    QScrollArea* m_scrollForMap;
    CharacterMap* m_map;
    QComboBox* m_cmbPlane;
    QLineEdit* m_txtSearch;

    QList<CharacterInfo> m_allCharacters;
};

#endif // CHARACTERMAPWIDGET_H