#include "uiLogin.h"

#include "uiMainFrame.h"

CLoginWindow::CLoginWindow() : wxPanel(g_mainFrame, wxID_ANY) {
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    auto* textSizer = new wxBoxSizer(wxVERTICAL);
    auto* optionSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* text = new wxTextCtrl(this, wxID_ANY, "Log in to Telegram by phone number or QR Code", wxDefaultPosition,
                                wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE);
    m_login = new wxButton(this, wxID_ANY, "Log in by phone number");
    textSizer->Add(text);
    optionSizer->Add(m_login);
    mainSizer->Add(textSizer);
    mainSizer->Add(optionSizer);
    this->SetSizer(mainSizer);
    text->SetFocus();
}
