#define G_LOG_DOMAIN "BsaBrowser"
#include "bsa/application.hpp"
#include <gsl/gsl>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treemodel.h>

namespace bsa {

ApplicationWindow::ApplicationWindow(BaseObjectType *cobject,
                                     const Glib::RefPtr<Gtk::Builder> &builder)
    : Gtk::ApplicationWindow(cobject), mBuilder(builder) {
  mBuilder->get_widget("BsaFileTreeStack", mBsaStack);
  if (!mBsaStack) {
    throw std::runtime_error("No \"BsaFileTreeStack\" in window.glade");
  }
}

gsl::not_null<ApplicationWindow *> ApplicationWindow::create() {
  auto builder = Gtk::Builder::create_from_resource(
      "/com/piepenguin/bsabrowser/window.glade");

  ApplicationWindow *window{nullptr};
  builder->get_widget_derived("BsaApplicationWindow", window);

  if (!window) {
    throw std::runtime_error("No \"BsaApplicationWindow\" in window.glade");
  }

  return gsl::make_not_null(window);
}

void ApplicationWindow::open_file_view(const Glib::RefPtr<Gio::File> &file) {
  BsaTreePage page{};

  page.basename = file->get_basename();

  page.scrolledWindow = Gtk::manage(new Gtk::ScrolledWindow());
  page.scrolledWindow->set_vexpand(true);
  page.scrolledWindow->show();

  page.treeView = Gtk::manage(new Gtk::TreeView());
  page.treeStore = Gtk::TreeStore::create(mColumns);
  page.treeView->set_model(page.treeStore);

  page.treeView->append_column("Name", mColumns.mColName);
  page.treeView->append_column("Size", mColumns.mColSize);
  page.treeView->append_column("Compressed?", mColumns.mColCompressed);
  page.treeView->append_column_numeric("Offset", mColumns.mColOffset, "0x%x");
  page.treeView->append_column_numeric("Hash", mColumns.mColHash, "0x%x");

  page.treeView->show();

  page.scrolledWindow->add(*page.treeView);

  page.reader = std::make_unique<BsaReader>(file->get_parse_name());
  for (const FolderRecord &folder : *page.reader) {
    const Gtk::TreeRow &row = *(page.treeStore->append());
    uint64_t folderHash = genHash(folder.name, HashType::Folder);

    row[mColumns.mColName] = folder.name;
    row[mColumns.mColHash] = folderHash;

    for (const auto &filename : folder.files) {
      const Gtk::TreeRow &childRow = *(page.treeStore->append(row.children()));
      uint64_t fileHash = genHash(filename, HashType::File);
      const auto fileRecord = page.reader->getRecord(folderHash, fileHash);
      childRow[mColumns.mColName] = filename;
      childRow[mColumns.mColSize] = fileRecord->size;
      childRow[mColumns.mColCompressed] = fileRecord->compressed;
      childRow[mColumns.mColOffset] = fileRecord->offset;
      childRow[mColumns.mColHash] = fileHash;
    }
  }

  auto &pageRef = mPages.emplace_back(std::move(page));
  mBsaStack->add(*pageRef.scrolledWindow, pageRef.basename, pageRef.basename);
  mBsaStack->set_visible_child(*pageRef.scrolledWindow);
}

Application::Application() : Gtk::Application("com.piepenguin.bsabrowser",
                                              Gio::APPLICATION_HANDLES_OPEN) {}

void Application::on_startup() {
  Gtk::Application::on_startup();

  add_action("file.open",
             sigc::mem_fun(*this, &Application::on_action_file_open));
  add_action("quit",
             sigc::mem_fun(*this, &Application::on_action_quit));
  set_accel_for_action("app.quit", "<Ctrl>Q");
}

void Application::on_activate() {
  try {
    auto window = create_appwindow();
    window->present();
    // Application will terminate if an exception is thrown, since no window
    // will be open.
  } catch (const Glib::Error &e) {
    g_error("Application::on_activate(): %s", e.what().c_str());
  } catch (const std::exception &e) {
    g_error("Application::on_activate(): %s", e.what());
  }
}

void Application::on_open(const Gio::Application::type_vec_files &files,
                          const Glib::ustring &hint) {

  // Attempt to use the first application window, but if none exist then create
  // a new one.
  ApplicationWindow *window{nullptr};

  auto windows = get_windows();
  if (!windows.empty()) {
    window = dynamic_cast<ApplicationWindow *>(windows.front());
  }

  try {
    if (!window) {
      window = create_appwindow();
    }

    // Create a separate view in the window for each file
    for (const auto &file : files) {
      window->open_file_view(file);
    }

    window->present();
  } catch (const Glib::Error &e) {
    g_error("Application::on_open(): %s", e.what().c_str());
  } catch (const std::exception &e) {
    g_error("Application::on_open(): %s", e.what());
  }
}

Glib::RefPtr<Application> Application::create() {
  return Glib::RefPtr<Application>(new Application());
}

gsl::not_null<ApplicationWindow *> Application::create_appwindow() {
  auto window = ApplicationWindow::create();

  // Application will close when all added windows are closed
  add_window(*window);

  // Call our on_hide_window when the window is hidden
  window->signal_hide().connect(sigc::bind<Gtk::Window *>(
      sigc::mem_fun(*this, &Application::on_hide_window),
      window));

  return window;
}

void Application::on_hide_window(Gtk::Window *window) {
  delete window;
}

void Application::on_action_file_open() {
  Gtk::FileChooserDialog dialog("Open File", Gtk::FILE_CHOOSER_ACTION_OPEN);
  auto window = dynamic_cast<ApplicationWindow *>(get_active_window());
  if (!window) {
    g_error("Application::on_action_file_open(): No active window");
  }
  dialog.set_transient_for(*window);

  dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
  dialog.add_button("Open", Gtk::RESPONSE_OK);

  int result = dialog.run();
  switch (result) {
    case Gtk::RESPONSE_OK: {
      window->open_file_view(dialog.get_file());
      break;
    }
    default: break;
  }
}

void Application::on_action_quit() {
  auto windows = get_windows();
  for (const auto &window : windows) {
    window->hide();
  }
  quit();
}

} // namespace bsa
