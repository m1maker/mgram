#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <wx/simplebook.h>
#include <wx/wx.h>

class CMainWindow : public wxPanel {
  public:
    CMainWindow(wxSimplebook* book);

  private:
    wxSimplebook* m_book;
};

#endif
