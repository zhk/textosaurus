// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/textapplication.h"

#include "gui/dialogs/formmain.h"
#include "gui/messagebox.h"
#include "gui/tabwidget.h"
#include "gui/texteditor.h"
#include "gui/toolbar.h"
#include "miscellaneous/textfactory.h"

#include <QFileDialog>
#include <QTextCodec>

TextApplication::TextApplication(QObject* parent) : QObject(parent) {}

TextEditor* TextApplication::currentEditor() const {
  return m_tabWidget->textEditorAt(m_tabWidget->currentIndex());
}

QList<TextEditor*> TextApplication::editors() const {
  QList<TextEditor*> editors;

  if (m_tabWidget != nullptr) {
    for (int i = 0; i < m_tabWidget->count(); i++) {
      TextEditor* edit = m_tabWidget->textEditorAt(i);

      if (edit != nullptr) {
        editors.append(edit);
      }
    }
  }

  return editors;
}

bool TextApplication::anyModifiedEditor() const {
  foreach (const TextEditor* editor, editors()) {
    if (editor->isModified()) {
      return true;
    }
  }

  return false;
}

void TextApplication::loadTextEditorFromFile(const QString& file_path, const QString& encoding) {
  QFile file(file_path);

  if (!file.exists()) {
    QMessageBox::critical(m_mainForm, tr("Cannot open file"),
                          tr("File '%1' does not exist and cannot be opened.").arg(QDir::toNativeSeparators(file_path)));
    return;
  }

  if (file.size() >= MAX_TEXT_FILE_SIZE) {
    QMessageBox::critical(m_mainForm, tr("Cannot open file"),
                          tr("File '%1' too big. %2 can only open files smaller than %3 MB.").arg(QDir::toNativeSeparators(file_path),
                                                                                                  QSL(APP_NAME),
                                                                                                  QString::number(MAX_TEXT_FILE_SIZE /
                                                                                                                  1000000.0)));
    return;
  }

  if (!file.open(QIODevice::OpenModeFlag::ReadOnly)) {
    QMessageBox::critical(m_mainForm, tr("Cannot read file"),
                          tr("File '%1' cannot be opened for reading. Insufficient permissions.").arg(QDir::toNativeSeparators(file_path)));
    return;
  }

  if (encoding != QSL(DEFAULT_TEXT_FILE_ENCODING) && file.size() > BIG_TEXT_FILE_SIZE) {
    if (MessageBox::show(m_mainForm, QMessageBox::Question, tr("Opening big file"),
                         tr("You want to open big text file in encoding which is different from %1. This operation "
                            "might take quite some time.").arg(QSL(DEFAULT_TEXT_FILE_ENCODING)),
                         tr("Do you really want to open the file?"),
                         file.fileName(), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
      return;
    }
  }

  TextEditor* new_editor = addEmptyTextEditor();

  if (new_editor != nullptr) {
    new_editor->loadFromFile(file, encoding);
    m_tabWidget->setCurrentWidget(new_editor);
  }
}

TextEditor* TextApplication::addEmptyTextEditor() {
  TextEditor* editor = new TextEditor(m_tabWidget);

  m_tabWidget->addTab(editor, qApp->icons()->fromTheme(QSL("text-plain")), tr("New text file"), TabBar::TabType::TextEditor);
  connect(editor, &TextEditor::modificationChanged, this, &TextApplication::onEditorTextChanged);
  connect(editor, &TextEditor::loadedFromFile, this, &TextApplication::onEditorLoadedFromFile);
  connect(editor, &TextEditor::savedToFile, this, &TextApplication::onEditorSavedToFile);
  connect(editor, &TextEditor::requestVisibility, this, &TextApplication::onEditorRequestVisibility);

  return editor;
}

void TextApplication::onEditorRequestVisibility() {
  TextEditor* editor = qobject_cast<TextEditor*>(sender());

  if (editor != nullptr) {
    m_tabWidget->setCurrentWidget(editor);
  }
}

void TextApplication::onEditorSavedToFile() {
  TextEditor* editor = qobject_cast<TextEditor*>(sender());

  renameEditor(editor);
  updateToolBarFromEditor(editor, true);
}

void TextApplication::onEditorLoadedFromFile() {
  TextEditor* editor = qobject_cast<TextEditor*>(sender());

  renameEditor(editor);
  updateToolBarFromEditor(editor, true);
}

void TextApplication::markEditorModified(TextEditor* editor, bool modified) {
  int index = m_tabWidget->indexOf(editor);

  if (index >= 0) {
    m_tabWidget->tabBar()->setTabIcon(index, modified ?
                                      qApp->icons()->fromTheme(QSL("dialog-warning")) :
                                      qApp->icons()->fromTheme(QSL("text-plain")));

    updateToolBarFromEditor(editor, true);
  }
}

void TextApplication::onEditorTextChanged(bool modified) {
  TextEditor* editor = qobject_cast<TextEditor*>(sender());

  markEditorModified(editor, modified);
}

void TextApplication::setMainForm(FormMain* main_form) {
  m_mainForm = main_form;
  m_tabWidget = main_form->tabWidget();

  connect(m_tabWidget, &TabWidget::currentChanged, this, &TextApplication::onEditorTabSwitched);
  connect(m_tabWidget->tabBar(), &TabBar::emptySpaceDoubleClicked, this, &TextApplication::addEmptyTextEditor);
  connect(m_mainForm->m_ui->m_actionFileNew, &QAction::triggered, this, [this]() {
    TextEditor* editor = addEmptyTextEditor();
    m_tabWidget->setCurrentWidget(editor);
  });
  connect(m_mainForm->m_ui->m_actionFileOpen, &QAction::triggered, this, [this]() {
    openTextFile();
  });

  // Setup menus.
  connect(m_mainForm->m_ui->m_menuFileOpenWithEncoding, &QMenu::aboutToShow, this, [this]() {
    if (m_mainForm->m_ui->m_menuFileOpenWithEncoding->isEmpty()) {
      TextFactory::initializeEncodingMenu(m_mainForm->m_ui->m_menuFileOpenWithEncoding);
    }
  });
  connect(m_mainForm->m_ui->m_menuFileOpenWithEncoding, &QMenu::triggered, this, &TextApplication::openTextFile);

  connect(m_mainForm->m_ui->m_menuFileSaveWIthEncoding, &QMenu::aboutToShow, this, [this]() {
    if (m_mainForm->m_ui->m_menuFileSaveWIthEncoding->isEmpty()) {
      TextFactory::initializeEncodingMenu(m_mainForm->m_ui->m_menuFileSaveWIthEncoding);;
    }
  });

  onEditorTabSwitched();
}

void TextApplication::openTextFile(QAction* action) {
  QString file_path = QFileDialog::getOpenFileName(m_mainForm, tr("Open file"), qApp->documentsFolder(),
                                                   tr("Text files (*.txt);;All files (*)"));

  if (!file_path.isEmpty()) {
    if (action != nullptr && !action->data().isNull()) {
      loadTextEditorFromFile(file_path, action->data().toString());
    }
    else {
      loadTextEditorFromFile(file_path);
    }
  }
}

void TextApplication::onEditorTabSwitched(int index) {
  updateToolBarFromEditor(m_tabWidget->textEditorAt(index), false);
}

void TextApplication::updateToolBarFromEditor(TextEditor* editor, bool only_modified) {
  if (editor != nullptr) {
    if (!only_modified) {
      // We change all stuff, document is totally changed.
    }

    // We update stuff related to document changes always.
    m_mainForm->m_ui->m_actionFileSave->setEnabled(editor->isModified());
    m_mainForm->m_ui->m_actionFileSaveAs->setEnabled(true);
    m_mainForm->m_ui->m_menuFileSaveWIthEncoding->setEnabled(true);
  }
  else {
    // No editor selected.
    m_mainForm->m_ui->m_actionFileSave->setEnabled(false);
    m_mainForm->m_ui->m_actionFileSaveAs->setEnabled(false);
    m_mainForm->m_ui->m_menuFileSaveWIthEncoding->setEnabled(false);
  }

  // Enable this if there is at least one unsaved editor.
  m_mainForm->m_ui->m_actionFileSaveAll->setEnabled(anyModifiedEditor());

  // TODO: je vybranej novej editor, načíst detaily do status baru a jinam.
}

void TextApplication::renameEditor(TextEditor* editor) {
  int index = m_tabWidget->indexOf(editor);

  if (index >= 0) {
    if (!editor->filePath().isEmpty()) {
      m_tabWidget->tabBar()->setTabText(index, QFileInfo(editor->filePath()).fileName());
    }
  }
}