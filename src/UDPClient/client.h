/*
* Filename: client.h
* Author: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
* Created: Tue Dec 21 17:41:26 2015 +0600
* Version: v1.0.0
* Last-Updated: Tue Dec 21 17:41:26 2015 +0600
* By: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
*
* Target platform for cross-compilation: ARCH=arm && proc=imx6
*/
#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include "locked_queue.h"

#define DEFAULT_STREAM_SCRIPT "/usr/local/UDPServerClient/default_stream_operator.sh"
#define CHANGE_STREAM_SCRIPT "/usr/local/UDPServerClient/change_stream_operator.sh"

using boost::asio::ip::udp;

class udp_client {
  
  public:
    udp_client(boost::asio::io_service& io_service, std::string host, std::string server_port, std::string client_id, unsigned short local_port = 8888):
      _socket(io_service, udp::endpoint(udp::v4(), local_port)), _message(new std::string("client_id="+client_id+";current_stream=0;change_ip=0;")),
      client_reg("^client_id=(\\d{8,8});.+"), change_ip_reg(".+change_ip=(\\d{1,1});$"),
      current_stream_reg(".+current_stream=(\\d{1,1});.+"), is_default_ip(false), is_changed_ip(false)
    {
      /* turn off audio streams outgoing to operator unit */
      this->cmd_do_server_set_default_ip();
      
      udp::resolver resolver(io_service);
      udp::resolver::query query(udp::v4(), host, server_port);
      _server_endpoint = *resolver.resolve(query);
      
      _socket.set_option(udp::socket::reuse_address(true));
      
    }
    
    ~udp_client()
    {

    }
    
    void run_send_msg_service();
    void run_get_msg_service();
    std::string pop_message();
    bool has_messages();
    
  private:
    bool is_default_ip, is_changed_ip;
    boost::regex client_reg, change_ip_reg, current_stream_reg;
    udp::endpoint _server_endpoint;
    udp::endpoint _remote_endpoint;
    udp::socket _socket;
    
    boost::array<char, 49> _send_buf;
    boost::array<char, 49> _recv_buf;
    
    boost::shared_ptr<std::string> _message;
    locked_queue<std::string> _incoming_messages;
    
    std::string parse_string(const std::string& input_str, boost::regex& regex_str);
    void cmd_do_server_set_default_ip();
    void cmd_do_server_set_ip();
};