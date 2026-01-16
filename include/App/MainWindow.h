#pragma once

#include <QMainWindow>

class QAction;

class OcctViewerWidget;

class MainWindow final : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  ~MainWindow() override = default;

private:
  void createUi();
  void connectSignals();

  void importIgs();
  void exportObj();

  OcctViewerWidget* m_viewer = nullptr;

  QAction* m_importIgsAction = nullptr;
  QAction* m_exportObjAction = nullptr;
  QAction* m_exitAction = nullptr;
};
