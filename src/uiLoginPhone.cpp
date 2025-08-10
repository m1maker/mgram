#include "uiLoginPhone.h"

#include "uiLogin.h"
#include "uiMainFrame.h"
#include "uiMainWindow.h"

#include <td/telegram/td_api.hpp>
#include <wx/msgdlg.h>

extern CMainFrame* g_mainFrame;

CLoginPhoneWindow::CLoginPhoneWindow(wxSimplebook* book, TdManager& manager)
    : wxPanel(book, wxID_ANY), m_book(book), m_loginState(LOGIN_PHONE) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    m_label = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_entry = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_next = new wxButton(this, wxID_ANY, "&Next");
    m_cancel = new wxButton(this, wxID_ANY, "&Cancel");

    mainSizer->Add(m_label, 0, wxALL, 5);
    mainSizer->Add(m_entry, 0, wxEXPAND | wxALL, 5);

    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_next, 0, wxALL, 5);
    buttonSizer->Add(m_cancel, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxTOP, 10);

    SetSizerAndFit(mainSizer);

    m_next->Bind(wxEVT_BUTTON, &CLoginPhoneWindow::OnNextPressed, this);
    m_cancel->Bind(wxEVT_BUTTON, &CLoginPhoneWindow::OnCancelPressed, this);
    m_entry->Bind(wxEVT_TEXT_ENTER, &CLoginPhoneWindow::OnNextPressed, this);

    SwitchLoginState(m_loginState);
}

void CLoginPhoneWindow::SwitchLoginState(const ELoginState& newState) {
    m_loginState = newState;
    m_entry->SetWindowStyle(m_entry->GetWindowStyle() & ~wxTE_PASSWORD);

    switch (m_loginState) {
        case LOGIN_PHONE:
            m_label->SetLabelText("Enter your phone number:");
            m_entry->SetValue("");
            m_entry->SetHint("+12025550132");
            break;
        case LOGIN_CODE:
            m_label->SetLabelText("Enter the code you received:");
            m_entry->SetValue("");
            m_entry->SetHint("12345");
            break;
        case LOGIN_PASSWORD:
            m_label->SetLabelText("Enter your two-factor authentication password:");
            m_entry->SetValue("");
            m_entry->SetHint("Password");
            m_entry->SetWindowStyle(m_entry->GetWindowStyle() | wxTE_PASSWORD);
            break;
    }

    m_entry->Enable();
    m_next->Enable();
    m_entry->SetFocus();
    Layout();
}

void CLoginPhoneWindow::OnNextPressed(wxCommandEvent& event) {
    wxString value = m_entry->GetValue().Trim();

    if (value.IsEmpty()) {
        wxMessageBox("The field cannot be empty.", "Input Error", wxOK | wxICON_WARNING);
        return;
    }

    m_next->Disable();
    m_entry->Disable();

    auto response_handler = [this](td::td_api::object_ptr<td::td_api::Object> object) {
        if (object->get_id() == td::td_api::error::ID) {
            auto error = td::td_api::move_object_as<td::td_api::error>(object);
            wxString error_message = wxString::Format("Error: %s (Code: %d)", error->message_, error->code_);

            CallAfter([this, error_message]() {
                wxMessageBox(error_message, "Login Failed", wxOK | wxICON_ERROR);
                m_next->Enable();
                m_entry->Enable();
                m_entry->SelectAll();
                m_entry->SetFocus();
            });
        }
    };

    if (m_loginState == LOGIN_PHONE) {
        auto set_phone_number = td::td_api::make_object<td::td_api::setAuthenticationPhoneNumber>();
        set_phone_number->phone_number_ = value.ToStdString();
        g_mainFrame->getTdManager()->send(std::move(set_phone_number), response_handler);
    } else if (m_loginState == LOGIN_CODE) {
        auto check_code = td::td_api::make_object<td::td_api::checkAuthenticationCode>();
        check_code->code_ = value.ToStdString();
        g_mainFrame->getTdManager()->send(std::move(check_code), response_handler);
    } else if (m_loginState == LOGIN_PASSWORD) {
        auto check_password = td::td_api::make_object<td::td_api::checkAuthenticationPassword>();
        check_password->password_ = value.ToStdString();
        g_mainFrame->getTdManager()->send(std::move(check_password), response_handler);
    }
}

void CLoginPhoneWindow::OnCancelPressed(wxCommandEvent& event) {
    SwitchLoginState(LOGIN_PHONE);
    g_mainFrame->getTdManager()->send(td::td_api::make_object<td::td_api::logOut>());

    if (m_book && g_mainFrame && g_mainFrame->m_loginWindow) {
        if (m_book->FindPage(g_mainFrame->m_loginWindow) != wxNOT_FOUND) {
            m_book->SetSelection(m_book->FindPage(g_mainFrame->m_loginWindow));
        }
    }
}
