#ifndef UI_LOGIN_H
#define UI_LOGIN_H
#include <wx/wx.h>

class CLoginWindow : public wxPanel {
  public:
    CLoginWindow();

    void OnLoginPressed(wxCommandEvent& event);

  private:
    wxButton* m_login;
    wxCheckBox* m_useTestDataCenter;
};

#endif