#include "nif/bhk.hpp"

namespace nif::bhk {

void RefObject::read(std::istream &is) {
  nif::NiObject::read(is);
}

void Serializable::read(std::istream &is) {
  RefObject::read(is);
}

void Shape::read(std::istream &is) {
  Serializable::read(is);
}

void TransformShape::read(std::istream &is) {
  Shape::read(is);
  is >> shape >> material;
  io::readBytes(is, radius);
  io::readBytes(is, unused);
  is >> transform;
}

void SphereRepShape::read(std::istream &is) {
  Shape::read(is);
  is >> material;
  io::readBytes(is, radius);
}

void ConvexShape::read(std::istream &is) {
  SphereRepShape::read(is);
}

void SphereShape::read(std::istream &is) {
  ConvexShape::read(is);
}

void CapsuleShape::read(std::istream &is) {
  ConvexShape::read(is);
  io::readBytes(is, unused);
  is >> firstPoint;
  io::readBytes(is, firstRadius);
  is >> secondPoint;
  io::readBytes(is, secondRadius);
}

void BoxShape::read(std::istream &is) {
  ConvexShape::read(is);
  io::readBytes(is, unused);
  is >> dimensions;
}

void ConvexVerticesShape::read(std::istream &is) {
  ConvexShape::read(is);
  is >> verticesProperty >> normalsProperty;

  io::readBytes(is, numVertices);
  vertices.reserve(numVertices);
  for (auto i = 0; i < numVertices; ++i) {
    vertices.emplace_back();
    is >> vertices.back();
  }

  io::readBytes(is, numNormals);
  normals.reserve(numNormals);
  for (auto i = 0; i < numNormals; ++i) {
    normals.emplace_back();
    is >> normals.back();
  }
}

void WorldObject::read(std::istream &is) {
  Serializable::read(is);
  is >> shape;
  io::readBytes(is, unknownInt);
  is >> havokFilter;
  io::readBytes(is, unused1);
  io::readBytes(is, broadPhaseType);
  io::readBytes(is, unused2);
  is >> cinfoProperty;
}

void Phantom::read(std::istream &is) {
  WorldObject::read(is);
}

void ShapePhantom::read(std::istream &is) {
  Phantom::read(is);
}

void SimpleShapePhantom::read(std::istream &is) {
  ShapePhantom::read(is);
  io::readBytes(is, unused3);
  is >> transform;
}

void Entity::read(std::istream &is) {
  WorldObject::read(is);
}

void NiCollisionObject::read(std::istream &is) {
  nif::NiCollisionObject::read(is);
  io::readBytes(is, flags);
  is >> body;
}

void CollisionObject::read(std::istream &is) {
  NiCollisionObject::read(is);
}

} // namespace nif::bhk
