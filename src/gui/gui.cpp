#include "gui/gui_impl.hpp"
#include "gui/logging.hpp"
#include "gui/xml.hpp"

namespace gui {

std::optional<MenuContext> loadMenu(const std::string &filename,
                                    const std::string &stringsFilename) {
  auto doc{gui::readXmlDocument(filename)};
  auto stringsDoc{gui::readXmlDocument(stringsFilename)};
  doc.save_file("out.xml", "  ");
  return gui::loadMenu(std::move(doc), std::move(stringsDoc));
}

MenuContext::MenuContext(std::unique_ptr<Impl> impl) : mImpl(std::move(impl)) {}

MenuContext::~MenuContext() = default;

MenuContext::MenuContext(MenuContext &&) noexcept = default;
MenuContext &MenuContext::operator=(MenuContext &&) noexcept = default;

void MenuContext::update() { mImpl->update(); }

void MenuContext::clearEvents() { mImpl->clearEvents(); }

Ogre::Overlay *MenuContext::getOverlay() const { return mImpl->getOverlay(); }

Ogre::Vector2 MenuContext::normalizeCoordinates(int32_t x, int32_t y) const {
  return mImpl->normalizeCoordinates(x, y);
}

void MenuContext::set_user(int index, gui::UiElement::UserValue value) {
  mImpl->set_user(index, std::move(value));
}

gui::UiElement::UserValue MenuContext::get_user(int index) {
  return mImpl->get_user(index);
}

const gui::UiElement *MenuContext::getElementWithId(int id) const {
  return mImpl->getElementWithId(id);
}

} // namespace gui