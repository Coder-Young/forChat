#include "StatusZmqClient.h"
#include "ZmqPoolManager.h"

StatusZmqClient::StatusZmqClient()
{
}

StatusZmqClient::~StatusZmqClient()
{
}

bool StatusZmqClient::getChatServer(const int& uid, Json::Value &response)
{
    Json::Value request;
    request["ID"] = "GetChatServer";
    request["uid"] = uid;
    std::string jsonstr = request.toStyledString();

    zmq::message_t request_msg(jsonstr.size());
    memcpy(request_msg.data(), jsonstr.c_str(), jsonstr.size());

    auto pool = ZmqPoolManager::GetInstance()->getPool("StatusServer");
    auto dealer_ = pool->getConnection();
    if (dealer_ == nullptr){
        std::cerr << "[VerifyZmqClient]Failed to get Zmq connection" << std::endl;
        pool->returnConnection(dealer_);
        return false;
    }

    try{
        std::cout << "[getChatServer]ready to send" << std::endl;
        auto send_res = dealer_->send(request_msg, zmq::send_flags::none);
        if (!send_res.has_value()) {
            std::cerr << "[StatusZmqClient] Failed to send request." << std::endl;
            pool->returnConnection(dealer_);
            return false;
        }

        // std::cout << "[getChatServer]ready to read" << std::endl;
        // std::vector<zmq::message_t> parts;
        // while (true){
        //     zmq::message_t part;
        //     if (!dealer_->recv(part)){
        //         std::cerr << "[StatusZmqClient]Failed to receive message part1" << std::endl;
        //         return false;
        //     }
        //     std::cout << "recv " << part.to_string() << std::endl;
        //     parts.push_back(std::move(part));
        //     if (!dealer_->get(zmq::sockopt::rcvmore))
        //         break;
        // }
        // std::cout << "[getChatServer]read success" << std::endl;

        std::vector<std::string> parts;
        while (true)
        {
            zmq::message_t part;
            if (!dealer_->recv(part))
            {
                std::cerr << "[StatusZmqClient] Failed to receive response." << std::endl;
                pool->returnConnection(dealer_);
                return false;
            }

            parts.emplace_back(static_cast<char*>(part.data()), part.size());
            if (!dealer_->get(zmq::sockopt::rcvmore))
                break;
        }

        if (parts.size() < 2 || parts.at(1).empty())
        {
            std::cerr << "[StatusZmqClient] Failed to receive response." << std::endl;
            pool->returnConnection(dealer_);
            return false;
        }

        std::string res_str = parts.back();
        Json::Reader reader;
        bool parse_success = reader.parse(res_str, response);
        if (!parse_success)
        {
            std::cerr << "[StatusZmqClient] Failed to parse response." << std::endl;
            pool->returnConnection(dealer_);
            return false;
        }

        pool->returnConnection(dealer_);
        return true;
    }
    catch(const std::exception& e){
        std::cerr << "[StatusZmqClient]ZMQ Exception: " << e.what() << std::endl;
        pool->returnConnection(dealer_);
        pool->Shutdown();
        return false;
    }
}