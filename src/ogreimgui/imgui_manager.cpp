#include "ogreimgui/imgui_manager.hpp"
#include <imgui/imgui.h>
#include <OgreCamera.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreMaterialManager.h>
#include <OgrePass.h>
#include <OgreRenderSystem.h>
#include <OgreRoot.h>
#include <OgreTechnique.h>
#include <OgreTextureManager.h>
#include <OgreViewport.h>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace Ogre {

template<> ImGuiManager *Singleton<ImGuiManager>::msSingleton = nullptr;

ImGuiManager &ImGuiManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

ImGuiManager *ImGuiManager::getSingletonPtr() {
  return msSingleton;
}

ImGuiManager::ImGuiManager() {
  ImGui::CreateContext();
  setKeyMap();
  createFontTexture();
  createMaterial();
  auto &matMgr{MaterialManager::getSingleton()};
  mRenderable.mMaterial = matMgr.getByName(mMaterialName, mResourceGroup);
}

ImGuiManager::~ImGuiManager() {
  removeMaterial();
  removeFontTexture();
  ImGui::DestroyContext();
}

void ImGuiManager::renderQueueEnded(uint8 queueGroupId,
                                    const String &invocation,
                                    bool &) {
  if (queueGroupId != RENDER_QUEUE_OVERLAY || invocation == "SHADOWS") return;

  auto *viewport{Root::getSingleton().getRenderSystem()->_getViewport()};
  if (!viewport) return;

  auto *scnMgr{viewport->getCamera()->getSceneManager()};
  if (!scnMgr) return;

  if (scnMgr->_getCurrentRenderStage() == SceneManager::IRS_RENDER_TO_TEXTURE) {
    return;
  }

  ImGui::EndFrame();
  ImGui::Render();
  render(viewport, scnMgr->getRenderQueue());
}

bool ImGuiManager::handleEvent(const sdl::Event &event) {
  switch (sdl::typeOf(event)) {
    case sdl::EventType::MouseMotion: {
      return handleEvent(event.motion);
    }
    case sdl::EventType::MouseButtonDown: {
      return handleEvent(event.button, true);
    }
    case sdl::EventType::MouseButtonUp: {
      return handleEvent(event.button, false);
    }
    case sdl::EventType::MouseWheel: {
      return handleEvent(event.wheel);
    }
    case sdl::EventType::TextInput: {
      return handleEvent(event.text);
    }
    case sdl::EventType::KeyDown: {
      return handleEvent(event.key, true);
    }
    case sdl::EventType::KeyUp: {
      return handleEvent(event.key, false);
    }
    default: {
      return false;
    }
  }
}

void ImGuiManager::newFrame(float elapsed) {
  auto &io{ImGui::GetIO()};
  auto *viewport{Root::getSingleton().getRenderSystem()->_getViewport()};
  if (!viewport) return;

  io.DeltaTime = elapsed;
  io.DisplaySize.x = viewport->getActualWidth();
  io.DisplaySize.y = viewport->getActualHeight();
  ImGui::NewFrame();
}

void ImGuiManager::setKeyMap() {
  auto &io{ImGui::GetIO()};

  io.KeyMap[ImGuiKey_Tab] = asImGui(sdl::KeyCode::Tab);
  io.KeyMap[ImGuiKey_LeftArrow] = asImGui(sdl::KeyCode::Left);
  io.KeyMap[ImGuiKey_RightArrow] = asImGui(sdl::KeyCode::Right);
  io.KeyMap[ImGuiKey_UpArrow] = asImGui(sdl::KeyCode::Up);
  io.KeyMap[ImGuiKey_DownArrow] = asImGui(sdl::KeyCode::Down);
  io.KeyMap[ImGuiKey_PageUp] = asImGui(sdl::KeyCode::Pageup);
  io.KeyMap[ImGuiKey_PageDown] = asImGui(sdl::KeyCode::Pagedown);
  io.KeyMap[ImGuiKey_Home] = asImGui(sdl::KeyCode::Home);
  io.KeyMap[ImGuiKey_End] = asImGui(sdl::KeyCode::End);
  io.KeyMap[ImGuiKey_Insert] = asImGui(sdl::KeyCode::Insert);
  io.KeyMap[ImGuiKey_Delete] = asImGui(sdl::KeyCode::Delete);
  io.KeyMap[ImGuiKey_Backspace] = asImGui(sdl::KeyCode::Backspace);
  io.KeyMap[ImGuiKey_Space] = asImGui(sdl::KeyCode::Space);
  io.KeyMap[ImGuiKey_Enter] = asImGui(sdl::KeyCode::Return);
  io.KeyMap[ImGuiKey_Escape] = asImGui(sdl::KeyCode::Escape);
  io.KeyMap[ImGuiKey_A] = asImGui(sdl::KeyCode::A);
  io.KeyMap[ImGuiKey_C] = asImGui(sdl::KeyCode::C);
  io.KeyMap[ImGuiKey_V] = asImGui(sdl::KeyCode::V);
  io.KeyMap[ImGuiKey_X] = asImGui(sdl::KeyCode::X);
  io.KeyMap[ImGuiKey_Y] = asImGui(sdl::KeyCode::Y);
  io.KeyMap[ImGuiKey_Z] = asImGui(sdl::KeyCode::Z);
}

void ImGuiManager::createFontTexture() {
  auto &io{ImGui::GetIO()};
  auto &texMgr{TextureManager::getSingleton()};

  int width{};
  int height{};
  unsigned char *pixels{};
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

  auto tex{texMgr.createManual(mFontTextureName,
                               mResourceGroup,
                               TEX_TYPE_2D,
                               static_cast<unsigned int>(width),
                               static_cast<unsigned int>(height),
                               1u,
                               MIP_DEFAULT,
                               PF_BYTE_RGBA,
                               TU_STATIC_WRITE_ONLY)};
  tex->getBuffer()->writeData(0, width * height * 4u, pixels, true);
  io.Fonts->TexID = tex.get();
  tex->load();
}

void ImGuiManager::removeFontTexture() noexcept {
  // ImGuiManager must be constructed after TextureManager, so should be---
  // and most likely is---destructed before, but it doesn't matter if it isn't
  // because TextureManager cleans up the textures for us.
  auto texMgr{TextureManager::getSingletonPtr()};
  if (texMgr) {
    // This will assert if someone has removed the texture from under us, but
    // we can assume everyone is using the api correctly...
    texMgr->remove(mFontTextureName, mResourceGroup);
  }
}

void ImGuiManager::createMaterial() {
  auto &matMgr{MaterialManager::getSingleton()};
  auto &texMgr{TextureManager::getSingleton()};

  auto mat{matMgr.getByName(mMaterialName, mResourceGroup)};
  auto pass{mat->getTechnique(0)->getPass(0)};

  if (pass->getNumTextureUnitStates() == 0) {
    auto tex{texMgr.getByName(mFontTextureName, mResourceGroup)};
    auto state{pass->createTextureUnitState()};
    state->setTexture(tex);
    state->setTextureFiltering(TFO_NONE);
  }
  mat->load();
}

void ImGuiManager::removeMaterial() noexcept {
  auto matMgr{MaterialManager::getSingletonPtr()};
  if (matMgr) {
    matMgr->remove(mMaterialName, mResourceGroup);
  }
}

void ImGuiManager::render(Viewport *viewport, RenderQueue *queue) {
  auto *draw_data{ImGui::GetDrawData()};
  auto &texMgr{TextureManager::getSingleton()};
  // TODO: This can be passed through, it does not need to be looked up again
  auto *scnMgr{viewport->getCamera()->getSceneManager()};

  // From imgui_impl_opengl3.cpp. GL takes matrix in column major so constructor
  // appears transposed.
  const float L{draw_data->DisplayPos.x};
  const float R{draw_data->DisplayPos.x + draw_data->DisplaySize.x};
  const float T{draw_data->DisplayPos.y};
  const float B{draw_data->DisplayPos.y + draw_data->DisplaySize.y};
  // @formatter:off
  const Matrix4 proj{
      2.0f / (R - L), 0.0f,           0.0f, (R + L) / (L - R),
      0.0f,           2.0f / (T - B), 0.0f, (T + B) / (B - T),
      0.0f,           0.0f,          -1.0f, 0.0f,
      0.0f,           0.0f,           0.0f, 1.0f
  };
  // @formatter:on
  mRenderable.setWorldTransforms(proj);

  // TODO: No raw loops
  for (int i = 0; i < draw_data->CmdListsCount; ++i) {
    const auto *cmdList{draw_data->CmdLists[i]};
    const auto &vtxBuf{cmdList->VtxBuffer};
    const auto &idxBuf{cmdList->IdxBuffer};
    std::ptrdiff_t idxOffset{0};
    mRenderable.generateVertexData(vtxBuf.Data,
                                   static_cast<std::size_t>(vtxBuf.size()));

    for (const auto &cmd : cmdList->CmdBuffer) {
      mRenderable.generateIndexData(idxBuf.Data + idxOffset, cmd.ElemCount);

      if (cmd.UserCallback) {
        cmd.UserCallback(cmdList, &cmd);
      } else {
        auto pass{mRenderable.mMaterial->getTechnique(0)->getPass(0)};
        auto state{pass->getTextureUnitState(0)};
        if (cmd.TextureId) {
          auto handle{reinterpret_cast<ResourceHandle>(cmd.TextureId)};
          auto res{texMgr.getByHandle(handle)};
          auto tex{std::static_pointer_cast<Texture>(res)};
          if (tex) {
            state->setTexture(tex);
          } else {
            state->setTexture(texMgr
                                  .getByName(mFontTextureName, mResourceGroup));
          }
        } else {
          state->setTexture(texMgr.getByName(mFontTextureName, mResourceGroup));
        }

        //queue->addRenderable(&mRenderable, RENDER_QUEUE_OVERLAY, 0);
        scnMgr->_injectRenderWithPass(pass, &mRenderable, false);
      }

      idxOffset += cmd.ElemCount;
    }
  }
}

bool ImGuiManager::handleEvent(const sdl::MouseMotionEvent &event) {
  auto &io{ImGui::GetIO()};

  io.MousePos.x = event.x;
  io.MousePos.y = event.y;

  return true;
}

bool ImGuiManager::handleEvent(const sdl::MouseButtonEvent &event,
                               bool isDown) {
  auto &io{ImGui::GetIO()};

  switch (sdl::mouseButtonOf(event)) {
    case sdl::MouseButton::Left: io.MouseDown[0] = isDown;
      break;
    case sdl::MouseButton::Right: io.MouseDown[1] = isDown;
      break;
    case sdl::MouseButton::Middle: io.MouseDown[2] = isDown;
      break;
    case sdl::MouseButton::Extra1: io.MouseDown[3] = isDown;
      break;
    case sdl::MouseButton::Extra2: io.MouseDown[4] = isDown;
      break;
  }

  return true;
}

bool ImGuiManager::handleEvent(const sdl::MouseWheelEvent &event) {
  auto &io{ImGui::GetIO()};

  // TODO: io.MouseWheel += event.wheel.x ?
  if (event.x > 0) ++io.MouseWheelH;
  else if (event.x < 0) --io.MouseWheelH;
  if (event.y > 0) ++io.MouseWheel;
  else if (event.y < 0) --io.MouseWheel;

  return true;
}

bool ImGuiManager::handleEvent(const sdl::TextInputEvent &event) {
  ImGui::GetIO().AddInputCharactersUTF8(event.text);

  return true;
}

bool ImGuiManager::handleEvent(const sdl::KeyboardEvent &event, bool isDown) {
  auto &io{ImGui::GetIO()};

  switch (sdl::keyCodeOf(event)) {
    case sdl::KeyCode::Lctrl: [[fallthrough]];
    case sdl::KeyCode::Rctrl: {
      io.KeyCtrl = isDown;
      break;
    }
    case sdl::KeyCode::Lshift: [[fallthrough]];
    case sdl::KeyCode::Rshift: {
      io.KeyShift = isDown;
      break;
    }
    case sdl::KeyCode::Lalt: [[fallthrough]];
    case sdl::KeyCode::Ralt: {
      io.KeyAlt = isDown;
      break;
    }
    case sdl::KeyCode::Lgui: [[fallthrough]];
    case sdl::KeyCode::Rgui: {
      io.KeySuper = isDown;
      break;
    }
    default: {
      auto scanCode{asImGui(sdl::keyCodeOf(event))};
      if (scanCode < IM_ARRAYSIZE(io.KeysDown)) {
        io.KeysDown[scanCode] = isDown;
        return true;
      }
      return false;
    }
  }
  return true;
}

int ImGuiManager::asImGui(sdl::KeyCode keyCode) const noexcept {
  auto uKeyCode{static_cast<std::underlying_type_t<sdl::KeyCode>>(keyCode)};
  auto scanCode{SDL_GetScancodeFromKey(uKeyCode)};
  return scanCode;
}

} // namespace Ogre