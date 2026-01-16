#pragma once

#include <QString>

struct TriMesh;
struct QuadMesh;

class ObjExporter
{
public:
  static bool exportTriMesh(const QString& filePath, const TriMesh& mesh, QString* errorText);
  static bool exportQuadMesh(const QString& filePath, const QuadMesh& mesh, QString* errorText);
};

