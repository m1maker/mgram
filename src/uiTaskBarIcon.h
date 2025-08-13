#ifndef UI_TASK_BAR_ICON_H
#define UI_TASK_BAR_ICON_H

#include <wx/menu.h>
#include <wx/taskbar.h>

class CMainFrame;

class CMgramTaskBarIcon final : public wxTaskBarIcon {
  public:
    CMgramTaskBarIcon(CMainFrame* frame);
    ~CMgramTaskBarIcon() = default;

    virtual wxMenu* CreatePopupMenu() override;

  private:
    void OnLeftDoubleClick(wxTaskBarIconEvent& event);
    void OnMenuRestore(wxCommandEvent& event);
    void OnMenuExit(wxCommandEvent& event);

    CMainFrame* m_frame;

    DECLARE_EVENT_TABLE()
};

#endif
