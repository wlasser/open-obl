#include "gui.hpp"
#include "job/job.hpp"
#include <imgui/imgui.h>
#include <OgreRoot.h>

namespace oo {

void quit() {
  Ogre::Root::getSingleton().queueEndRendering();
}

void showMainMenuBar() {
  static bool fileOpenDialogVisible = false;

  if (fileOpenDialogVisible) showFileOpenDialog(&fileOpenDialogVisible);

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Open")) {
        oo::JobManager::runJob([]() {
          boost::this_fiber::sleep_for(std::chrono::seconds(2));
          std::cerr << "Hello\n";
          fileOpenDialogVisible = true;
        });
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Exit")) quit();

      ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
  }
}

void showFileOpenDialog(bool *p_open) {
  ImGui::Begin("file_open_dialog", p_open);
  ImGui::Text("Which file?");
  ImGui::End();
}

} // namespace oo