#ifndef UI_LOGIN_H
#define UI_LOGIN_H
#include <wx/wx.h>

class CLoginWindow : public wxPanel {
  public:
    CLoginWindow();

  private:
    wxButton* m_login;
};

#endif