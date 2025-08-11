#ifndef UI_LOGIN_H
#define UI_LOGIN_H

#include <wx/simplebook.h>
#include <wx/wx.h>

class CLoginWindow final : public wxPanel {
  public:
    CLoginWindow(wxSimplebook* book);

    void OnLoginPressed(wxCommandEvent& event);

  private:
    wxButton* m_login;
    wxCheckBox* m_useTestDataCenter;
    wxSimplebook* m_book;
};

#endif
