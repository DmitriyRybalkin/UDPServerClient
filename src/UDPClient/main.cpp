/*
* Filename: main.cpp
* Author: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
* Created: Tue Dec 21 17:41:26 2015 +0600
* Version: v1.0.0
* Last-Updated: Tue Dec 21 17:41:26 2015 +0600
* By: Dmitriy Rybalkin <dmitriy.rybalkin@gmail.com>
*
* Target platform for cross-compilation: ARCH=arm && proc=imx6
*/
#include "client.h"

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 4)
    {
      std::cerr << "Usage: client <host_ip> <client_id> <socket_port>" << std::endl;
      return 1;
    }
    
    boost::asio::io_service io_service;
    boost::thread send_msg_service_thread, get_msg_service_thread;
    
    udp_client client(io_service, argv[1], "daytime", argv[2], std::atoi(argv[3]));
    send_msg_service_thread = boost::thread(boost::bind(&udp_client::run_send_msg_service, &client));
    get_msg_service_thread = boost::thread(boost::bind(&udp_client::run_get_msg_service, &client));
    
    send_msg_service_thread.join();
    get_msg_service_thread.join();
    
    io_service.run();
  }
  catch (std::exception &e)
  {
    std::cerr <<"udp_client::main:error = "<< e.what() << std::endl;
  }

  return 0;
}