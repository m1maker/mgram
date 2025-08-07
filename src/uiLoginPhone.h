#ifndef UI_LOGIN_PHONE_H
#define UI_LOGIN_PHONE_H

#include <wx/simplebook.h>
#include <wx/wx.h>

class CLoginPhoneWindow : public wxPanel {
    enum ELoginState {
        LOGIN_PHONE,
        LOGIN_CODE,
        LOGIN_PASSWORD
    };

    ELoginState m_loginState;

  public:
    CLoginPhoneWindow(wxSimplebook* book);

    void OnNextPressed(wxCommandEvent& event);
    void OnCancelPressed(wxCommandEvent& event);

    void SwitchLoginState(const ELoginState& newState);

  private:
    wxStaticText* m_label;
    wxTextCtrl* m_entry;
    wxButton* m_next;
    wxButton* m_cancel;
    wxSimplebook* m_book;
};

#endif
