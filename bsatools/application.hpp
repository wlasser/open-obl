#ifndef OPENOBLIVION_BSA_APPLICATION_HPP
#define OPENOBLIVION_BSA_APPLICATION_HPP

#include "bsa/bsa.hpp"
#include <gsl/gsl>
#include <gtkmm/application.h>
#include <gtkmm/applicationwindow.h>
#include <gtkmm/builder.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/stack.h>
#include <gtkmm/treemodel.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeview.h>
#include <memory>

namespace bsa {

class BsaModel : public Gtk::TreeModelColumnRecord {
 public:

  BsaModel() {
    add(mColName);
    add(mColSize);
    add(mColCompressed);
    add(mColOffset);
    add(mColHash);
  }

  Gtk::TreeModelColumn<Glib::ustring> mColName{};
  Gtk::TreeModelColumn<uint32_t> mColSize{};
  Gtk::TreeModelColumn<bool> mColCompressed{};
  Gtk::TreeModelColumn<uint32_t> mColOffset{};
  Gtk::TreeModelColumn<uint64_t> mColHash{};
};

struct BsaTreePage {
  // Name of the bsa file
  std::string basename{};

  // View
  Gtk::ScrolledWindow *scrolledWindow{};
  Gtk::TreeView *treeView{};

  // Model
  Glib::RefPtr<Gtk::TreeStore> treeStore{};
  std::unique_ptr<BsaReader> reader{};
};

class ApplicationWindow : public Gtk::ApplicationWindow {
 public:
  ApplicationWindow(BaseObjectType *cobject,
                    const Glib::RefPtr<Gtk::Builder> &builder);

  static gsl::not_null<ApplicationWindow *> create();

  void open_file_view(const Glib::RefPtr<Gio::File> &file);

 private:
  Glib::RefPtr<Gtk::Builder> mBuilder;
  // Pointer to the stack of open bsa trees
  Gtk::Stack *mBsaStack{nullptr};
  // Tree store columns, shared among all tree stores
  BsaModel mColumns{};
  // Pages of open BSA files
  std::vector<BsaTreePage> mPages{};
};

class Application : public Gtk::Application {
 private:
  Application();

  void on_startup() override;
  void on_activate() override;
  void on_open(const Gio::Application::type_vec_files &files,
               const Glib::ustring &hint) override;

 public:
  static Glib::RefPtr<Application> create();

 private:
  gsl::not_null<ApplicationWindow *> create_appwindow();
  void on_hide_window(Gtk::Window *window);
  void on_action_file_open();
  void on_action_quit();
};

} // namespace bsa

#endif // OPENOBLIVION_BSA_APPLICATION_HPP
