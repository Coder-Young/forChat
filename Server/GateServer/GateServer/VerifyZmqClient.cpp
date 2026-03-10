#include "VerifyZmqClient.h"
#include "CfgManager.h"

VerifyZmqClient::VerifyZmqClient() : 
    context_(1), dealer_(context_, ZMQ_DEALER)
{
	//dealer_.setsockopt(ZMQ_IDENTITY, "LogicSystem", 11);
    zmq_setsockopt(dealer_, ZMQ_IDENTITY, "LogicSystem", 11);

    CfgManager gCfgMgr;
    endpoint_ = gCfgMgr["VerifyServer"]["Address"] + ":" + gCfgMgr["VerifyServer"]["Port"];
	dealer_.connect(endpoint_);
}

bool VerifyZmqClient::SendAndRecvMessage(const Json::Value& request, Json::Value& response)
{
    std::string jsonstr = request.toStyledString();
    zmq::message_t request_msg(jsonstr.size());
    memcpy(request_msg.data(), jsonstr.c_str(), jsonstr.size());

    try {
        auto send_res = dealer_.send(request_msg, zmq::send_flags::none);
        if (!send_res.has_value()) {
            std::cerr << "[VerifyZmqClient] Failed to send request." << std::endl;
            return false;
        }

        zmq::message_t reply_msg;
        auto recv_res = dealer_.recv(reply_msg, zmq::recv_flags::none);
        if (!recv_res.has_value()) {
            std::cerr << "[VerifyZmqClient] Failed to receive response." << std::endl;
            return false;
        }

        std::string reply_str(static_cast<char*>(reply_msg.data()), reply_msg.size());
        Json::Reader reader;
        bool parse_success = reader.parse(reply_str, response);
        if (!parse_success) {
            std::cerr << "[VerifyZmqClient] Failed to parse response JSON." << std::endl;
            return false;
        }

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