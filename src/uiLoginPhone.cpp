#include "uiLoginPhone.h"

#include "uiMainFrame.h"

#include <td/telegram/td_api.h>

CLoginPhoneWindow::CLoginPhoneWindow(wxSimplebook* book) : wxPanel(book, wxID_ANY), m_book(book) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* phoneNumberLabel = new wxStaticText(this, wxID_ANY, "Phone Number");
    m_phoneNumber = new wxTextCtrl(this, wxID_ANY, "+", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_next = new wxButton(this, wxID_ANY, "&Next");
    m_cancel = new wxButton(this, wxID_ANY, "&Cancel");

    mainSizer->Add(phoneNumberLabel, 0, wxALL, 5);
    mainSizer->Add(m_phoneNumber, 0, wxEXPAND | wxALL, 5);

    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_next, 0, wxALL, 5);
    buttonSizer->Add(m_cancel, 0, wxALL, 5);

    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxTOP, 10);

    SetSizerAndFit(mainSizer);

    m_next->Bind(wxEVT_BUTTON, &CLoginPhoneWindow::OnNextPressed, this);
    m_cancel->Bind(wxEVT_BUTTON, &CLoginPhoneWindow::OnCancelPressed, this);
    m_phoneNumber->SetFocus();
}

extern CMainFrame* g_mainFrame;

void CLoginPhoneWindow::OnNextPressed(wxCommandEvent& event) {
    wxString phoneNumber = m_phoneNumber->GetValue();

    auto set_phone_number = td::td_api::make_object<td::td_api::setAuthenticationPhoneNumber>();
    set_phone_number->phone_number_ = phoneNumber.ToStdString();

    g_mainFrame->getTdManager()->send(std::move(set_phone_number),
                                      [](td::td_api::object_ptr<td::td_api::Object> object) {});
}

void CLoginPhoneWindow::OnCancelPressed(wxCommandEvent& event) {
    if (m_book->FindPage(g_mainFrame->m_loginWindow) != wxNOT_FOUND) {
        m_book->SetSelection(m_book->FindPage(g_mainFrame->m_loginWindow));
    }
}
