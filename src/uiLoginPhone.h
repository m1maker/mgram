#ifndef UI_LOGIN_PHONE_H
#define UI_LOGIN_PHONE_H

#include <wx/simplebook.h>
#include <wx/wx.h>

class CLoginPhoneWindow : public wxPanel {
  public:
    CLoginPhoneWindow(wxSimplebook* book);

    void OnNextPressed(wxCommandEvent& event);
    void OnCancelPressed(wxCommandEvent& event);

  private:
    wxTextCtrl* m_phoneNumber;
    wxButton* m_next;
    wxButton* m_cancel;
    wxSimplebook* m_book;
};

#endif
