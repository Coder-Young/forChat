#include "VerifyZmqClient.h"
#include "CfgManager.h"
#include <vector>

VerifyZmqClient::VerifyZmqClient()
{
    zmqConnPool = std::make_unique<ZmqConnectPool>(3);
}

VerifyZmqClient::~VerifyZmqClient()
{
    if (zmqConnPool)
        zmqConnPool->Close();
}

bool VerifyZmqClient::SendAndRecvMessage(const Json::Value& request, Json::Value& response)
{
    std::string jsonstr = request.toStyledString();
    zmq::message_t request_msg(jsonstr.size());
    memcpy(request_msg.data(), jsonstr.c_str(), jsonstr.size());

    try {
        auto dealer_ = zmqConnPool->getConnection();
        auto send_res = dealer_->send(request_msg, zmq::send_flags::none);
        if (!send_res.has_value()) {
            std::cerr << "[VerifyZmqClient] Failed to send request." << std::endl;
            return false;
        }

        std::vector<std::string> parts;
        while (true){
            zmq::message_t part;
            auto recv_res = dealer_->recv(part);
            parts.emplace_back(static_cast<char*>(part.data()), part.size());
        }

        if (parts.at(1).empty())
        {
            std::cerr << "[VerifyZmqClient] Failed to receive response." << std::endl;
            return false;
        }

        std::string reply_str = parts.at(2);
        Json::Reader reader;
        bool parse_success = reader.parse(reply_str, response);
        if (!parse_success) {
            std::cerr << "[VerifyZmqClient] Failed to parse response JSON." << std::endl;
            return false;
        }

        zmqConnPool->returnConnection(dealer_);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[VerifyZmqClient] ZMQ Exception: " << e.what() << std::endl;
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