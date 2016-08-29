// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <zmqpp/zmqpp.hpp>

#include "Util/Serializer.hpp"

using namespace std;
using namespace zmqpp;

class User {
private:
  string name;
  string password;
  string netId;
  bool connected;
  list<string> contacts;

public:
  User() {}
  // Attribute initialization
  User(const string &name, const string &pwd, const string &id)
      : name(name), password(pwd), netId(id), connected(false) {}

  bool isPassword(const string &pwd) const { return password == pwd; }

  const string &getId() const { return netId; }

  const string &getName() const { return name; }

  void connect(const string &id) {
    connected = true;
    netId = id;
  }

  bool isConnected() const { return connected; }

  bool isFriend(const string &user) {
    for (const auto &contact : contacts) {
      if (contact == user)
      return true;
    }
    return false;
  }

  bool addContact(const string &name) {
    if (!isFriend(name)) {
      contacts.push_back(name);
      return true;
    } else {
      return false;
    }
  }
};

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
    if (users.count(name)) return users[name].getId();
    else return "";
  }

  string getUserName(const string &id) {
    for (const auto &user : users) {
      if (user.second.getId() == id)
      return user.first;
    }
    return "";
  }

  void newUser(const string &name, const string &pwd, const string &id) {
    if (users.count(name)) {
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
    if (users.count(name)) {
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
      string id = users[from].getId();
      m << id << "warning" << "You can not be your own friend!" << true;
      send(m);
    } else if (users.count(to)) {
      if (users[from].addContact(to)) {
        users[to].addContact(from);

        // Messages for me
        string fromId = users[from].getId();
        string txt1 = to + " is already your friend.";
        message m1;
        m1 << fromId << "warning" << txt1 << true;
        send(m1);

        // Messages for my friend
        string toId = users[to].getId();
        string txt2 = from + " has added you as your friend.";
        message m2;
        m2 << toId << "warning" << txt2 << true;
        send(m2);
      } else {
        string id = users[from].getId();
        string txt = "The user " + to + " was already your friend.";
        message m;
        m << id << "warning" << txt << true;
        send(m);
      }
    } else {
      message m;
      m << users[from].getId() << "warning"
        << "The user " + to + " is not found" << true;
      send(m);
    }
  }

  string chatTo(const string &senderName, const string &friendName) {
    if (users.count(friendName) and users[friendName].isConnected() and users[senderName].isFriend(friendName)) {
      return users[friendName].getId();
    } else
    return "";
  }

  void createGroup(const string &groupName, const string &name) {
    string id = users[name].getId();
    if (groups.count(groupName)) {
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
    if (!groups.count(groupName)) return false;
    for (const auto &it : groups[groupName]) if (it == name) {
      return true;
    }
    return false;
  }

  bool addGroup(const string &groupName, const string &senderId, const string &friendName, const string &senderName) {
    if (!groups.count(groupName)) {
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
    } else if (users.count(friendName) and users[senderName].isFriend(friendName) and users[friendName].isConnected()) {
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

  void groupChat(const string &groupName, const string &sender, const string &senderName, const string &text) {
    if (groups.count(groupName)) {
      for (const auto &user : groups[groupName]) {
        string id = getId(user);
        if (id.size() and id != sender) {
          message m;
          m << id << "groupReceive" << groupName << senderName << text;
          send(m);
        }
      }
    } else {
      message m;
      m << sender << "warning" << "The group " + groupName + " does not exist/not found." << true;
      send(m);
    }
  }

  void recordTo(const string &sender, const string &senderName, const string &friendName, vector<uint8_t> &samples,
                const int sampleCount, const int channelCount, const int sampleRate) {

    // Extract the id of the user
    string id = chatTo(senderName, friendName);

    if (id.size()) {
      message msg;
      string friendId = users[friendName].getId();
      msg << friendId << "recordReceive" << senderName << samples << sampleCount << channelCount << sampleRate;
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

  void recordGroup(const string &sender, const string &senderName, const string &groupName, vector<uint8_t> &samples,
                   const int sampleCount, const int channelCount, const int sampleRate) {

    if (groups.count(groupName)) {
      for (const auto &user : groups[groupName]) {
        string id = getId(user);
        if (id.size() and id != sender) {
          message m;
          m << id << "recordReceiveGroup" << groupName << senderName << samples << sampleCount << channelCount << sampleRate;
          send(m);
        }
      }
    } else {
      message m;
      m << sender << "warning" << "The group " + groupName + " does not exist/not found." << true;
      send(m);
    }
  }
};

string join(message &msg, int start = 1) {
  string result;
  for (int i = start; i < msg.parts(); i++) {
    string word;
    msg >> word;
    result += word + " ";
  }
  return result;
}

bool checker(const message &msg, int parts, const string &sender, ServerState &server) {
  if (msg.parts() < parts) {
    message m;
    string txt = "Error! expected at least " + to_string(parts - 1) + " arguments";
    m << sender << "warning" << txt << true;
    server.send(m);
    return false;
  }
  return true;
}

void newUser(message &msg, const string &sender, ServerState &server) {
  if (!checker(msg, 4, sender, server)) return;

  string userName;
  msg >> userName;

  string password;
  msg >> password;
  server.newUser(userName, password, sender);
}

void login(message &msg, const string &sender, ServerState &server) {
  if (!checker(msg, 4, sender, server)) return;

  string userName;
  msg >> userName;
  string password;
  msg >> password;

  if (server.login(userName, password, sender)) {
    message m;
    cout << "User " << userName << " joins the chat server." << endl;
    m << sender << "warning" << "Your welcome!" << true;
    server.send(m);
  } else {
    message m;
    m << sender << "warning" << "Wrong userName/password." << false;
    server.send(m);
  }
}

void addFriend(message &msg, const string &sender, const string &senderName, ServerState &server) {
  if (!checker(msg, 3, sender, server)) return;

  string friendName;
  msg >> friendName;
  server.addFriend(senderName, friendName);
}

void chatTo(message &msg, const string &sender, const string &senderName, ServerState &server) {
  if (!checker(msg, 4, sender, server)) return;

  string friendName;
  msg >> friendName;
  string textContent = join(msg, 3);

  // Extract the id of the user
  string id = server.chatTo(senderName, friendName);

  if (id.size()) {
    message m;
    m << id << "receive" << senderName << textContent;
    server.send(m);
  } else {
    if (senderName == friendName) {
      message m;
      m << sender << "warning" << "You can not send messages to yourself!" << true;
      server.send(m);
    } else {
      message m;
      m << sender << "warning" << "The user " + friendName + " is offline/not exist/not your friend." << true;
      server.send(m);
    }
  }
}

void createGroup(message &msg, const string &sender, const string &senderName, ServerState &server) {
  if (!checker(msg, 3, sender, server)) return;

  string groupName;
  msg >> groupName;
  server.createGroup(groupName, senderName);
}

void addGroup(message &msg, const string &sender, const string &senderName, ServerState &server) {
  if (!checker(msg, 4, sender, server)) return;

  string groupName;
  msg >> groupName;
  string friendName;
  msg >> friendName;

  if (server.addGroup(groupName, sender, friendName, senderName)) {
    string id = server.chatTo(senderName, friendName);
    if (id.size()) {
      message m;
      m << id << "addedGroup" << "Your added in the group " << groupName;
      server.send(m);
    } else {
      cout << "The user " << friendName << " is offline/not exist/not your friend" << endl;
    }
  }
}

void groupChat(message &msg, const string &sender, const string &senderName, ServerState &server) {
  if (!checker(msg, 4, sender, server)) return;

  string groupName;
  msg >> groupName;
  string textContent = join(msg, 3);
  server.groupChat(groupName, sender, senderName, textContent);
}

void recordTo(message &msg, const string &sender, const string &senderName, ServerState &server) {
  if (checker(msg, 7, sender, server)) return;

  string friendName;
  msg >> friendName;
  vector<uint8_t> samples;
  msg >> samples;
  int sampleCount;
  msg >> sampleCount;
  int channelCount;
  msg >> channelCount;
  int sampleRate;
  msg >> sampleRate;
  server.recordTo(sender, senderName, friendName, samples, sampleCount, channelCount, sampleRate);
}

void recordGroup(message &msg, const string &sender, const string &senderName, ServerState &server) {
  if (checker(msg, 7, sender, server)) return;

  string groupName;
  msg >> groupName;
  vector<uint8_t> samples;
  msg >> samples;
  int sampleCount;
  msg >> sampleCount;
  int channelCount;
  msg >> channelCount;
  int sampleRate;
  msg >> sampleRate;
  server.recordGroup(sender, senderName, groupName, samples, sampleCount, channelCount, sampleRate);
}

void dispatch(message &msg, ServerState &server) {
  string sender;
  msg >> sender;

  if (!checker(msg, 3, sender, server)) return;

  string action;
  msg >> action;
  string senderName = server.getUserName(sender);

  if (action == "login") {
    login(msg, sender, server);
  } else if (action == "register") {
    newUser(msg, sender, server);
  } else if (action == "chatTo") {
    chatTo(msg, sender, senderName, server);
  } else if (action == "addFriend") {
    addFriend(msg, sender, senderName, server);
  } else if (action == "createGroup") {
    createGroup(msg, sender, senderName, server);
  } else if (action == "addGroup") {
    addGroup(msg, sender, senderName, server);
  } else if (action == "groupChat") {
    groupChat(msg, sender, senderName, server);
  } else if (action == "recordTo") {
    recordTo(msg, sender, senderName, server);
  } else if (action == "recordGroup") {
    recordGroup(msg, sender, senderName, server);
  } else {
    message m;
    m << sender << "warning" << "The action " + action + " is not supported/implemented." << true;
    server.send(m);
  }
}

int main(int argc, char *argv[]) {
  const string endpoint = "tcp://*:4242";
  context ctx;

  socket s(ctx, socket_type::xreply);
  s.bind(endpoint);

  ServerState state(s);
  state.newUser("sebas", "123", "");
  state.newUser("caro", "123", "");
  state.newUser("pepe", "123", "");

  while (true) {
    message req;
    s.receive(req);
    dispatch(req, state);
  }

  // Convert uint8 to int16
  // vector<uint8_t> v;
  // message rep;
  // s.receive(rep);
  // string id;
  // rep >> id;
  // rep >> v;
  //// BIG ENDIAN
  // for (int i = 0; i < v.size(); i+= 2) {
  //   int16_t tmp = (v[i] << 8) | v[i + 1];
  //   cout << tmp << " ";
  // }
  // cout << endl;

  return EXIT_SUCCESS;
}
