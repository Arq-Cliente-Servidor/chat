#pragma once

#include <list>
#include <string>

class User {
private:
  std::string name;
  std::string password;
  std::string netId;
  bool connected;
  std::list<std::string> contacts;

public:
  User() {}
  // Attribute initialization
  User(const std::string &name, const std::string &pwd, const std::string &id)
      : name(name), password(pwd), netId(id), connected(false) {}

  bool isPassword(const std::string &pwd) const { return password == pwd; }

  const std::string &getId() const { return netId; }

  const std::string &getName() const { return name; }

  void connect(const std::string &id) {
    connected = true;
    netId = id;
  }

  void disconnect(const std::string &id) {
    connected = false;
    netId = "";
  }

  bool isConnected() const { return connected; }

  bool isFriend(const std::string &user) {
    for (const auto &contact : contacts) {
      if (contact == user)
        return true;
    }
    return false;
  }

  bool addContact(const std::string &name) {
    if (!isFriend(name)) {
      contacts.push_back(name);
      return true;
    } else {
      return false;
    }
  }

  bool removeContact(const std::string &name) {
    std::list<std::string>::iterator contact;
    for (contact = contacts.begin(); contact != contacts.end(); ++contact) {
      if (*contact == name) {
        contacts.erase(contact);
        return true;
      }
    }
    return false;
  }
};
