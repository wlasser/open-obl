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

void ConvexTransformShape::read(std::istream &is) {
  TransformShape::read(is);
}

void ConvexSweepShape::read(std::istream &is) {
  Shape::read(is);
  is >> shape;
  is >> material;
  io::readBytes(is, radius);
  is >> unknown;
}

void BvTreeShape::read(std::istream &is) {
  Shape::read(is);
}

void MoppBvTreeShape::read(std::istream &is) {
  BvTreeShape::read(is);
  is >> shape;
  is >> material;
  io::readBytes(is, unused);
  io::readBytes(is, shapeScale);
  io::readBytes(is, moppDataSize);

  is >> origin;
  io::readBytes(is, scale);

  io::readBytes(is, moppData, moppDataSize);
}

void ShapeCollection::read(std::istream &is) {
  Shape::read(is);
}

void PackedNiTriStripsShape::read(std::istream &is) {
  ShapeCollection::read(is);
  io::readBytes(is, numSubShapes);
  subShapes.reserve(numSubShapes);
  for (auto i = 0; i < numSubShapes; ++i) {
    subShapes.emplace_back(version);
    is >> subShapes.back();
  }

  io::readBytes(is, userData);
  io::readBytes(is, unused1);
  io::readBytes(is, radius);
  io::readBytes(is, unused2);
  is >> scale;
  io::readBytes(is, radiusCopy);
  is >> scaleCopy;

  is >> data;
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

void RigidBody::read(std::istream &is) {
  Entity::read(is);
  io::readBytes(is, collisionResponse);
  io::readBytes(is, unusedByte1);
  io::readBytes(is, processContactCallbackDelay);
  io::readBytes(is, unknownInt1);
  is >> havokFilterCopy;
  io::readBytes(is, unused2);
  io::readBytes(is, collisionResponse2);
  io::readBytes(is, unusedByte2);
  io::readBytes(is, processContactCallbackDelay2);
  io::readBytes(is, unknownInt2);
  is >> translation;
  is >> rotation;
  is >> linearVelocity;
  is >> angularVelocity;
  is >> inertiaTensor;
  is >> center;
  io::readBytes(is, mass);
  io::readBytes(is, linearDamping);
  io::readBytes(is, angularDamping);
  io::readBytes(is, friction);
  io::readBytes(is, restitution);
  io::readBytes(is, maxLinearVelocity);
  io::readBytes(is, maxAngularVelocity);
  io::readBytes(is, penetrationDepth);
  io::readBytes(is, motionSystem);
  io::readBytes(is, deactivatorType);
  io::readBytes(is, solverDeactivation);
  io::readBytes(is, qualityType);
  io::readBytes(is, unknownBytes1);
  io::readBytes(is, numConstraints);
  constraints.reserve(numConstraints);
  for (auto i = 0; i < numConstraints; ++i) {
    constraints.emplace_back();
    is >> constraints.back();
  }
  io::readBytes(is, bodyFlags);
}

void RigidBodyT::read(std::istream &is) {
  RigidBody::read(is);
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

namespace nif::hk {

void PackedNiTriStripsData::read(std::istream &is) {
  nif::bhk::ShapeCollection::read(is);
  io::readBytes(is, numTriangles);
  triangles.reserve(numTriangles);
  for (auto i = 0; i < numTriangles; ++i) {
    triangles.emplace_back();
    is >> triangles.back();
  }

  io::readBytes(is, numVertices);
  vertices.reserve(numVertices);
  for (auto i = 0; i < numVertices; ++i) {
    vertices.emplace_back();
    is >> vertices.back();
  }
}

} // namespace nif::hk
