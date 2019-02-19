# Terrain

## Land Textures

This was originally just a comment in `oo::World::makeCellGrid` documenting how
the `record::LAND` structure and how to obtain OGRE-compatible terrain layers
from it, but it got so long it deserved to become documentation.

A quick overview of how we *want* to peform terrain layering, in a way supported
by OGRE; each layer has a diffuse and normal map which are passed to the shader
in two different samplers. Each layer above the first (`layer0`) has a blend map
which assigns an opacity of the layer texture to each vertex in the terrain. The
blend maps are packed 4 layers at a time into an RGBA texture and passed to the
shader as another sampler. The shader then blends the layers together using the
'over' alpha-compositing operator.

`record::LAND` records on the other hand appear to have two different texturing
schemes. The simple scheme is a single `record::VTEX` which stores 64
`record::LTEX` ids for the entire cell, presumably an assignment of a `layer0`
texture to every 4 grid points that are then interpolated somehow between the
cells. This scheme---whilst present in the game data---does not appear to be
recognised by the engine. For example, `BravilRiver02` has a `record::VTEX`
filled with zeroes and `TerrainHDRock01`, but that `record::LTEX` does not
appear in either `BravilRiver02` or its parent cell `BravilRiver21`.

The second scheme is to split the cell into quadrants. We will start by
considering each quadrant separately, as Oblivion presumably does. Up to four
`record::BTXT` specify the `layer0` texture of a different quadrant. There are
then some number of `record::ATXT`/`record::VTXT` pairs which specify the
opacity of a fixed texture on a fixed layer for some collection of points in a
fixed quadrant. To a `record::ATXT`, `layer0` is actually `layer1`, as
`record::ATXT` records never describe a base layer, only a blend layer.

Assuming that each layer in the quadrant has a unique `record::LTEX` (otherwise,
what is a layer?), we can assume that no two `record::ATXT` share the same layer
and quadrant. If they did, they would have to share the same `record::LTEX`, but
then they could be merged together. Thus, there is a function mapping some
subset of the positive layer numbers to textures. It is still possible for two
`record::ATXT` to have the same texture, provided they are on different layers.
From a physical point of view this should not happen, but from an interface
point of view the lack of an eraser tool forces the artist to reapply the
texture from a lower layer onto a new layer above the current one in lieu of
erasing. Provided the reapplication is fully opaque we can just discard all
intervening layers instead of creating a new layer, but if not then we run into
problems; one may hope that the reapplication can be merged into the original
layer, after a modification of the alpha values of both the intermediate layers
and the original layer, but this does not seem to be possible with allowing for
opacities greater than 1. Instead, we will assume that no two layers have the
same texture.

It is not necessarily the case that every quadrant without a `record::BTXT`
is missing one because every vertex is fully covered by some combination of
`record::ATXT`/`record::VTXT` pairs. Indeed, newly created exterior cells do
not have any pairs or `record::BTXT` in their `record::LAND`. Nonetheless they
are displayed with a default texture that is not present as an `record::LTEX`
record. We interpret this as each quadrant having an implicit `record::BTXT`
with a hardcoded `layer0` texture that is replaced by an explicit
`record::BTXT`, if present.

Per quadrant then, we are guaranteed to have a base texture and zero or more
blend layers consisting of a single texture with varying opacity applied to
each point (any points not mentioned in the `record::ATXT` for that layer are
assigned opacity zero). This is precisely what OGRE expects, and we are done.

Now consider the cell as a whole. Since the quadrant split is abstracted away
from the artist, it is reasonable to assume that many of the textures used in
the cell will be shared across the quadrants with the same layer numbers. It is
therefore likely that most layers extend to other quadrants by simply taking
the union of their blend maps.

Unfortunately, 'most' and 'likely' are not good enough. Some textures will be
present in less than all quadrants, and the layer numbers may not line up. A
simple solution is for any layer number where two or more quadrants disagree on
the texture, create a new texture that is the concatenation of all the quadrant
textures, then taking the union of the blend maps for that layer. In the shader,
the uv coordinates can be used to determine the quadrant and the correct part of
the texture can be sampled and scaled appropriately. This approach is simple,
but requires generating a texture for each inconsistently textured layer.

A more complicated approach is to attempt to move quadrant layers around to
create a consistent layer numbering. For instance, quadrants `AB`, `0A`, `B`,
and `0C` can all be moved to the form `0ABC` by promoting all layers above some
layer number upwards and filling in the hole in the blend map with zeroes,
as well as filling any blend maps above a layer up to the highest. Since this
cannot change the ordering of layers within a quadrant, a necessary condition
for this to work is that there must exist some ordering of all the layers of
all the quadrants that is consistent with each quadrant's ordering; at the end
we arrive at one of those orderings. For instance, it is not possible for
quadrants `AB` and `BA` to both be present, as their orderings are incompatible.

Abstractly, each quadrant defines a strict partial order on the set of all
layer textures, which must be combined into a strict total order. Let the
textures be the vertices of a directed graph and draw an edge from texture
`A` to texture `B` iff layer `A` is below layer `B` in some quadrant. If the
graph is not a DAG, no ordering is possible. Topologically sort the graph to
obtain a common layer order. For each layer in the common order, form a blend
map by taking, from each quadrant, the blend map of the layer with the same
texture---if it exists---and a zero blend map if it doesn't.
