/*
* Filename: client.cpp
* Author: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
* Created: Tue Dec 21 17:41:26 2015 +0600
* Version: v1.0.0
* Last-Updated: Tue Dec 21 17:41:26 2015 +0600
* By: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
*
* Target platform for cross-compilation: ARCH=arm && proc=imx6
*/
#include "client.h"

/* define/undef to turn on/off std output */
#define DEBUG

bool udp_client::has_messages() {
  return !_incoming_messages.empty();
}

std::string udp_client::pop_message() {
  std::cout << "udp_client::pop_message" << std::endl;
  if(_incoming_messages.empty())
    throw std::logic_error("No messages to pop");
  else
    return _incoming_messages.pop();
}

/* sending messages to server */
void udp_client::run_send_msg_service() {
  std::cout << "udp_client::run_send_msg_service" << std::endl;
  
    for(;;) {
      boost::system::error_code errcode;
      _socket.send_to(boost::asio::buffer(*_message), _server_endpoint, 0, errcode);
      
      /* check error code returned by packet sending function */
      switch(errcode.value()) {
	/* if success */
	case boost::system::errc::success:
	  #if defined DEBUG
	  std::cout << "udp_client::run_send_msg_service:status = Success" << std::endl;
	  #endif
	  break;
	/* if network is unreachable */
	case boost::system::errc::network_unreachable: 
	  std::cout << "udp_client::run_send_msg_service:error = Network is unreachable" << std::endl;
	  
	  /* wait 1 sec. and try again */
	  boost::this_thread::sleep(boost::posix_time::seconds(1));
	
	  _socket.send_to(boost::asio::buffer(*_message), _server_endpoint);
	  break;
	/* if network is down */
	case boost::system::errc::network_down:
	  std::cout << "udp_client::run_send_msg_service:error = Network is down" << std::endl;
	  
	  /* wait 10 sec. and try again */
	  boost::this_thread::sleep(boost::posix_time::seconds(10));
	  
	  _socket.send_to(boost::asio::buffer(*_message), _server_endpoint);
	  break;
	/* if network is in reset state */
	case boost::system::errc::network_reset:
	  std::cout << "udp_client::run_send_msg_service:error = Network is reset" << std::endl;
	  
	  /* wait 5 sec. and try again */
	  boost::this_thread::sleep(boost::posix_time::seconds(5));
	  
	  _socket.send_to(boost::asio::buffer(*_message), _server_endpoint);
	  break;
	/* if connection is in progress */
	case boost::system::errc::connection_already_in_progress:
	  std::cout << "udp_client::run_send_msg_service:error = Connection is in progress" << std::endl;
	  
	  /* wait 5 sec. and try again */
	  boost::this_thread::sleep(boost::posix_time::seconds(5));
	  
	  _socket.send_to(boost::asio::buffer(*_message), _server_endpoint);
	  break;
	/* error code is not essential or unknown */
	default:
	  std::cout << "udp_client::run_send_msg_service:error = Unknown error" << std::endl;
	  break;
      }
      
      boost::this_thread::sleep(boost::posix_time::seconds(3));
    }
}

/* service to handle incoming server messages */
void udp_client::run_get_msg_service() {
  #if defined DEBUG
  std::cout << "udp_client::run_get_msg_service" << std::endl;
  #endif
  
  for(;;) {
    size_t len = _socket.receive_from(boost::asio::buffer(_recv_buf), _remote_endpoint);
    
    std::string message_rec(_recv_buf.data(), len);
    _incoming_messages.push(message_rec);
    if(this->has_messages()) {
      std::string server_msg = this->pop_message();
      #if defined DEBUG
      std::cout << "UDPClient last recieved message: " << server_msg << std::endl;
      #endif
      /* if server tells to start video/audio streaming */
      if((this->parse_string(server_msg, change_ip_reg) == "1") && (this->parse_string(server_msg, current_stream_reg) == "1"))
	this->cmd_do_server_set_ip();
      else
	this->cmd_do_server_set_default_ip();
    }
    
    boost::this_thread::sleep(boost::posix_time::seconds(1));
  }
}

/* commands to be executed by udp client */
void udp_client::cmd_do_server_set_default_ip()
{
  try {
    if(!is_default_ip) {
      system(DEFAULT_STREAM_SCRIPT);
      
      is_changed_ip = false;
      is_default_ip = true;
    }
  } catch(std::exception ex) {
   std::cout << "udp_server::cmd_do_server_set_default_ip:exception = " << ex.what() << std::endl; 
  }
}

void udp_client::cmd_do_server_set_ip()
{
  try {
    if(!is_changed_ip) {
      system(CHANGE_STREAM_SCRIPT);
      
      is_changed_ip = true;
      is_default_ip = false;
    }
  } catch(std::exception ex) {
   std::cout << "udp_server::cmd_do_server_set_ip:exception = " << ex.what() << std::endl; 
  }
}

std::string udp_client::parse_string(const std::string& input_str, boost::regex& regex_str)
{
    boost::smatch str_m;
    
    if(boost::regex_match(input_str, str_m, regex_str))
      return str_m[1].str();
    else
      return "no_match";
}