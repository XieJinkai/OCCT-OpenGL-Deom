#pragma once

#include <QWidget>

#include <memory>

#include <Standard_Handle.hxx>
#include <TopoDS_Shape.hxx>

#include "Occt/MeshTypes.h"

class AIS_InteractiveContext;
class V3d_Viewer;
class V3d_View;
class QPaintEngine;

class OcctViewerWidget final : public QWidget
{
  Q_OBJECT

public:
  explicit OcctViewerWidget(QWidget* parent = nullptr);
  ~OcctViewerWidget() override;

  bool loadIgsFile(const QString& filePath, QString* errorText = nullptr);
  bool exportObjFile(const QString& filePath, bool exportQuads, QString* errorText = nullptr);

protected:
  QPaintEngine* paintEngine() const override;
  void resizeEvent(QResizeEvent* event) override;
  void paintEvent(QPaintEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

private:
  void initOcct();
  void ensureOcctWindow();
  void fitAll();
  void redraw();

  bool buildTriangulation(bool buildQuads, QString* errorText);
  void displayShape();

  Handle(AIS_InteractiveContext) m_context;
  Handle(V3d_Viewer) m_viewer;
  Handle(V3d_View) m_view;

  bool m_occtWindowInitialized = false;

  bool m_isMouseRotating = false;
  bool m_isMousePanning = false;
  QPoint m_lastMousePos;

  TopoDS_Shape m_shape;
  std::shared_ptr<TriMesh> m_triMesh;
  std::shared_ptr<QuadMesh> m_quadMesh;
};
