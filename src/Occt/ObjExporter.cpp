#include "Occt/ObjExporter.h"

#include <QFile>
#include <QTextStream>
#include <QtGlobal>

#include "Occt/MeshTypes.h"

static bool openForWrite(const QString& filePath, QFile& file, QString* errorText)
{
  file.setFileName(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    if (errorText)
      *errorText = QStringLiteral("无法写入文件：%1").arg(filePath);
    return false;
  }
  return true;
}

bool ObjExporter::exportTriMesh(const QString& filePath, const TriMesh& mesh, QString* errorText)
{
  QFile file;
  if (!openForWrite(filePath, file, errorText))
    return false;

  QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  out.setEncoding(QStringConverter::Utf8);
#else
  out.setCodec("UTF-8");
#endif

  for (const auto& p : mesh.vertices)
    out << "v " << p.X() << " " << p.Y() << " " << p.Z() << "\n";

  if (mesh.indices.size() % 3 != 0)
  {
    if (errorText)
      *errorText = QStringLiteral("三角索引数量不是3的倍数");
    return false;
  }

  for (size_t i = 0; i < mesh.indices.size(); i += 3)
  {
    const int a = mesh.indices[i + 0] + 1;
    const int b = mesh.indices[i + 1] + 1;
    const int c = mesh.indices[i + 2] + 1;
    out << "f " << a << " " << b << " " << c << "\n";
  }

  return true;
}

bool ObjExporter::exportQuadMesh(const QString& filePath, const QuadMesh& mesh, QString* errorText)
{
  QFile file;
  if (!openForWrite(filePath, file, errorText))
    return false;

  QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  out.setEncoding(QStringConverter::Utf8);
#else
  out.setCodec("UTF-8");
#endif

  for (const auto& p : mesh.vertices)
    out << "v " << p.X() << " " << p.Y() << " " << p.Z() << "\n";

  if (mesh.quadIndices.size() % 4 != 0)
  {
    if (errorText)
      *errorText = QStringLiteral("四边形索引数量不是4的倍数");
    return false;
  }
  if (mesh.triIndices.size() % 3 != 0)
  {
    if (errorText)
      *errorText = QStringLiteral("三角索引数量不是3的倍数");
    return false;
  }

  for (size_t i = 0; i < mesh.quadIndices.size(); i += 4)
  {
    const int a = mesh.quadIndices[i + 0] + 1;
    const int b = mesh.quadIndices[i + 1] + 1;
    const int c = mesh.quadIndices[i + 2] + 1;
    const int d = mesh.quadIndices[i + 3] + 1;
    out << "f " << a << " " << b << " " << c << " " << d << "\n";
  }

  for (size_t i = 0; i < mesh.triIndices.size(); i += 3)
  {
    const int a = mesh.triIndices[i + 0] + 1;
    const int b = mesh.triIndices[i + 1] + 1;
    const int c = mesh.triIndices[i + 2] + 1;
    out << "f " << a << " " << b << " " << c << "\n";
  }

  return true;
}
