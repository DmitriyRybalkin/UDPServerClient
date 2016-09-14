
/*
* Filename: server.h
* Author: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
* Created: Tue Dec 21 17:41:26 2015 +0600
* Version: v1.0.0
* Last-Updated: Tue Dec 21 17:41:26 2015 +0600
* By: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
*
* Target platform for cross-compilation: ARCH=arm && proc=imx6
*/
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/regex.hpp>
#include <boost/function.hpp>
#include <Qt/QtDBus>
#include <QtCore/QString>

#include "locked_queue.h"

#define MAX_CLIENT_LIST_SIZE 12
#define BUFFER_SIZE 12

#define DEFAULT_STREAM_SCRIPT "/usr/local/UDPServerClient/default_stream_main.sh"
#define CHANGE_STREAM_SCRIPT "/usr/local/UDPServerClient/change_stream_main.sh"
#define CURRENT_SSID_PATH "/usr/local/UDPServerClient/current_ssid"
#define CLIENT_SSID_LIST_PATH "/usr/local/UDPServerClient/client_ssid_list"
#define CURRENT_CLIENT_IP "/usr/local/UDPServerClient/current_client_ip"

using boost::asio::ip::udp;

typedef std::pair<std::string, int> client_message;
typedef std::map<int, udp::endpoint> client_list;
typedef std::map<std::string, int> client_ssid_list;
typedef client_list::value_type client;
typedef client_ssid_list::value_type ssid;

class udp_server {

public:
   udp_server(boost::asio::io_service& io_service, unsigned short local_port):
   bus(QDBusConnection::sessionBus()),
   _socket(io_service, udp::endpoint(udp::v4(), local_port)),
   next_client_id(0), client_reg("^client_id=(\\d{8,8});.+"), change_ip_reg(".+change_ip=(\\d{1,1});$"),
   current_stream_reg(".+current_stream=(\\d{1,1});.+"),
   dbus_iface("org.Custom.QRadioWidget", "/drad", "org.Custom.QRadioWidget", bus),
   is_default_ssid(false), is_changed_ssid(false)
   {
     _socket.set_option(boost::asio::socket_base::receive_buffer_size(8192));
     _socket.set_option(udp::socket::reuse_address(true));
     
     strcpy(_prev_ssid, "20000101");
     strcpy(_current_ssid, "20000101");
     
     #if defined DEBUG
     std::cout << "udp_server:_prev_ssid = " << _prev_ssid << std::endl;
     std::cout << "udp_server:_current_ssid = " << _current_ssid << std::endl;
     #endif
     
     dbus_iface.call("clearClientList");
     /* start default audio/video stream configuration */
     this->cmd_do_server_set_default_ssid();
     
     this->start_receive();
   }
   
   ~udp_server() {

   }
   
   void send_to_client(const std::string& message, int client_id);
   void send_to_all(const std::string& message);
   void send_disconnect_to_client();
   
   int get_client_by_index(size_t index);
   
   std::vector<boost::function<void(int)> > client_disconnected_handlers;
   void read_params();
   
private:
   bool is_default_ssid, is_changed_ssid;
   char _current_ssid[12];
   char _prev_ssid[12];
   
   QDBusConnection bus;
   QDBusInterface dbus_iface;
   
   boost::regex client_reg, change_ip_reg, current_stream_reg;
   udp::socket _socket;
   udp::endpoint _server_endpoint;
   udp::endpoint _remote_endpoint;
   boost::array<char, 49> _recv_buffer;
   boost::mutex _mutex;
   
   void start_receive();
   void handle_remote_error(const boost::system::error_code error_code, const udp::endpoint remote_endpoint);
   void handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred);
   void handle_send(std::string /*message*/, const boost::system::error_code& /*error*/, std::size_t /*bytes_transferred*/) {};
    
   int get_or_create_client_id(udp::endpoint endpoint);
   void on_client_disconnected(int id);
   void send(const std::string& message, udp::endpoint target);
   std::string parse_string(const std::string& input_str, boost::regex& regex_str);
   void add_client_ssid_to_list(const std::string& ssid_, int client_id);
   void write_to_file(const char * file_name, const std::string& value);
   void append_string_to_file(const char * file_name, const std::string& value);
   
   /* client commands */
   std::string cmd_do_client_change_ip(const std::string& input_msg);
   std::string cmd_do_make_client_current_stream(const std::string& input_msg);
   void cmd_do_server_set_default_ssid();
   void cmd_do_server_set_ssid();
   
   locked_queue<client_message> _incoming_messages;
   
   client_list clients;
   client_ssid_list ssids;
   
   int next_client_id;
};