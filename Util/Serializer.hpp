#pragma once

#include <zmqpp/zmqpp.hpp>

zmqpp::message& operator << (zmqpp::message &msg, const std::vector<uint8_t> &buffer) {
  msg.add_raw(reinterpret_cast<const void*>(buffer.data()), buffer.size());
  return msg;
}

zmqpp::message& operator >> (zmqpp::message &msg, std::vector<uint8_t> &buffer) {
  size_t part = msg.read_cursor();
  const uint8_t* data = static_cast<const uint8_t*>(msg.raw_data(part));
  size_t len = msg.size(part);
  msg.next();
  buffer.assign(data, data + len);
  return msg;
}
