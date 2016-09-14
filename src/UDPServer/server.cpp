/*
* Filename: server.cpp
* Author: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
* Created: Tue Dec 21 17:41:26 2015 +0600
* Version: v1.0.0
* Last-Updated: Tue Dec 21 17:41:26 2015 +0600
* By: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
*
* Target platform for cross-compilation: ARCH=arm && proc=imx6
*/
#include "server.h"

/* define/undef to turn on/off std output */
#define DEBUG

void udp_server::start_receive()
{
    _socket.async_receive_from(boost::asio::buffer(_recv_buffer), _remote_endpoint,
        boost::bind(&udp_server::handle_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}
  
void udp_server::handle_receive(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (!error)
    {
        try {
	    /* read params */
	    this->read_params();
	    #if defined DEBUG
	    std::cout << "handle_receive:current_ssid = " << _current_ssid << std::endl;
	    #endif
	    /* handle incoming client messages */
            auto message = client_message(std::string(_recv_buffer.data(), bytes_transferred), get_or_create_client_id(_remote_endpoint));
	    #if defined DEBUG
	    std::cout << "handle_receive:message = " << std::string(_recv_buffer.data()) << std::endl;
	    #endif
	    
	    if (!message.first.empty())
	      _incoming_messages.push(message);
	    
	    std::cout << "handle_receive:enumerate_clients_messages" << std::endl;
	    while(!_incoming_messages.empty()) {
	      client_message client_msg = _incoming_messages.pop();
	      #if defined DEBUG
	      std::cout << "message: " << client_msg.first << " client_id: " << client_msg.second << std::endl;
	      #endif
	      
	      /* maintain list of client id numbers */
	      this->add_client_ssid_to_list(this->parse_string(client_msg.first, client_reg), client_msg.second);
	      
	      if(std::string(_current_ssid).substr(0, 8) == this->parse_string(client_msg.first, client_reg)) {
		#if defined DEBUG
		std::cout << "handle_receive:send_back_to_client_with_current_ssid" << std::endl;
		#endif
		
		/* send back the message to the client that is selected for audio/video streaming */
		this->send_to_client(cmd_do_client_change_ip(cmd_do_make_client_current_stream(std::string(_recv_buffer.data()))), client_msg.second);
		
		/* start streaming audio to the selected client */
		this->cmd_do_server_set_ssid();
	      }
	    }
	      
	    #if defined DEBUG
	    std::cout << "handle_receive:enumerate_clients; size = " << clients.size() << std::endl;
	    
	    for(std::map<int, udp::endpoint>::iterator it = clients.begin(); it != clients.end(); ++it) {
	      std::cout << "client_id = " << it->first << "; client_endpoint = "<< it->second << "; client_ip = " << it->second.address().to_string() << std::endl;
	    }
	    #endif
	    
	    #if defined DEBUG
	    std::cout << "handle_receive:enumerate_ssids; size = " << ssids.size() << std::endl;
	    for(std::map<std::string, int>::iterator it = ssids.begin(); it != ssids.end(); ++it)
	     std::cout << "ssid = " << it->first << "; client_id = "<< it->second << std::endl;
	    #endif
	    
	    this->send_disconnect_to_client();
        }
        catch (std::exception ex) {
	  this->handle_remote_error(error, _remote_endpoint);
          std::cerr << "udp_server::handle_receive: Error parsing incoming message: " << ex.what() << std::endl;
        }
        catch (...) {
	  this->handle_remote_error(error, _remote_endpoint);
	  std::cerr << "udp_server::handle_receive: Unknown error while parsing incoming message" << std::endl;
        }
    }
    else
    {
      this->handle_remote_error(error, _remote_endpoint);
      std::cerr << "udp_server::handle_receive: error: " << error.message() << std::endl;
    }
    this->start_receive();
}

/* reconnect to new client in case current ssid has been changed */
void udp_server::send_disconnect_to_client()
{
  if(std::string(_prev_ssid).substr(0,8) != std::string(_current_ssid).substr(0,8))
  {
    if(std::string(_prev_ssid).substr(0,8) != "20000101") {
      std::map<std::string, int>::iterator it;
      it = ssids.find(std::string(_prev_ssid).substr(0,8));
      
      if(it != ssids.end()) {
	std::string disconnect_msg("client_id=");
	disconnect_msg.append(std::string(_prev_ssid).substr(0,8));
	disconnect_msg.append(";current_stream=0;change_ip=0;");
	
	#if defined DEBUG
	std::cout << "udp_server::send_disconnect_to_client; disconnect_msg = " << disconnect_msg << std::endl;
	std::cout << "udp_server::send_disconnect_to_client; prev_ssid = " << _prev_ssid << std::endl;
	#endif
	this->send_to_client(disconnect_msg, it->second);
	this->cmd_do_server_set_default_ssid();
      }
    }
  }
  
  strcpy(_prev_ssid, _current_ssid);
}

/* maintain the list of client serial numbers, aka AP SSIDs */
void udp_server::add_client_ssid_to_list(const std::string& ssid_, int client_id)
{
  std::map<std::string, int>::iterator it;
  
  if(!ssids.empty()) {
   it = ssids.find(ssid_);
   
   if( it == ssids.end()) {
    if(ssids.size() < MAX_CLIENT_LIST_SIZE) {
      ssids.insert(ssid(ssid_, client_id));
      #if defined DEBUG
      std::cout << "add_client_ssid_to_list:dbus_iface" << std::endl;
      #endif
      //dbus_iface.call("appendClientToList", QString::fromStdString(ssid_));
      this->append_string_to_file(CLIENT_SSID_LIST_PATH, ssid_);
    }
   }
  } else {
    ssids.insert(ssid(ssid_, client_id));
    #if defined DEBUG
    std::cout << "add_client_ssid_to_list:dbus_iface" << std::endl;
    #endif
    //dbus_iface.call("appendClientToList", QString::fromStdString(ssid_));
    this->append_string_to_file(CLIENT_SSID_LIST_PATH, ssid_);
  }
}

/* commands to be executed by clients */
std::string udp_server::cmd_do_client_change_ip(const std::string& input_msg)
{
  std::string output_msg("");
  
  output_msg.append(input_msg.substr(0, 36));
  output_msg.append("change_ip=");
  output_msg.append("1;");
  
  return output_msg;
}

std::string udp_server::cmd_do_make_client_current_stream(const std::string& input_msg)
{
  std::string output_msg("");
  
  output_msg.append(input_msg.substr(0, 19));
  output_msg.append("current_stream=");
  output_msg.append("1;");
  output_msg.append(input_msg.substr(37, 12));
  
  return output_msg;
}

void udp_server::append_string_to_file(const char * file_name, const std::string& value)
{
  std::ofstream outfile;
  outfile.open(file_name, std::ofstream::out | std::ofstream::app);

  outfile << value << std::endl;
  
  outfile.flush();
  outfile.close();
}

void udp_server::write_to_file(const char * file_name, const std::string& value)
{
  std::ofstream outfile;
  outfile.open(file_name);

  outfile << value << std::endl;
  
  outfile.flush();
  outfile.close();
}

/* commands to be executed by server */
void udp_server::cmd_do_server_set_default_ssid()
{
  try {
    if(!is_default_ssid) {
      #if defined DEBUG
      std::cout << "cmd_do_server_set_default_ssid" << std::endl;
      #endif
      this->write_to_file(CURRENT_CLIENT_IP, "192.168.1.4");
      
      system(DEFAULT_STREAM_SCRIPT);
      
      is_default_ssid = true;
      is_changed_ssid = false;
    }
  } catch(std::exception ex) {
   std::cout << "udp_server::cmd_do_server_set_default_ssid:exception = " << ex.what() << std::endl; 
  }
}

void udp_server::cmd_do_server_set_ssid()
{
  try {
    if(!is_changed_ssid) {
      #if defined DEBUG
      std::cout << "cmd_do_server_set_ssid" << std::endl;
      #endif
      if(!ssids.empty()) {
	std::map<std::string, int>::iterator it;
	it = ssids.find(std::string(_current_ssid).substr(0, 8));
	
	if(it != ssids.end()) {
	  if(!clients.empty()) {
	    std::map<int, udp::endpoint>::iterator client_it;
	    client_it = clients.find(it->second);
	    
	    if(client_it != clients.end()) {
	      /* write current client ip to file */
	      this->write_to_file(CURRENT_CLIENT_IP, client_it->second.address().to_string());
	    }
	  }
	}
      }
	
      system(CHANGE_STREAM_SCRIPT);
    
      is_changed_ssid = true;
      is_default_ssid = false;
    }
  } catch(std::exception ex) {
   std::cout << "udp_server::cmd_do_server_set_ssid:exception = " << ex.what() << std::endl; 
  }
}

std::string udp_server::parse_string(const std::string& input_str, boost::regex& regex_str)
{
    boost::smatch str_m;
    
    if(boost::regex_match(input_str, str_m, regex_str))
      return str_m[1].str();
    else
      return "no_match";
}
  
void udp_server::read_params()
{
    int fd;
    ssize_t read_bytes;
    char buffer[BUFFER_SIZE+1];
    
    /* read ssid value for AP config */
    fd = open (CURRENT_SSID_PATH, O_RDONLY);
      
    if(fd < 0) {
	strcpy(_current_ssid, "0");
    } else {
	while ((read_bytes = read (fd, buffer, BUFFER_SIZE)))
	    buffer[read_bytes] = 0;
	
	strcpy(_current_ssid, buffer);
	
	close (fd);
    }
}

/* send message to client */
void udp_server::send(const std::string& message, udp::endpoint target_endpoint)
{
  _socket.send_to(boost::asio::buffer(message), target_endpoint);
}

/* add new client id or retrieve the existing one */
int udp_server::get_or_create_client_id(udp::endpoint endpoint)
{
  for (const auto& client : clients)
    if (client.second == endpoint)
      return client.first;
    
  if(clients.size() < MAX_CLIENT_LIST_SIZE) {
    auto id = ++next_client_id;
    clients.insert(client(id, endpoint));
    return id;
  } else {
    next_client_id = 0;
    
    return 13;
  }
};

void udp_server::on_client_disconnected(int id)
{
  for (auto& handler : client_disconnected_handlers)
    if (handler)
      handler(id);
}

/* remove client from the list in case of system error */
void udp_server::handle_remote_error(const boost::system::error_code error_code, const udp::endpoint remote_endpoint)
{
  bool found = false;
  int id;
  for (const auto& client : clients)
    if (client.second == remote_endpoint) {
      found = true;
      id = client.first;
      break;
    }
  if (found == false)
    return;
  
  clients.erase(id);
  
  on_client_disconnected(id);
}

/* send message to specific client */
void udp_server::send_to_client(const std::string& message, int client_id)
{
  try {
   this->send(message, clients.at(client_id));
  }
  catch (std::out_of_range) {
    std::cerr << "Unknown client ID " << client_id << std::endl;
  }
}

/* send messages to all clients in the list */
void udp_server::send_to_all(const std::string& message)
{
  for (auto client : clients)
    this->send(message, client.second);
}