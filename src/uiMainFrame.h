#ifndef UI_MAIN_FRAME_H
#define UI_MAIN_FRAME_H
#include "uiLogin.h"
#include "uiLoginPhone.h"

#include <wx/wx.h>

class CMainFrame : public wxFrame {
  public:
    CMainFrame(const wxString& title);
};

extern CMainFrame* g_mainFrame;

class CMgramEntry : public wxApp {
  public:
    virtual bool OnInit() override;
};
#endif
