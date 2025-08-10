#include "uiMainWindow.h"

CMainWindow::CMainWindow(wxSimplebook* book) : wxPanel(book, wxID_ANY), m_book(book) {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    SetSizerAndFit(sizer);
}
