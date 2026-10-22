#include "VerifyZmqClient.h"
#include "CfgManager.h"
#include "ZmqPoolManager.h"

VerifyZmqClient::VerifyZmqClient()
{
}

VerifyZmqClient::~VerifyZmqClient()
{
}

bool VerifyZmqClient::SendAndRecvMessage(const Json::Value& request, Json::Value& response)
{
    std::string jsonstr = request.toStyledString();
    zmq::message_t request_msg(jsonstr.size());
    memcpy(request_msg.data(), jsonstr.c_str(), jsonstr.size());

    auto pool = ZmqPoolManager::GetInstance()->getPool("VerifyServer");
    auto dealer_ = pool->getConnection();
    if (dealer_ == nullptr){
        std::cerr << "[VerifyZmqClient]Failed to get Zmq connection" << std::endl;
        return false;
    }

    try {
        auto send_res = dealer_->send(request_msg, zmq::send_flags::none);
        if (!send_res.has_value()) {
            std::cerr << "[VerifyZmqClient] Failed to send request." << std::endl;
            return false;
        }

        std::vector<std::string> parts;
        while (true)
        {
            zmq::message_t part;
            if (!dealer_->recv(part))
            {
                std::cerr << "[VerifyZmqClient] Failed to receive response." << std::endl;
                return false;
            }

            parts.emplace_back(static_cast<char*>(part.data()), part.size());
            if (!dealer_->get(zmq::sockopt::rcvmore))
                break;
        }

        if (parts.size() < 2 || parts.at(1).empty())
        {
            std::cerr << "[VerifyZmqClient] Failed to receive response." << std::endl;
            return false;
        }

        std::string reply_str = parts.back();
        Json::Reader reader;
        bool parse_success = reader.parse(reply_str, response);
        if (!parse_success) {
            std::cerr << "[VerifyZmqClient] Failed to parse response JSON." << std::endl;
            return false;
        }

        pool->returnConnection(dealer_);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[VerifyZmqClient] ZMQ Exception: " << e.what() << std::endl;
        pool->returnConnection(dealer_);
        pool->Shutdown();
        return false;
    }
}

std::string VerifyZmqClient::RequestVerifyCode(const std::string& email)
{
    Json::Value root;
    root["email"] = email;

    Json::Value response;
    if (!SendAndRecvMessage(root, response)) {
        return "";
    }

    std::string res_status = response["status"].asString();
    if (res_status == "error")
    {
        return "";
    }

    return response["msg"].asString();
}