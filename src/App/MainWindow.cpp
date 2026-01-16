#include "App/MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>

#include "Occt/OcctViewerWidget.h"

MainWindow::MainWindow(QWidget* parent)
  : QMainWindow(parent)
{
  createUi();
  connectSignals();
}

void MainWindow::createUi()
{
  setWindowTitle(QStringLiteral("IgsMesh (MVP)"));

  m_viewer = new OcctViewerWidget(this);
  setCentralWidget(m_viewer);

  auto* fileMenu = menuBar()->addMenu(QStringLiteral("文件"));

  m_importIgsAction = fileMenu->addAction(QStringLiteral("导入 IGS..."));
  m_exportObjAction = fileMenu->addAction(QStringLiteral("导出 OBJ..."));
  fileMenu->addSeparator();
  m_exitAction = fileMenu->addAction(QStringLiteral("退出"));

  auto* toolBar = addToolBar(QStringLiteral("工具"));
  toolBar->setMovable(false);
  toolBar->addAction(m_importIgsAction);
  toolBar->addAction(m_exportObjAction);

  statusBar()->showMessage(QStringLiteral("就绪"));
}

void MainWindow::connectSignals()
{
  connect(m_importIgsAction, &QAction::triggered, this, &MainWindow::importIgs);
  connect(m_exportObjAction, &QAction::triggered, this, &MainWindow::exportObj);
  connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
}

void MainWindow::importIgs()
{
  const QString filePath = QFileDialog::getOpenFileName(
    this,
    QStringLiteral("选择 IGS 文件"),
    QString(),
    QStringLiteral("IGS/IGES (*.igs *.iges);;所有文件 (*.*)"));

  if (filePath.isEmpty())
    return;

  QString errorText;
  if (!m_viewer->loadIgsFile(filePath, &errorText))
  {
    QMessageBox::critical(this, QStringLiteral("导入失败"), errorText);
    return;
  }

  statusBar()->showMessage(QStringLiteral("已导入：%1").arg(filePath), 3000);
}

void MainWindow::exportObj()
{
  const QString filePath = QFileDialog::getSaveFileName(
    this,
    QStringLiteral("导出 OBJ"),
    QString(),
    QStringLiteral("OBJ (*.obj)"));

  if (filePath.isEmpty())
    return;

  QString errorText;
  if (!m_viewer->exportObjFile(filePath, false, &errorText))
  {
    QMessageBox::critical(this, QStringLiteral("导出失败"), errorText);
    return;
  }

  statusBar()->showMessage(QStringLiteral("已导出：%1").arg(filePath), 3000);
}
