#ifndef NOTIFICATION_SENDER_H
#define NOTIFICATION_SENDER_H
#include "audio.h"
#include "singleton.h"

#include <memory>
#include <string>
#include <wx/notifmsg.h>
#include <wx/wx.h>

class CNotificationSender {
    std::unique_ptr<wxNotificationMessage> m_appNotification;
    std::unique_ptr<Ma::Sound> m_appNotificationSound;

  public:
    CNotificationSender() = default;
    ~CNotificationSender() = default;
    void Send(const wxString& title, const wxString& text);
};

#define g_notificationSender CSingleton<CNotificationSender>::GetInstance()

#endif
