#include "EmailService.h"
#include "EmailService.h"
#include <iostream>
#include <sstream>
#include <cstring>

struct MailPayload {
    const char* read_ptr;
    size_t read_size;
};

EmailService::~EmailService() {
    Shutdown();
}

void EmailService::init(const std::string& smtpServer, int port,
    const std::string& username, const std::string& authCode,
    const std::string& fromName) {
    std::lock_guard<std::mutex> lock(m_initMutex);

    if (m_initialized) {
        return;
    }

    if (!m_curlGlobalInitDone) {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
            throw std::runtime_error("Failed to initialize");
        }
        m_curlGlobalInitDone = true;
    }

    m_smtpServer = smtpServer;
    m_smtpPort = port;
    m_username = username;
    m_authCode = authCode;
    m_fromName = fromName;
    m_fromAddress = username;

    m_initialized = true;
    myspdlog::info("[EmailService] Initialized with SMTP: {}:{} authCode: {}", smtpServer, port, authCode.length());
}

void EmailService::Shutdown() {
    std::lock_guard<std::mutex> lock(m_initMutex);
    if (m_curlGlobalInitDone) {
        curl_global_cleanup();
        m_curlGlobalInitDone = false;
        m_initialized = false;
    }
}

EmailResult EmailService::SendEmail(const std::string& to, const std::string& subject, const std::string& body) {
    if (!m_initialized) {
        return { false, "EmailService not initialized" };
    }

    std::stringstream mail_data;
    mail_data << "To: " << to << "\r\n"
        << "From: " << m_fromName << " <" << m_fromAddress << ">\r\n"
        << "Subject: " << subject << "\r\n"
        << "MIME-Version: 1.0\r\n"
        << "Content-Type: text/plain; charset=UTF-8\r\n"
        << "Content-Transfer-Encoding: 8bit\r\n"
        << "\r\n"
        << body;

    std::string mail_str = mail_data.str();
    MailPayload payload = { mail_str.c_str(), mail_str.size() };

    CURL* curl = curl_easy_init();
    if (!curl) {
        return { false, "Failed to init CURL handle" };
    }

    struct curl_slist* recipients = NULL;
    CURLcode res = CURLE_FAILED_INIT;

    std::string url = (m_smtpPort == 465 ? "smtps://" : "smtp://") +
        m_smtpServer + ":" + std::to_string(m_smtpPort);

    if (m_smtpPort == 465) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        // 587: 显式 STARTTLS
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_USERNAME, m_username.c_str());
    curl_easy_setopt(curl, CURLOPT_PASSWORD, m_authCode.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m_fromAddress.c_str());

    //std::string temp = "2803664719@qq.com";
    recipients = curl_slist_append(recipients, to.c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

    curl_easy_setopt(curl, CURLOPT_READFUNCTION, PayloadCallback);
    curl_easy_setopt(curl, CURLOPT_READDATA, &payload);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);

    EmailResult result;
    if (res != CURLE_OK) {
        result.success = false;
        result.message = std::string("SMTP Error: " + std::string(curl_easy_strerror(res)));
        std::cerr << "[EmailService] Send failed to " << to << ": " << result.message << std::endl;
    }
    else {
        result.success = true;
        result.message = "OK";
    }

    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);

    return result;
}

size_t EmailService::PayloadCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    MailPayload* upload_ctx = static_cast<MailPayload*>(userp);
    size_t buffer_size = size * nmemb;

    if (upload_ctx->read_size == 0) return 0;

    size_t copy_size = (upload_ctx->read_size < buffer_size) ? upload_ctx->read_size : buffer_size;
    memcpy(ptr, upload_ctx->read_ptr, copy_size);

    upload_ctx->read_ptr += copy_size;
    upload_ctx->read_size -= copy_size;

    return copy_size;
}