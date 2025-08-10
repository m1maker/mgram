#ifndef UI_LOGIN_PHONE_H
#define UI_LOGIN_PHONE_H
#include "tdManager.h"

#include <wx/simplebook.h>
#include <wx/wx.h>

class CLoginPhoneWindow : public wxPanel {
  public:
    enum ELoginState {
        LOGIN_PHONE,
        LOGIN_CODE,
        LOGIN_PASSWORD
    };

    CLoginPhoneWindow(wxSimplebook* book, TdManager& manager);

    void OnNextPressed(wxCommandEvent& event);
    void OnCancelPressed(wxCommandEvent& event);

    void SwitchLoginState(const ELoginState& newState);

  private:
    wxStaticText* m_label;
    wxTextCtrl* m_entry;
    wxButton* m_next;
    wxButton* m_cancel;
    wxSimplebook* m_book;
    ELoginState m_loginState;
};

#endif
