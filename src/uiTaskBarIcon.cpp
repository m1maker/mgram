#include "uiTaskBarIcon.h"

#include "uiMainFrame.h"

BEGIN_EVENT_TABLE(CMgramTaskBarIcon, wxTaskBarIcon)
EVT_TASKBAR_LEFT_DCLICK(CMgramTaskBarIcon::OnLeftDoubleClick)
EVT_MENU(wxID_ANY, CMgramTaskBarIcon::OnMenuRestore)
EVT_MENU(wxID_EXIT, CMgramTaskBarIcon::OnMenuExit)
END_EVENT_TABLE()

CMgramTaskBarIcon::CMgramTaskBarIcon(CMainFrame* frame) : wxTaskBarIcon(), m_frame(frame) {}

void CMgramTaskBarIcon::OnLeftDoubleClick(wxTaskBarIconEvent& event) {
    m_frame->Show();
    m_frame->Restore();
    m_frame->Raise();
}

wxMenu* CMgramTaskBarIcon::CreatePopupMenu() {
    wxMenu* menu = new wxMenu;
    menu->Append(wxID_ANY, "Open");
    menu->AppendSeparator();
    menu->Append(wxID_EXIT, "Exit");
    return menu;
}

void CMgramTaskBarIcon::OnMenuRestore(wxCommandEvent& event) {
    m_frame->Show();
    m_frame->Restore();
    m_frame->Raise();
}

void CMgramTaskBarIcon::OnMenuExit(wxCommandEvent& event) {
    m_frame->Destroy();
}
