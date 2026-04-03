#include "StatusZmqClient.h"
#include "ZmqPoolManager.h"
#include "ZmqConnectionGuard.h"

StatusZmqClient::StatusZmqClient()
{
}

StatusZmqClient::~StatusZmqClient()
{
}

bool StatusZmqClient::VerifyToken(int uid, std::string token)
{
    Json::Value request;
    request["ID"] = "VerifyToken";
    request["uid"] = uid;
    request["token"] = token;
    std::string jsonstr = request.toStyledString();
    zmq::message_t request_msg(jsonstr.size());
    memcpy(request_msg.data(), jsonstr.c_str(), jsonstr.size());

    auto pool = ZmqPoolManager::GetInstance()->getPool("StatusServer");
    ZmqConnectionGuard guard(*pool);
    auto dealer = guard.get();
    try{
        auto send_res = dealer->send(request_msg, zmq::send_flags::none);
        if (!send_res.has_value()){
            std::cerr << "[StatusZmqClient] Failed to send request." << std::endl;
            return false;
        }

        std::vector<std::string> parts;
        while (true)
        {
            zmq::message_t part;
            if (!dealer->recv(part))
            {
                std::cerr << "[StatusZmqClient] Failed to receive response." << std::endl;
                return false;
            }

            parts.emplace_back(static_cast<char*>(part.data()), part.size());
            if (!dealer->get(zmq::sockopt::rcvmore))
                break;
        }

        // if (parts.size() < 3 || parts.at(1).empty())
        // {
        //     std::cerr << "[StatusZmqClient] Failed to receive response." << std::endl;
        //     return false;
        // }

        Json::Value response;
        std::string res_str = parts.back();
        Json::Reader reader;
        bool parse_success = reader.parse(res_str, response);
        if (!parse_success)
        {
            std::cerr << "[StatusZmqClient] Failed to parse response." << std::endl;
            return false;
        }
        return true;
    }
    catch(const std::exception& e)
    {
        std::cout << "[VerifyToken]" << e.what() << std::endl;
        return false;
    }
    return false;
}