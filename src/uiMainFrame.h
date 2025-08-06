#ifndef UI_MAIN_FRAME_H
#define UI_MAIN_FRAME_H

#include "tdManager.h"
#include "uiLogin.h"
#include "uiLoginPhone.h"

#include <wx/simplebook.h>
#include <wx/wx.h>

class CMainFrame : public wxFrame {
  public:
    CLoginWindow* m_loginWindow;
    CLoginPhoneWindow* m_loginPhoneWindow;

    CMainFrame(const wxString& title);
    TdManager* getTdManager() { return &m_tdManager; }

  private:
    TdManager m_tdManager;
    wxSimplebook* m_book;
};

extern CMainFrame* g_mainFrame;

class CMgramEntry : public wxApp {
  public:
    virtual bool OnInit() override;
};

#endif
