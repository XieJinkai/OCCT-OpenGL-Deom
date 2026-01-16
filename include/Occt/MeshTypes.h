#pragma once

#include <vector>

#include <gp_Pnt.hxx>

struct TriMesh
{
  std::vector<gp_Pnt> vertices;
  std::vector<int> indices;
};

struct QuadMesh
{
  std::vector<gp_Pnt> vertices;
  std::vector<int> quadIndices;
  std::vector<int> triIndices;
};

