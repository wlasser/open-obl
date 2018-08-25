#ifndef OPENOBLIVION_NIF_NIF_HPP
#define OPENOBLIVION_NIF_NIF_HPP

#include "nif/compound.hpp"
#include "nif/niobject.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <array>
#include <istream>
#include <iostream>
#include <optional>
#include <string>

namespace nif {

class NifModel {
 public:
  using Copyright = std::array<std::string, 3>;
  // Vertex properties are required to be copy constructible, so we cannot use
  // std::unique_ptr.
  using Block = std::shared_ptr<nif::NiObject>;
  using BlockGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                           boost::bidirectionalS, Block>;

  Version version{};
  std::optional<Version> userVersion{};
  std::optional<Version> userVersion2{};

  // Forest of blocks.
  // There is an edge from block A to block B if B is a child of A.
  BlockGraph blocks{};
  std::vector<std::string> possibleBlockTypes{};
  std::vector<std::string *> blockTypes{};
  std::vector<uint32_t> groups{};

  explicit NifModel(std::istream &is);

 private:
  template<class T>
  // Add an edge from u to v. Does not check that v is a valid reference.
  void addEdge(BlockGraph::vertex_descriptor u, basic::Ref<T> v) {
    auto vDesc = static_cast<BlockGraph::vertex_descriptor>(
        static_cast<int32_t>(v));
    boost::add_edge(u, vDesc, blocks);
  }
};

} // namespace nif

#endif //OPENOBLIVION_NIF_NIF_HPP
