#include "Occt/OcctViewerWidget.h"

#include <cmath>
#include <unordered_map>

#include <QMouseEvent>
#include <QWheelEvent>

#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <Graphic3d_Camera.hxx>
#include <Graphic3d_GraphicDriver.hxx>
#include <Graphic3d_RenderingParams.hxx>
#include <IGESControl_Reader.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Poly_Triangulation.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopAbs_Orientation.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>

#ifdef _WIN32
  #include <WNT_Window.hxx>
#endif

#include "Occt/MeshTypes.h"
#include "Occt/ObjExporter.h"

static gp_Pnt transformedNode(const Handle(Poly_Triangulation)& tri, int nodeIndex1, const gp_Trsf& trsf)
{
  gp_Pnt p = tri->Node(nodeIndex1);
  p.Transform(trsf);
  return p;
}

static bool isCoplanar(const gp_Pnt& a, const gp_Pnt& b, const gp_Pnt& c, const gp_Pnt& d)
{
  const gp_Vec ab(a, b);
  const gp_Vec ac(a, c);
  const gp_Vec n = ab.Crossed(ac);
  const double n2 = n.SquareMagnitude();
  if (n2 < 1e-18)
    return false;

  const gp_Vec ad(a, d);
  const double dist = std::abs(ad.Dot(n)) / std::sqrt(n2);
  return dist < 1e-6;
}

OcctViewerWidget::OcctViewerWidget(QWidget* parent)
  : QWidget(parent)
{
  setAttribute(Qt::WA_NoSystemBackground);
  setAttribute(Qt::WA_PaintOnScreen);
  setAttribute(Qt::WA_OpaquePaintEvent);
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);

  initOcct();
}

OcctViewerWidget::~OcctViewerWidget()
{
  m_view.Nullify();
  m_context.Nullify();
  m_viewer.Nullify();
}

void OcctViewerWidget::initOcct()
{
  Handle(Aspect_DisplayConnection) displayConnection = new Aspect_DisplayConnection();
  Handle(OpenGl_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(displayConnection, false);

  m_viewer = new V3d_Viewer(graphicDriver);
  m_viewer->SetDefaultLights();
  m_viewer->SetLightOn();

  m_context = new AIS_InteractiveContext(m_viewer);
  m_view = m_viewer->CreateView();
  m_view->SetImmediateUpdate(false);
}

void OcctViewerWidget::ensureOcctWindow()
{
  if (m_occtWindowInitialized)
    return;

#ifdef _WIN32
  Handle(WNT_Window) wnd = new WNT_Window(reinterpret_cast<Aspect_Handle>(winId()));
  m_view->SetWindow(wnd);
  if (!wnd->IsMapped())
    wnd->Map();
#else
  (void)0;
#endif

  m_occtWindowInitialized = true;
  m_view->SetBackgroundColor(Quantity_NOC_GRAY20);

  Graphic3d_RenderingParams& params = m_view->ChangeRenderingParams();
  params.NbMsaaSamples = 4;
  params.LineFeather = 1.5f;
  params.ShadingModel = Graphic3d_TypeOfShadingModel_Phong;

  m_view->MustBeResized();
  m_view->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_GRAY40, 0.08, V3d_ZBUFFER);
  fitAll();
}

void OcctViewerWidget::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);
  ensureOcctWindow();
  if (!m_view.IsNull())
    m_view->MustBeResized();
  redraw();
}

void OcctViewerWidget::paintEvent(QPaintEvent* event)
{
  (void)event;
  ensureOcctWindow();
  if (!m_view.IsNull())
    m_view->Redraw();
}

void OcctViewerWidget::redraw()
{
  update();
}

void OcctViewerWidget::fitAll()
{
  if (m_view.IsNull())
    return;
  m_view->FitAll(0.01, false);
  m_view->ZFitAll();
}

void OcctViewerWidget::mousePressEvent(QMouseEvent* event)
{
  m_lastMousePos = event->pos();
  if (event->button() == Qt::LeftButton)
  {
    m_isMouseRotating = true;
    if (!m_view.IsNull())
      m_view->StartRotation(m_lastMousePos.x(), m_lastMousePos.y());
  }
  if (event->button() == Qt::MiddleButton)
    m_isMousePanning = true;
}

void OcctViewerWidget::mouseMoveEvent(QMouseEvent* event)
{
  if (m_view.IsNull())
    return;

  const QPoint cur = event->pos();
  const QPoint delta = cur - m_lastMousePos;
  m_lastMousePos = cur;

  if (m_isMouseRotating)
  {
    m_view->Rotation(cur.x(), cur.y());
    redraw();
    return;
  }

  if (m_isMousePanning)
  {
    m_view->Pan(delta.x(), -delta.y());
    redraw();
    return;
  }
}

void OcctViewerWidget::mouseReleaseEvent(QMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
    m_isMouseRotating = false;
  if (event->button() == Qt::MiddleButton)
    m_isMousePanning = false;
}

void OcctViewerWidget::wheelEvent(QWheelEvent* event)
{
  if (m_view.IsNull())
    return;

  const QPoint numDegrees = event->angleDelta() / 8;
  if (numDegrees.isNull())
    return;

  const double factor = numDegrees.y() > 0 ? 0.9 : 1.1;
  Handle(Graphic3d_Camera) cam = m_view->Camera();
  if (!cam.IsNull())
  {
    if (cam->IsOrthographic())
      cam->SetScale(cam->Scale() * factor);
    else
      cam->SetDistance(cam->Distance() * factor);
  }
  m_view->Invalidate();
  redraw();
}

QPaintEngine* OcctViewerWidget::paintEngine() const
{
  return nullptr;
}

bool OcctViewerWidget::loadIgsFile(const QString& filePath, QString* errorText)
{
  IGESControl_Reader reader;
  const IFSelect_ReturnStatus status = reader.ReadFile(filePath.toUtf8().constData());
  if (status != IFSelect_RetDone)
  {
    if (errorText)
      *errorText = QStringLiteral("IGES读取失败：%1").arg(filePath);
    return false;
  }

  reader.TransferRoots();
  TopoDS_Shape shape = reader.OneShape();
  if (shape.IsNull())
  {
    if (errorText)
      *errorText = QStringLiteral("IGES文件未生成有效Shape");
    return false;
  }

  m_shape = shape;
  m_triMesh.reset();
  m_quadMesh.reset();

  if (!m_context.IsNull())
    m_context->RemoveAll(false);
  displayShape();
  fitAll();
  redraw();
  return true;
}

void OcctViewerWidget::displayShape()
{
  if (m_shape.IsNull() || m_context.IsNull())
    return;
  Handle(AIS_Shape) prs = new AIS_Shape(m_shape);
  m_context->Display(prs, false);
  m_context->SetDisplayMode(prs, AIS_Shaded, false);
  fitAll();
}

static std::shared_ptr<TriMesh> buildGlobalTriMesh(const TopoDS_Shape& shape)
{
  BRepMesh_IncrementalMesh mesher(shape, 0.5, false, 0.5, true);
  mesher.Perform();

  auto mesh = std::make_shared<TriMesh>();
  struct Key
  {
    long long x = 0;
    long long y = 0;
    long long z = 0;
    bool operator==(const Key& o) const { return x == o.x && y == o.y && z == o.z; }
  };

  struct KeyHash
  {
    size_t operator()(const Key& k) const noexcept
    {
      const size_t hx = static_cast<size_t>(k.x) * 73856093ULL;
      const size_t hy = static_cast<size_t>(k.y) * 19349663ULL;
      const size_t hz = static_cast<size_t>(k.z) * 83492791ULL;
      return hx ^ hy ^ hz;
    }
  };

  std::unordered_map<Key, int, KeyHash> vertexMap;
  vertexMap.reserve(200000);

  const double scale = 1000000.0;
  auto keyOf = [&](const gp_Pnt& p) -> Key {
    return Key{
      static_cast<long long>(std::llround(p.X() * scale)),
      static_cast<long long>(std::llround(p.Y() * scale)),
      static_cast<long long>(std::llround(p.Z() * scale)),
    };
  };

  for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next())
  {
    const TopoDS_Face face = TopoDS::Face(exp.Current());
    TopLoc_Location loc;
    const Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);
    if (tri.IsNull())
      continue;

    const gp_Trsf trsf = loc.Transformation();
    const int nbTriangles = tri->NbTriangles();
    for (int i = 1; i <= nbTriangles; ++i)
    {
      int n1 = 0, n2 = 0, n3 = 0;
      tri->Triangle(i).Get(n1, n2, n3);
      if (face.Orientation() == TopAbs_REVERSED)
        std::swap(n2, n3);

      const gp_Pnt p1 = transformedNode(tri, n1, trsf);
      const gp_Pnt p2 = transformedNode(tri, n2, trsf);
      const gp_Pnt p3 = transformedNode(tri, n3, trsf);

      const gp_Pnt pts[3] = {p1, p2, p3};
      int ids[3] = {-1, -1, -1};

      for (int k = 0; k < 3; ++k)
      {
        const Key key = keyOf(pts[k]);
        auto it = vertexMap.find(key);
        if (it == vertexMap.end())
        {
          const int newId = static_cast<int>(mesh->vertices.size());
          mesh->vertices.push_back(pts[k]);
          vertexMap.emplace(key, newId);
          ids[k] = newId;
        }
        else
        {
          ids[k] = it->second;
        }
      }

      mesh->indices.push_back(ids[0]);
      mesh->indices.push_back(ids[1]);
      mesh->indices.push_back(ids[2]);
    }
  }

  return mesh;
}

static std::shared_ptr<QuadMesh> buildQuadMeshFromTriMesh(const TriMesh& triMesh)
{
  auto quad = std::make_shared<QuadMesh>();
  quad->vertices = triMesh.vertices;

  struct EdgeKey
  {
    int a = 0;
    int b = 0;
    bool operator==(const EdgeKey& o) const { return a == o.a && b == o.b; }
  };

  struct EdgeHash
  {
    size_t operator()(const EdgeKey& e) const noexcept
    {
      return (static_cast<size_t>(e.a) << 32) ^ static_cast<size_t>(e.b);
    }
  };

  struct TriRef
  {
    int triIndex = 0;
    int localEdge = 0;
  };

  const int triCount = static_cast<int>(triMesh.indices.size() / 3);
  std::vector<bool> used(triCount, false);
  std::unordered_map<EdgeKey, TriRef, EdgeHash> edgeOwner;
  edgeOwner.reserve(static_cast<size_t>(triCount) * 2);

  auto edgeKey = [](int u, int v) -> EdgeKey {
    if (u < v)
      return {u, v};
    return {v, u};
  };

  auto triVertex = [&](int t, int i) -> int { return triMesh.indices[static_cast<size_t>(t) * 3 + i]; };

  struct QuadCandidate
  {
    int t0 = -1;
    int t1 = -1;
    int sharedA = -1;
    int sharedB = -1;
    int other0 = -1;
    int other1 = -1;
  };

  std::vector<QuadCandidate> candidates;
  candidates.reserve(triCount / 2);

  for (int t = 0; t < triCount; ++t)
  {
    const int v0 = triVertex(t, 0);
    const int v1 = triVertex(t, 1);
    const int v2 = triVertex(t, 2);

    const int ev[3][2] = {{v0, v1}, {v1, v2}, {v2, v0}};
    for (int e = 0; e < 3; ++e)
    {
      const EdgeKey k = edgeKey(ev[e][0], ev[e][1]);
      auto it = edgeOwner.find(k);
      if (it == edgeOwner.end())
      {
        edgeOwner.emplace(k, TriRef{t, e});
      }
      else
      {
        const int t2 = it->second.triIndex;
        if (t2 == t)
          continue;

        const int a = k.a;
        const int b = k.b;

        const int tv0 = triVertex(t, 0);
        const int tv1 = triVertex(t, 1);
        const int tv2 = triVertex(t, 2);
        const int ov_t = (tv0 != a && tv0 != b) ? tv0 : (tv1 != a && tv1 != b) ? tv1 : tv2;

        const int uv0 = triVertex(t2, 0);
        const int uv1 = triVertex(t2, 1);
        const int uv2 = triVertex(t2, 2);
        const int ov_t2 = (uv0 != a && uv0 != b) ? uv0 : (uv1 != a && uv1 != b) ? uv1 : uv2;

        candidates.push_back(QuadCandidate{t2, t, a, b, ov_t2, ov_t});
      }
    }
  }

  auto angleOk = [&](int sharedA, int sharedB, int other0, int other1) -> bool {
    const gp_Pnt& A = quad->vertices[sharedA];
    const gp_Pnt& B = quad->vertices[sharedB];
    const gp_Pnt& C = quad->vertices[other0];
    const gp_Pnt& D = quad->vertices[other1];
    if (!isCoplanar(A, B, C, D))
      return false;

    const gp_Vec n1(gp_Vec(A, C).Crossed(gp_Vec(A, B)));
    const gp_Vec n2(gp_Vec(B, A).Crossed(gp_Vec(B, D)));
    const double n1m = n1.Magnitude();
    const double n2m = n2.Magnitude();
    if (n1m < 1e-12 || n2m < 1e-12)
      return false;
    const double cosang = std::abs(n1.Dot(n2) / (n1m * n2m));
    return cosang > 0.99;
  };

  for (const auto& c : candidates)
  {
    if (c.t0 < 0 || c.t1 < 0)
      continue;
    if (used[c.t0] || used[c.t1])
      continue;
    if (!angleOk(c.sharedA, c.sharedB, c.other0, c.other1))
      continue;

    used[c.t0] = true;
    used[c.t1] = true;

    quad->quadIndices.push_back(c.other0);
    quad->quadIndices.push_back(c.sharedA);
    quad->quadIndices.push_back(c.other1);
    quad->quadIndices.push_back(c.sharedB);
  }

  for (int t = 0; t < triCount; ++t)
  {
    if (used[t])
      continue;
    quad->triIndices.push_back(triVertex(t, 0));
    quad->triIndices.push_back(triVertex(t, 1));
    quad->triIndices.push_back(triVertex(t, 2));
  }

  return quad;
}

bool OcctViewerWidget::buildTriangulation(bool buildQuads, QString* errorText)
{
  if (m_shape.IsNull())
  {
    if (errorText)
      *errorText = QStringLiteral("当前没有模型");
    return false;
  }

  if (!m_triMesh)
    m_triMesh = buildGlobalTriMesh(m_shape);

  if (m_triMesh->vertices.empty() || m_triMesh->indices.empty())
  {
    if (errorText)
      *errorText = QStringLiteral("模型网格为空（可能是导入失败或无法三角化）");
    return false;
  }

  if (buildQuads && !m_quadMesh)
    m_quadMesh = buildQuadMeshFromTriMesh(*m_triMesh);

  return true;
}

bool OcctViewerWidget::exportObjFile(const QString& filePath, bool exportQuads, QString* errorText)
{
  if (m_shape.IsNull())
  {
    if (errorText)
      *errorText = QStringLiteral("当前没有模型");
    return false;
  }

  QString err;
  if (!buildTriangulation(exportQuads, &err))
  {
    if (errorText)
      *errorText = err;
    return false;
  }

  if (exportQuads && m_quadMesh)
    return ObjExporter::exportQuadMesh(filePath, *m_quadMesh, errorText);

  if (m_triMesh)
    return ObjExporter::exportTriMesh(filePath, *m_triMesh, errorText);

  if (errorText)
    *errorText = QStringLiteral("网格数据不可用");
  return false;
}
