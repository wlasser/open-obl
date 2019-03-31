#ifndef OPENOBLIVION_NIFLOADER_ANIMATION_HPP
#define OPENOBLIVION_NIFLOADER_ANIMATION_HPP

#include "nifloader/nif_resource.hpp"
#include <OgreAnimation.h>
#include <OgreSkeleton.h>
#include <string>

/// \file animation.hpp
/// \addtogroup OpenOblivionNifloader
///
/// ### Animation
///
/// #### A Brief Overview of Skeletal Animation
///
/// First, a quick overview of skeletal animation using hardware skinning for
/// those unfamiliar with it. Instead of moving every vertex of a mesh directly,
/// we introduce a *skeleton* made up of *bones* and link different parts of the
/// mesh to different bones. By animating just the bones, we can indirectly
/// animate the entire mesh. Specifically, if the skeleton has bones
/// \f$b_1, b_2, \dots, b_N\f$ then each vertex \f$v_i\f$ in the mesh is
/// assigned some subset \f$b_{i_1}, b_{i_2}, \dots, b_{i_n}\f$ of the bones,
/// along with some weights \f$w_{i_1}, w_{i_2}, \dots, w_{i_n}\f$.
/// (The value \f$n\f$ is constant across all meshes and is usually
/// \f$n = 4\f$ for hardware reasons.) When a bone \f$b_j\f$ is moved, all the
/// vertices that have a nonzero weight associated with \f$b_j\f$ move as well,
/// proportional to the weight. The bones are arranged in a tree structure so
/// that moving one bone moves all the bones further down the hierarchy.
/// Rotating a shoulder bone would implicitly move the entire arm, for example.
///
/// The bones all start in a *binding pose* that matches the pose that the mesh
/// was modelled in. An animation then consists of a series of *keyframes* which
/// specify linear transformations of some subset of the bones from their
/// binding pose at some instance of time. By interpolating the linear
/// transformations of bones between keyframes, each bone in the skeleton is
/// given a linear transformation \f$T_i(t)\f$ from its binding pose for every
/// point in time \f$t\f$. Vertex \f$v_i\f$ of the mesh is then animated by
/// applying the weighted linear transformation
/// \f$w_{i_1} T_{i_1}(t) + w_{i_2} T_{i_2}(t) + \dots + w_{i_n} T_{i_n}(t)\f$
/// to the vertex from its binding pose.
///
/// The assignment of bones and weights to each vertex are stored in the vertex
/// buffer of the mesh together with the vertex positions, normals, etc. and
/// passed to the vertex shader. The transformations are computed on the CPU
/// and passed to the vertex shader as an array of uniforms. The bone
/// assignments act as indices into this array and together with the weights
/// the vertex shader computes the weighted transformation for the vertex.
/// This is where the 'hardware skinning' comes from; one could compute the
/// weighted transformations on the CPU and update the vertex positions and
/// normals before passing them to the GPU.
///
/// #### Skeletal Animation in OGRE
///
/// Because we need to implement skeletal animation in a slightly different way
/// to what (unpatched) OGRE expects, it is important to know how OGRE
/// implements skeletal animation. Suppose that we have an `Ogre::Mesh` that we
/// would like to skeletally animate using hardware skinning, and that each of
/// its `Ogre::SubMesh`es own their own vertex data. (If they don't then the
/// `Ogre::SubMesh` methods are called on the parent `Ogre::Mesh` instead.)
///
/// Before assigning bones to vertices, OGRE requires that the `Ogre::Mesh` be
/// assigned an `Ogre::Skeleton` by calling `Ogre::Mesh::setSkeletonName()`.
/// This method attempts to load the named `Ogre::Skeleton` resource, and
/// stores a pointer to it in the `Ogre::Mesh`. An `Ogre::Skeleton` consists
/// of a tree of `Ogre::Bone`s (which derive from `Ogre::Node`s) which have an
/// integer *handle* and an optional name. Bone handles correspond to the
/// \f$b_i\f$ notation above, and must be sequential integers starting from
/// zero. They can be generated automatically by the `Ogre::Skeleton` or
/// specified explicitly by the loader. After a bone is created using one of
/// the overloads of `Ogre::Skeleton::createBone()`, it can be transformed into
/// its binding pose and marked as such by `Ogre::Bone::setBindingPose()`.
///
/// To add bone assignments, one constructs a collection of
/// `Ogre::VertexBoneAssignment`s and adds them to the `Ogre::SubMesh` using
/// `Ogre::SubMesh::addBoneAssignment()`. Each `Ogre::VertexBoneAssignment`
/// holds a vertex index, a bone handle, and a weight. Multiple assignments can
/// be made for each vertex index, though OGRE will only use the
/// `OGRE_MAX_BLEND_WEIGHTS` most highly weighted. (This defaults to 4 and
/// corresponds to \f$n\f$ above.)
///
/// Once the assignments have been added, they must be compiled. If this is not
/// done manually by calling `Ogre::SubMesh::_compileBoneAssignments()` on each
/// `Ogre::SubMesh` or by calling `Ogre::Mesh::_updateCompiledBoneAssignments()`
/// on the parent `Ogre::Mesh`, then it will be done the first time
/// `Ogre::Entity::_initialise()` is called on an `Ogre::Entity` created from
/// the `Ogre::Mesh`. Compilation is handled by the parent `Ogre::Mesh` and
/// involves `Ogre::Mesh::_rationaliseBoneAssignments()` and
/// `Ogre::Mesh::compileBoneAssignments()`.
///
/// `Ogre::Mesh::_rationaliseBoneAssignments()` takes the bone assignments of
/// the `Ogre::SubMesh`, keeps the `OGRE_MAX_BLEND_WEIGHTS` most highly weighted
/// assignments for each vertex and normalizes those weights so that
/// \f$w_{i_1} + w_{i_2} + \dots + w_{i_n} = 1\f$.
///
/// `Ogre::Mesh::compileBoneAssignments()` stores the bone assignments and
/// weights into the vertex data of the `Ogre::SubMesh` so that they can be
/// passed to the vertex shader. Since the `Ogre::Skeleton` is shared between
/// the `Ogre::SubMesh`es but the vertex data is not, only a subset of the bones
/// will be used by the vertices in any given `Ogre::SubMesh`. It is a waste to
/// send the transformations of unused bones to the vertex shader, but omitting
/// them may break the contiguity of the bone indices. OGRE therefore introduces
/// a  contiguous *blend index* local to the `Ogre::SubMesh`, constructing a
/// `blendIndexToBoneIndexMap` and a `boneIndexToBlendIndexMap` map to
/// translate between the two indices. Before adding the vertex data, OGRE
/// creates a new vertex buffer for the blend indices and blend weights, and
/// binds it to the `Ogre::SubMesh`'s vertex `Ogre::VertexBufferBinding`. It
/// then iterates over every vertex in the `Ogre::SubMesh`, collecting the
/// blend weights, translating the bone indices to blend indices, and storing it
/// in the newly created buffer.
///
/// With the `Ogre::Skeleton` and `Ogre::Mesh` constructed, one can create an
/// `Ogre::Entity`. In `Ogre::Entity::_initialise()`, the `Ogre::Skeleton` is
/// used to construct an `Ogre::SkeletonInstance`, memory is allocated for
/// the bone transformations \f$T_1, \dots, T_N\f$, an `Ogre::AnimationStateSet`
/// is constructed and initialized, and
/// `Ogre::Entity::prepareTempBlendBuffers()` is called. Each of these steps
/// will be looked at in more detail.
///
/// An `Ogre::SkeletonInstance` is a copy (see remark) of an `Ogre::Skeleton`
/// that shares the same `Ogre::Animation`s as the `Ogre::Skeleton`. This means
/// that the skeletons of individual `Ogre::Entities` can be animated
/// individually but without the overhead of copying a lot of
/// `Ogre::Animation`s. Because the `Ogre::SkeletonInstance` dispatches to its
/// parent for all things animation, `Ogre::Animation`s added to the parent
/// after the construction of the `Ogre::SkeletonInstance` will be visible to
/// the `Ogre::SkeletonInstance`.
///
/// \remark The copying is not done in the copy constructor, it is done during
///         an explicit call to `Ogre::SkeletonInstance::load()`.
///
/// Skeletal animations are created in the context of, and owned by, a single
/// `Ogre::Skeleton`. They are shared among all `Ogre::SkeletonInstance` copies
/// of that `Ogre::Skeleton`, and are created by calling
/// `Ogre::Skeleton::createAnimation()`, providing a name that is unique within
/// the `Ogre::Skeleton`. It is possible to share animations between
/// `Ogre::Skeleton`s by calling
/// `Ogre::Skeleton::addLinkedSkeletonAnimationSource()` with a skeleton name
/// and optional scaling factor. When a lookup is required the linked animations
/// are checked after the animations of the skeleton itself, so a skeleton can
/// override the animations of any other skeletons that it has linked with.
///
/// Because an `Ogre::Entity` can have multiple animations playing
/// simultaneously, the state of each of its animations must be managed
/// individually. An `Ogre::AnimationState` represents a weighted
/// `Ogre::Animation` at a particular instant in time, and can be enabled or
/// disabled depending on whether that `Ogre::Animation` should be playing.
/// An `Ogre::AnimationState` is created for each `Ogre::Animation` in the
/// `Ogre::Mesh` (via its `Ogre::Skeleton`, and any linked ones) used to create
/// the `Ogre::Entity`, and stored together in an `Ogre::AnimationStateSet`
/// owned by the `Ogre::Entity`.
///
/// Regarding `Ogre::Entity::prepareTempBlendBuffers()`, in our case this simply
/// calls `Ogre::SubEntity::prepareTempBlendBufers()` on each of the
/// `Ogre::SubEntity`s. This function clones the `Ogre::VertexData` of the
/// `Ogre::SubMesh` (without copying the data itself) and removes the blend
/// indices/weights by unbinding the vertex buffers containing them if the
/// respective constants `Ogre::Root::isBlendIndicesGpuRedundant()` and
/// `Ogre::Root::isBlendWeightsGpuRedundant()` are true. It then extracts
/// `Ogre::TempBlendedBufferInfo` from the cloned `Ogre::VertexData`, in
/// particular getting the binding indices for the vertex buffers containing
/// the vertex positions and normals. This information is used later when the
/// animation states of the `Ogre::Entity` are updated, but crucially *only
/// for software animation*.
///
/// Finally we reach the actual updating of the animations, which occurs in
/// `Ogre::Entity::updateAnimation()`. This first needs to determine that the
/// skeletal animation is being done in hardware, and not in software. Briefly,
/// the animation is determined to be done in hardware if the function
/// `Ogre::GpuProgram::isSkeletalAnimationIncluded()` returns true for the
/// vertex program of the first pass of the best technique of the material of
/// every `Ogre::SubMesh`. For *that* to hold, one should add
/// `includes_skeletal_animation true` to the vertex program definitions.
/// Once that is done, the `Ogre::SkeletonInstance` updates all its bone
/// transforms by `Ogre::SkeletonInstance::setAnimationState()` (see remark)
/// and transfers them to the `Ogre::Entity` with
/// `Ogre::SkeletonInstance::_getBoneMatrices()`. These are concatenated with
/// the parent transform of the `Ogre::Entity` and stored in
/// `mBoneWorldMatrices`, ready to be passed as a uniform array to the vertex
/// shader using the `ACT_WORLD_MATRIX_ARRAY_3x4` auto-constant. (See remark.)
/// Recall that only those bone transforms used by the `Ogre::SubMesh` are used;
/// the `blendIndexToBoneIndexMap` is consulted to pass the bone transforms
/// in blend index order.
///
/// \remark This function neither sets an `Ogre::AnimationState` nor takes one;
///         it takes an `Ogre::AnimationStateSet` and uses each of the
///         `Ogre::AnimationState`s to update the transforms of the
///         `Ogre::Bone`s.
/// \remark In GLSL this is an array of `mat4x3`, not an array of `mat3x4`.
///
/// #### Skeletal Animation in OpenOblivion
///
/// With OGRE's process and assumptions described, we can discuss where our
/// requirements differ. Firstly, the skinning information of each `Ogre::Mesh`
/// is contained in the NIF file that produced it, and hence all the bone
/// assignments must be added during the creation of each `Ogre::SubMesh`.
/// It is also useful if the `Ogre::SkeletonInstance` that an `Ogre::Entity`
/// uses is decoupled from the `Ogre::Skeleton` assigned to its `Ogre::Mesh`,
/// because each NPC can have its own unique skeleton. If this were not
/// possible, then we would have to duplicate every mesh that is equipped by an
/// NPC with a nonstandard (if there even is such a thing) skeleton.
/// A consequence of both these things is that we cannot know the
/// `Ogre::Skeleton` that will be used with an `Ogre::Mesh` during the
/// construction of that `Ogre::Mesh`, and thus cannot satisfy the
/// precondition that `Ogre::Mesh::setSkeletonName()` be called before calling
/// `Ogre::SubMesh::addBoneAssignment()`.
///
/// On the other hand, a look at OGRE's internals shows that this precondition
/// is only expressed in documentation and is not exploited in the code. In
/// fact, the `Ogre::Skeleton` does not need to be known and loaded until
/// `Ogre::Entity::_initialise()` is called. Even then, if no skeleton has
/// been added to the `Ogre::Mesh` then the `Ogre::Entity` will simply be
/// created without skeletal animation support. One could then add an
/// `Ogre::Skeleton` to the `Ogre::Mesh` and call
/// `Ogre::Entity::_initialise(true)` to reinitialise the `Ogre::Entity` with
/// the new `Ogre::Skeleton`.
///
/// Of course, there is still the question of how the bone handles can be known
/// before an `Ogre::Skeleton` has been assigned. The fact that we require that
/// the same `Ogre::Mesh` produce `Ogre::Entity`s with different underlying
/// `Ogre::Skeleton`s implies that there is some consistency between the bone
/// handles of those `Ogre::Skeleton`s. While the `Ogre::Skeleton`s may differ,
/// there must be common structure amongst the parts that act on the
/// `Ogre::Mesh`; it would be strange if a piece of a mesh was controlled by
/// an arm bone in one skeleton but a leg bone in another, for instance.
/// Thus if a NIF file is known to be used with an `Ogre::Skeleton`, there is
/// still some implied structure known *a priori* even if the exact skeleton is
/// not known. NIF files that can be skeletally animated contain `nif::NiNode`s
/// that are named and positioned in the same way that the bones should be in
/// any skeleton used to animate the mesh. The skinning information is specified
/// in relation to the included nodes, which are arranged in a contiguous array;
/// their index in that array is analogous to OGRE's blend indices, and the
/// *a priori* information is the `blendIndexToBoneIndexMap`. Such a map
/// can be constructed given any specific `Ogre::Skeleton` by matching the
/// names of the `nif::NiNode` bones, or hardcoded.
///
/// Unfortunately, both methods of constructing the map prevent modders from
/// creating meshes that use new bones specific to custom skeletons. If the
/// map was hardcoded then it would have to overwritten, and would only support
/// one set of bone additions. If it was taken from a standard skeleton, then
/// that skeleton would have to include every bone from every mod used by the
/// player. That's actually how things have turned out; there is a
/// ['maximum compatibility' skeleton]
/// (https://www.nexusmods.com/oblivion/mods/27945)
/// including the bones used by many different mods. Unfortunately, such a
/// method doesn't work with the vanilla game because `skeleton.nif` does not
/// include a tail, unlike `skeletonbeast.nif`, but every creature has only a
/// `skeleton.nif` and not a `skeletonbeast.nif`. There is therefore no
/// consistent way of picking a default skeleton (creatures also have
/// instance configurable skeletons like NPCs too, they are not mesh dependent).
///
/// Ideally we would be able to use bone names in the bone assignments instead
/// of handles, until we knew what skeleton to use, but this is incompatible
/// with the requirement that the skinning information be set up during
/// `Ogre::SubMesh::load()`. On the other hand, if we modify OGRE to allow
/// specification of the blend indices instead of the bone indices then we can
/// satsify that requirement, but lose the information necessary to construct
/// the `blendIndexToBoneIndexMap` and `boneIndexToBlendIndexMap` map. Since
/// these are only required once the `Ogre::Skeleton` is known, storing both the
/// bone names and blend indices of each assignment solves the problem.
///
/// The next incompatibility is the inevitable call to
/// `Ogre::SubMesh::_compileBoneAssignments()`. Since we are manually populating
/// a vertex buffer with the position, normal, tangent, vertex colour etc.
/// vertex properties during the construction of each `Ogre::SubMesh`, and the
/// skinning information is already available at that point, it's quite
/// convenient to simultaneously put the blend indices and weights into the
/// same vertex buffer. We're prevented from doing this however as
/// `Ogre::SubMesh::_compileBoneAssignments()` *unbinds any existing buffer that
/// contains blend indices or blend weights*, then creates a new buffer to put
/// them in. Skinning data must therefore be stored in a separate vertex buffer.
///
/// \todo There is a lot more to say on this. Figuring out a fix for the above,
///       but also documenting how animations themselves work in both OGRE and,
///       more importantly, NIF files.

namespace oo {

/// \addtogroup OpenOblivionNifloader
/// @{

/// Load an animation from the given `nif` resource and attach it to the
/// `skeleton`.
Ogre::Animation *createAnimation(Ogre::Skeleton *skeleton,
                                 Ogre::NifResource *nif);

/// Load an animation from the given nif file and attach it to the `skeleton`.
Ogre::Animation *createAnimation(Ogre::Skeleton *skeleton,
                                 const std::string &nifName,
                                 const std::string &nifGroup);

/// @}

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_ANIMATION_HPP
