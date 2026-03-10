#pragma once
#include "const.h"
#include "Singleton.h"

class EmailService : public Singleton<EmailService>
{
	friend class Singleton<EmailService>;

public:
    EmailService(const EmailService&) = delete;
    EmailService& operator=(const EmailService&) = delete;

    EmailService() = default;
    ~EmailService();

    void init(const std::string& smtpServer, int port,
        const std::string& username, const std::string& authCode,
        const std::string& fromName);

    EmailResult SendEmail(const std::string& to, const std::string& subject, const std::string& body);

private:
    void Shutdown();

    static size_t PayloadCallback(void* ptr, size_t size, size_t nmemb, void* userp);

private:
    bool m_initialized = false;
    bool m_curlGlobalInitDone = false;
    std::string m_smtpServer;
    int m_smtpPort;
    std::string m_username;
    std::string m_authCode;
    std::string m_fromName;
    std::string m_fromAddress;

    std::mutex m_initMutex;
};

