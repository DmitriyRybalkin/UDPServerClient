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
#include "server.h"

int main(int argc, char* argv[])
{
  try {
    boost::asio::io_service io_service;
    
    udp_server server(io_service, 13);
    io_service.run();
  } catch(std::exception &e) {
    std::cerr <<"udp_server::main:error = "<< e.what() << std::endl;
  }
    
  return 0;
}