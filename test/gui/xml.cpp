#include "gui/xml.hpp"
#include <catch2/catch.hpp>
#include <pugixml.hpp>
#include <map>
#include <sstream>

TEST_CASE("can find closest node", "[gui]") {
  std::istringstream is{R"xml(
<node name="A" val="1">
  <node name="B" val="2">
    <node name="E" val="3" />
    <node name="F" val="5" />
    <node name="G" val="1" />
  </node>

  <node name="C" val="4">
    <node name="H" val="5" />
    <node name="I" val="2">
      <node name="L" val="1" />
    </node>
  </node>

  <node name="D" val="3">
    <node name="J" val="3" />
    <node name="K" val="2" />
  </node>
</node>
  )xml"};
  pugi::xml_document doc{};
  REQUIRE(doc.load(is));

  // Helper functions for getting "name" and "val" attributes
  auto getName = [](pugi::xml_node n) -> std::string {
    return n.attribute("name").value();
  };
  auto getVal = [](pugi::xml_node n) -> int {
    return n.attribute("val").as_int();
  };

  // Build map of node names to nodes, for convenience
  std::map<char, pugi::xml_node> nodes{};
  for (char c{'A'}; c <= 'L'; ++c) {
    nodes[c] = doc.find_node([c, &getName](pugi::xml_node n) {
      auto name{getName(n)};
      return !name.empty() && name[0] == c;
    });
  }

  {
    // Every node should find itself, matching its own value
    for (auto c{'A'}; c <= 'L'; ++c) {
      const auto node{gui::findClosestNode(nodes[c], [&](pugi::xml_node n) {
        return getVal(n) == getVal(nodes[c]);
      })};
      REQUIRE(getName(node) == getName(nodes[c]));
    }
  }

  {
    // Find I from L, not B or K
    const auto node{gui::findClosestNode(nodes['L'], [&](pugi::xml_node n) {
      return getVal(n) == 2;
    })};
    REQUIRE(getName(node) == "I");
  }

  {
    // Find D from I, not J or E
    const auto node{gui::findClosestNode(nodes['I'], [&](pugi::xml_node n) {
      return getVal(n) == 3;
    })};
    REQUIRE(getName(node) == "D");
  }

  {
    // Find D from A, not J or E
    const auto node{gui::findClosestNode(nodes['A'], [&](pugi::xml_node n) {
      return getVal(n) == 3;
    })};
    REQUIRE(getName(node) == "D");
  }

  {
    // Find E from B, not J or E
    const auto node{gui::findClosestNode(nodes['B'], [&](pugi::xml_node n) {
      return getVal(n) == 3;
    })};
    REQUIRE(getName(node) == "E");
  }

  {
    // Find F or H from A, we don't care which
    const auto node{gui::findClosestNode(nodes['A'], [&](pugi::xml_node n) {
      return getVal(n) == 5;
    })};
    REQUIRE((getName(node) == "F" || getName(node) == "H"));
  }
}
