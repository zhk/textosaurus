// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILESYSTEMSIDEBAR_H
#define FILESYSTEMSIDEBAR_H

#include "src/gui/docks/dockwidget.h"

class QFileSystemModel;

class FilesystemSidebar : public DockWidget {
  Q_OBJECT

  public:
    explicit FilesystemSidebar(QWidget* parent = nullptr);

    Qt::DockWidgetArea initialArea() const;
    bool initiallyVisible() const;
    int initialWidth() const;

  public slots:
    void load();

  private:
    QFileSystemModel* m_fsModel;
};

#endif // FILESYSTEMSIDEBAR_H