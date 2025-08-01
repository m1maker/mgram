#ifndef UI_LOGIN_H
#define UI_LOGIN_H
#include <wx/wx.h>

class CMainFrame : public wxFrame {
  public:
    CMainFrame(const wxString& title);

  private:
    wxPanel* m_panel;
    wxButton* m_login;
};

class CMgramEntry : public wxApp {
  public:
    virtual bool OnInit() override;
};
#endif
