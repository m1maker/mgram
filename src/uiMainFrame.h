#ifndef UI_MAIN_FRAME_H
#define UI_MAIN_FRAME_H

#include "tdManager.h"

#include <wx/ipc.h>
#include <wx/simplebook.h>
#include <wx/snglinst.h>
#include <wx/wx.h>

class CLoginWindow;
class CLoginPhoneWindow;
class CMainWindow;

class CMainFrame final : public wxFrame {
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

    void OnClose(wxCloseEvent& event);

    TdManager m_tdManager;
    DECLARE_EVENT_TABLE()
};

extern CMainFrame* g_mainFrame;

class CMgramEntry final : public wxApp {
  public:
    virtual bool OnInit() override;
    virtual int OnExit() override;

  private:
    wxSingleInstanceChecker* m_instanceChecker;
    wxServer* m_server;
};

class CMgramConnection final : public wxConnection {
  public:
    CMgramConnection() = default;
    ~CMgramConnection() = default;

    bool OnExec(const wxString& topic, const wxString& data) override;
};

class CMgramServer final : public wxServer {
  public:
    CMgramServer() = default;
    wxConnectionBase* OnAcceptConnection(const wxString& topic) override;
};
#endif
