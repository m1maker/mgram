#include "notificationSender.h"

void CNotificationSender::Send(const wxString& title, const wxString& content) {
    m_appNotificationSound = g_audioEngine.createSound("snd/NotificationMessageReceived.ogg");
    if (m_appNotificationSound)
        m_appNotificationSound->play();
    m_appNotification = std::make_unique<wxNotificationMessage>(title, content);
    m_appNotification->Show();
}
