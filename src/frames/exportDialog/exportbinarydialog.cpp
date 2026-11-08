#include "exportbinarydialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>
#include <utility>

namespace editor {

ExportBinaryDialog::ExportBinaryDialog(QList<PageInfo> pages, int defaultPageIndex,
                                       const QString &defaultDirectory,
                                       QWidget *parent)
    : QDialog(parent), pages_(std::move(pages)) {
  setWindowTitle(QStringLiteral("Экспорт микрокоманд К1804"));
  setMinimumWidth(500);

  auto *layout = new QVBoxLayout(this);
  auto *form = new QFormLayout;

  // 1. Выбор страницы
  pageCombo_ = new QComboBox(this);
  pageCombo_->setObjectName(QStringLiteral("pageCombo"));
  for (const PageInfo &page : pages_) {
    int count = 0;
    if (page.scene) {
      for (const ShapeElement *element : page.scene->allElements()) {
        if (dynamic_cast<const MasterBlockElement *>(element))
          ++count;
      }
    }
    pageCombo_->addItem(QStringLiteral("%1 — блоков К1804: %2").arg(page.name).arg(count));
  }
  int initialIndex = (defaultPageIndex >= 0 && defaultPageIndex < pages_.size())
                         ? defaultPageIndex
                         : (pages_.isEmpty() ? -1 : 0);
  pageCombo_->setCurrentIndex(initialIndex);
  form->addRow(QStringLiteral("Страница:"), pageCombo_);

  // 2. Имя файла
  fileNameEdit_ = new QLineEdit(this);
  fileNameEdit_->setObjectName(QStringLiteral("fileNameEdit"));
  form->addRow(QStringLiteral("Имя файла:"), fileNameEdit_);

  // 3. Выбор каталога
  auto *dirLayout = new QHBoxLayout;
  dirEdit_ = new QLineEdit(this);
  dirEdit_->setObjectName(QStringLiteral("dirEdit"));
  QString initialDir = defaultDirectory;
  if (initialDir.isEmpty() || !QDir(initialDir).exists()) {
    initialDir = QDir::currentPath();
  }
  dirEdit_->setText(QDir::toNativeSeparators(initialDir));

  auto *browseButton = new QPushButton(QStringLiteral("Обзор..."), this);
  browseButton->setAutoDefault(false);
  dirLayout->addWidget(dirEdit_);
  dirLayout->addWidget(browseButton);
  form->addRow(QStringLiteral("Каталог:"), dirLayout);

  layout->addLayout(form);

  // Обновляем имя файла при открытии и при смене страницы в комбобоксе
  updateFileNameForPage(pageCombo_->currentIndex());
  connect(pageCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &ExportBinaryDialog::updateFileNameForPage);

  // 4. Информационный блок
  auto *info = new QFrame(this);
  info->setFrameShape(QFrame::StyledPanel);
  auto *infoLayout = new QHBoxLayout(info);
  auto *icon = new QLabel(info);
  icon->setPixmap(style()->standardIcon(QStyle::SP_MessageBoxInformation).pixmap(24, 24));
  auto *hint = new QLabel(QStringLiteral("1 страница = 1 прошивка ПЗУ К1804. В выбранный каталог будет сохранён бинарный файл микрокоманд."), info);
  hint->setWordWrap(true);
  infoLayout->addWidget(icon);
  infoLayout->addWidget(hint, 1);
  layout->addWidget(info);

  // 5. Кнопки Экспорт / Отмена
  auto *buttons = new QDialogButtonBox(this);
  auto *exportButton = buttons->addButton(QStringLiteral("Экспорт"), QDialogButtonBox::AcceptRole);
  buttons->addButton(QStringLiteral("Отмена"), QDialogButtonBox::RejectRole);
  exportButton->setDefault(true);
  layout->addWidget(buttons);

  connect(browseButton, &QPushButton::clicked, this, [this]() {
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Выберите каталог для экспорта"), dirEdit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
      dirEdit_->setText(QDir::toNativeSeparators(dir));
    }
  });

  connect(buttons, &QDialogButtonBox::accepted, this, &ExportBinaryDialog::accept);
  connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ExportBinaryDialog::updateFileNameForPage(int index) {
  if (index >= 0 && index < pages_.size()) {
    QString pageName = pages_.at(index).name.trimmed();
    // Заменяем недопустимые символы в имени файла
    pageName.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("_"));
    if (pageName.isEmpty()) {
      pageName = QStringLiteral("rom");
    }
    fileNameEdit_->setText(pageName + QStringLiteral(".bin"));
  }
}

int ExportBinaryDialog::selectedPageIndex() const {
  return pageCombo_->currentIndex();
}

QString ExportBinaryDialog::selectedDirectory() const {
  return dirEdit_->text().trimmed();
}

QString ExportBinaryDialog::selectedFileName() const {
  QString name = fileNameEdit_->text().trimmed();
  if (!name.endsWith(QStringLiteral(".bin"), Qt::CaseInsensitive) &&
      !name.endsWith(QStringLiteral(".rom"), Qt::CaseInsensitive)) {
    name += QStringLiteral(".bin");
  }
  return name;
}

QString ExportBinaryDialog::selectedFilePath() const {
  QDir dir(selectedDirectory());
  return dir.filePath(selectedFileName());
}

void ExportBinaryDialog::accept() {
  const int index = selectedPageIndex();
  if (index < 0 || index >= pages_.size() || !pages_.at(index).scene) {
    QMessageBox::warning(this, QStringLiteral("Экспорт"),
                         QStringLiteral("Выберите доступную страницу для экспорта."));
    return;
  }

  const QString dirPath = selectedDirectory();
  if (dirPath.isEmpty() || !QDir(dirPath).exists()) {
    QMessageBox::warning(this, QStringLiteral("Экспорт"),
                         QStringLiteral("Укажите существующий каталог для сохранения."));
    dirEdit_->setFocus();
    return;
  }

  const QString fileName = fileNameEdit_->text().trimmed();
  if (fileName.isEmpty()) {
    QMessageBox::warning(this, QStringLiteral("Экспорт"),
                         QStringLiteral("Укажите имя файла для экспорта."));
    fileNameEdit_->setFocus();
    return;
  }

  // Проверка перезаписи существующего файла
  QFileInfo fi(selectedFilePath());
  if (fi.exists()) {
    auto res = QMessageBox::question(
        this, QStringLiteral("Подтверждение перезаписи"),
        QStringLiteral("Файл \"%1\" уже существует в выбранном каталоге.\nПерезаписать его?")
            .arg(fi.fileName()),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (res != QMessageBox::Yes) {
      return;
    }
  }

  QDialog::accept();
}

} // namespace editor
