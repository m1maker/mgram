#ifndef UI_MAIN_FRAME_H
#define UI_MAIN_FRAME_H

#include "tdManager.h"

#include <wx/simplebook.h>
#include <wx/wx.h>

class CLoginWindow;
class CLoginPhoneWindow;
class CMainWindow;

class CMainFrame : public wxFrame {
  public:
    CLoginWindow* m_loginWindow;
    CLoginPhoneWindow* m_loginPhoneWindow;
    CMainWindow* m_mainWindow;
    wxSimplebook* m_book;

    CMainFrame(const wxString& title);
    TdManager* getTdManager() { return &m_tdManager; }

  private:
    void InitializeTdlib();
    void OnAuthorizationStateUpdate(td::td_api::object_ptr<td::td_api::Object> update);

    TdManager m_tdManager;
};

extern CMainFrame* g_mainFrame;

class CMgramEntry : public wxApp {
  public:
    virtual bool OnInit() override;
};

#endif
