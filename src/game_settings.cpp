#include "game_settings.hpp"
#include "record/records.hpp"

namespace oo {

GameSettings &GameSettings::getSingleton() {
  // Cannot std::make_unique on a private constructor
  // TODO: Abseil TOTW #134
  static std::unique_ptr<GameSettings> instance{new GameSettings()};
  return *instance;
}

void GameSettings::load(const char *filename, bool /*overwrite*/) {
  // The game ini files has a duplicate key General.STestFile1 and a
  // multiline string GeneralWarnings.SMasterMismatchWarning, which are not
  // supported by the property_tree parser.
  // TODO: Replace ini parser with a custom parser (boost::spirit?)
  boost::property_tree::ini_parser::read_ini(filename, tree);
}

void GameSettings::load(const record::GMST &gmst, bool overwrite) {
  std::string key = gmst.editorId.data;
  if (key.empty()) return;
  // put overwrites without question, so we have to query existence first if
  // overwrite is not set.
  switch (key[0]) {
    case 'f':
      if (overwrite || !tree.get_optional<float>(key)) {
        tree.put<float>(key, gmst.value.data.f);
      }
      break;
    case 'i':
      if (overwrite || !tree.get_optional<int>(key)) {
        tree.put<int>(key, gmst.value.data.i);
      }
      break;
    case 's':
      if (overwrite || !tree.get_optional<std::string>(key)) {
        const auto &arr = gmst.value.data.s;
        std::string s(arr.begin(), arr.end());
        tree.put<std::string>(key, s);
      }
      break;
    default: break;
  }
}

oo::Path GameSettings::getFont(int index) const {
  auto fnt{get<std::string>("Fonts.SFontFile_" + std::to_string(index))};
  if (fnt.has_value()) return oo::Path{fnt.value()};

  fnt = get<std::string>("Fonts.SFontFile_1");
  if (fnt.has_value()) return oo::Path{fnt.value()};

  return oo::Path{"libertine"};
}

} // namespace oo