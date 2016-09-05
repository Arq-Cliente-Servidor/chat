#pragma once

#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <zmqpp/zmqpp.hpp>

#include "Serializer.hpp"
#include "User.hpp"

using namespace std;
using namespace zmqpp;

class ServerState {
private:
  // conenected users
  socket &sckt;
  unordered_map<string, User> users;
  unordered_map<string, list<string>> groups;

public:
  ServerState(socket &s) : sckt(s) {}

  void send(message &msg) { sckt.send(msg); }

  string getId(const string &name) {
    if (isUser(name))
      return users[name].getId();
    else return "";
  }

  string getUserName(const string &id) {
    for (const auto &user : users) {
      if (user.second.getId() == id)
        return user.first;
    }
    return "";
  }

  bool isFriend(const string &userName, const string &friendName) {
    if (isUser(friendName)) {
      return users[userName].isFriend(friendName);
    }
    return false;
  }

  bool removeFriend(const string &senderName, const string &friendName) {
    if (isUser(friendName) and isFriend(senderName, friendName)) {
      return users[senderName].removeContact(friendName);
    }
    return false;
  }

  void disconnect(const string &id, const string &name) {
    users[name].disconnect(id);
  }

  bool isUser(const string &userName) {
    return (users.count(userName) > 0);
  }

  bool isGroup(const string &groupName) {
    return (groups.count(groupName) > 0);
  }

  bool isConnected(const string &name) { users[name].isConnected(); }

  void newUser(const string &name, const string &pwd, const string &id) {
    if (isUser(name)) {
      message m;
      m << id << "warning" << "this user already exists, please choose another.." << false;
      send(m);
    } else {
      message m;
      users[name] = User(name, pwd, id);
      cout << "User " << name << " has successfully registered" << endl;
      m << id << "warning" << "You have successfully registered!." << false;
      send(m);
    }
  }

  bool login(const string &name, const string &pwd, const string &id) {
    if (isUser(name)) {
      // User is registered
      bool ok = users[name].isPassword(pwd);
      if (ok) users[name].connect(id);
      return ok;
    }
    return false;
  }

  void addFriend(const string &from, const string &to) {
    if (from == to) {
      message m;
      string id = getId(from);
      m << id << "warning" << "You can not be your own friend!" << true;
      send(m);
    } else if (isUser(to)) {
      if (users[from].addContact(to)) {
        users[to].addContact(from);

        // Messages for me
        string fromId = getId(from);
        string txt1 = to + " is already your friend.";
        message m1;
        m1 << fromId << "warning" << txt1 << true;
        send(m1);

        // Messages for my friend
        string toId = getId(to);
        string txt2 = from + " has added you as your friend.";
        message m2;
        m2 << toId << "warning" << txt2 << true;
        send(m2);
      } else {
        string id = getId(from);
        string txt = "The user " + to + " was already your friend.";
        message m;
        m << id << "warning" << txt << true;
        send(m);
      }
    } else {
      message m;
      m << getId(from) << "warning" << "The user " + to + " is not found" << true;
      send(m);
    }
  }

  string chatTo(const string &senderName, const string &friendName) {
    if (isUser(friendName) and isConnected(friendName) and isFriend(senderName, friendName)) {
      return getId(friendName);
    } else return "";
  }

  void createGroup(const string &groupName, const string &name) {
    string id = getId(name);
    if (isGroup(groupName)) {
      message m;
      m << id << "warning" << "The group " + groupName + " was already created." << true;
      send(m);
    } else {
      message m;
      string txt = "The group " + groupName + " has been created.";
      groups[groupName].push_back(name);
      cout << txt << endl;
      m << id << "warning" << txt << true;
      send(m);
    }
  }

  bool belongsGroup(const string &groupName, const string &name) {
    if (!isGroup(groupName)) return false;
    for (const auto &it : groups[groupName]) if (it == name) {
      return true;
    }
    return false;
  }

  bool addGroup(const string &groupName, const string &senderId,
                const string &friendName, const string &senderName) {
    if (!isGroup(groupName)) {
      message m;
      m << senderId << "warning" << "The group " + groupName + " does not exist." << true;
      send(m);
      return false;
    } else if (belongsGroup(groupName, friendName)) {
      message m;
      string name = (friendName == senderName) ? "You" : friendName;
      m << senderId << "warning" << name + " was already in the group " + groupName << true;
      send(m);
      return false;
    } else if (isUser(friendName) and isFriend(senderName, friendName) and
              isConnected(friendName) and belongsGroup(groupName, senderName)) {

      groups[groupName].push_back(friendName);
      for (const auto &user : groups[groupName]) {
        string id = getId(user);
        if (id.size()) {
          message m;
          m << id << "warning" << friendName + " has been added to the group." << true;
          send(m);
        }
      }
      return true;
    } else {
      message m;
      m << senderId << "warning" << "User " + friendName + " not found/offline/not is your friend" << true;
      send(m);
      return false;
    }
  }

  bool exit(const string &groupName, const string &name) {
    if (isGroup(groupName) and belongsGroup(groupName, name)) {
      list<string>::iterator user;
      for (user = groups[groupName].begin(); user != groups[groupName].end();
           ++user) {
        if (*user == name) {
          groups[groupName].erase(user);
          return true;
        }
      }
      return false;
    }
    return false;
  }

  void leaveGroup(const string &groupName, const string &sender, const string &senderName) {
    if (exit(groupName, senderName)) {
      for (const auto &user : groups[groupName]) {
        string id = getId(user);
        if (id.size() and id != sender) {
          message m;
          m << id << "warning" << "The user " + senderName + " has left the group " + groupName << true;
          send(m);
        }
      }
      message m;
      m << sender << "warning" << "You have successfully left the group " + groupName << true;
      send(m);
      if (groups[groupName].size() == 0) {
        groups.erase(groupName);
        cout << "The group " << groupName << " has been removed" << endl;
      }
    } else {
      message m;
      m << sender << "warning" << "The group " + groupName + "do not exist/not found/not belong" << true;
      send(m);
    }
  }

  void groupChat(const string &groupName, const string &sender, const string &senderName, const string &text) {
    if (isGroup(groupName)) {
      for (const auto &user : groups[groupName]) {
        string id = getId(user);
        if (id.size() and id != sender) {
          message m;
          m << id << "groupReceive" << groupName << senderName << text;
          send(m);
        }
      }
      if (groups[groupName].size() == 1) {
        message m;
        m << sender << "warning" << "There are not more users in the group " + groupName << true;
        send(m);
      }
    } else {
      message m;
      m << sender << "warning" << "The group " + groupName + " does not exist/not found." << true;
      send(m);
    }
  }

  void recordTo(const string &sender, const string &senderName, const string &friendName, vector<int16_t> &samples,
                const int sampleCount, const int channelCount, const int sampleRate, bool isCall = false) {

    // Extract the id of the user
    string id = chatTo(senderName, friendName);

    if (id.size()) {
      message msg;
      string friendId = getId(friendName);
      string act = (isCall) ? "callReceive" : "recordReceive";
      msg << friendId << act << senderName << samples << sampleCount << channelCount << sampleRate;
      send(msg);
    } else {
      if (senderName == friendName) {
        message m;
        m << sender << "warning" << "You can not send voice messages to yourself!" << true;
        send(m);
      } else {
        message m;
        m << sender << "warning" << "The user " + friendName + " is offline/not exist/not your friend." << true;
        send(m);
      }
    }
  }

  void recordGroup(const string &sender, const string &senderName, const string &groupName, vector<int16_t> &samples,
                   const int sampleCount, const int channelCount, const int sampleRate, bool isCall = false) {

    if (isGroup(groupName)) {
      for (const auto &user : groups[groupName]) {
        string id = getId(user);
        if (id.size() and id != sender) {
          message m;
          string act = (isCall) ? "callGroupReceive" : "recordReceiveGroup";
          m << id << act << groupName << senderName << samples << sampleCount << channelCount << sampleRate;
          send(m);
        }
      }
    } else {
      message m;
      m << sender << "warning" << "The group " + groupName + " does not exist/not found." << true;
      send(m);
    }
  }

  void acceptCallGroup(const string &sender, const string &senderName, const string &groupName) {

    if (isGroup(groupName)) {
      for (const auto &user : groups[groupName]) {
        string id = getId(user);
        if (id.size()) {
          message rep;
          rep << id << "callResponseGroup" << groupName;
          send(rep);
        }
      }
    } else {
      message m;
      m << sender << "warning" << "The group " + groupName + " does not exist/not found." << true;
      send(m);
    }
  }

  void stopCallGroup(const string &sender, const string &senderName, const string &groupName) {
    for (const auto &user : groups[groupName]) {
      string id = getId(user);
      if (id.size() and id != sender) {
        message m;
        m << id << "stopGroup" << groupName;
        send(m);
      }
    }
  }

  void listGroup(const string &name) {
    message m;
    string id = getId(name);
    string listGroups = "\nList Group:\n";
    string gr;
    for (const auto &group : groups) {
      for (const auto &user : group.second) {
        if (user == name) {
          gr += "* " + group.first + "\n";
          break;
        }
      }
    }
    if (gr.size()) {
      listGroups += gr;
    } else {
      listGroups + "None";
    }
    m << id << "warning" << listGroups << true;
    send(m);
  }
};
