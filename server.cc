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

  void disconnect(const string &id) {
    connected = false;
    netId = "";
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

  bool removeContact(const string &name) {
    list<string>::iterator contact;
    for (contact = contacts.begin(); contact != contacts.end(); ++contact) {
      if (*contact == name) {
        contacts.erase(contact);
        return true;
      }
    }
    return false;
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
    } else if (isUser(friendName) and isFriend(senderName, friendName) and isConnected(friendName)) {
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
          string act = (isCall) ? "callReceive" : "recordReceiveGroup";
          m << id << act << groupName << senderName << samples << sampleCount << channelCount << sampleRate;
          send(m);
        }
      }
      message msg;
      msg << sender << "warning" << "The voice message has been sent" << true;
      send(msg);
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

void logout(const string &sender, const string &senderName, ServerState &server) {
  message msg;
  server.disconnect(sender, senderName);
  msg << sender << "warning" << "Good bye! " + senderName + " :D" << false;
  server.send(msg);
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

void recordTo(message &msg, const string &sender, const string &senderName,
              ServerState &server, bool isCall = false) {

  if (msg.parts() < 7 or !checker(msg, 7, sender, server)) return;

  string friendName;
  msg >> friendName;
  vector<int16_t> samples;
  msg >> samples;
  int sampleCount;
  msg >> sampleCount;
  int channelCount;
  msg >> channelCount;
  int sampleRate;
  msg >> sampleRate;
  server.recordTo(sender, senderName, friendName, samples, sampleCount, channelCount, sampleRate, isCall);
}

void recordGroup(message &msg, const string &sender, const string &senderName,
                 ServerState &server, bool isCall = false) {
  if (msg.parts() < 7 or !checker(msg, 7, sender, server)) return;

  string groupName;
  msg >> groupName;
  vector<int16_t> samples;
  msg >> samples;
  int sampleCount;
  msg >> sampleCount;
  int channelCount;
  msg >> channelCount;
  int sampleRate;
  msg >> sampleRate;
  server.recordGroup(sender, senderName, groupName, samples, sampleCount, channelCount, sampleRate, isCall);
}

void callRequest(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string friendName;
  msg >> friendName;
  if (server.isFriend(senderName, friendName)) {
    string friendId = server.getId(friendName);
    message m;
    m << friendId << "callRequest" << senderName << " is calling press (y = accept, n = reject) ";
    server.send(m);
  } else {
    message m;
    string txt = (senderName == friendName)
                     ? "You can not make calls yourself!"
                     : friendName + " is offline/not exist/not your friend";
    m << sender << "warning" << txt << true;
    server.send(m);
  }
}

void acceptCall(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string friendName;
  msg >> friendName;
  bool accept;
  msg >> accept;
  message rep, rep2;
  string friendId = server.getId(friendName);

  if (!accept) {
    rep << friendId << "callResponse" << senderName << " has rejected your call." << false;
    server.send(rep);
  } else {
    rep << friendId << "callResponse" << senderName << " has accepted your call." << true;
    server.send(rep);
    rep2 << sender << "callResponse" << friendName << " is ready for the call." << true;
    server.send(rep2);
  }
}

void stopCall(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string friendName;
  msg >> friendName;
  string friendId = server.getId(friendName);
  message rep;
  rep << friendId << "stop" << senderName;
  server.send(rep);
}

void removeFriend(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string friendName;
  msg >> friendName;
  string friendId = server.getId(friendName);
  if (server.removeFriend(senderName, friendName)) {
    server.removeFriend(friendName, senderName);
    message m1, m2;
    m1 << sender << "warning" << "The user " + friendName + " has been successfully removed" << true;
    server.send(m1);
    m2 << friendId << "warning" << "The user " + senderName + " has deleted you from her/his contact list" << true;
    server.send(m2);
  } else {
    message m;
    m << sender << "warning" << "The user " + friendName + " is not your friend/not exists/not found" << true;
    server.send(m);
  }
}

void leaveGroup(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string groupName;
  msg >> groupName;
  server.leaveGroup(groupName, sender, senderName);
}

void warning(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string friendName;
  msg >> friendName;
  message m;
  string friendId = server.getId(friendName);
  m << friendId << "warning" << "The user " + senderName + " is already in a call" << true;
  server.send(m);
}

void dispatch(message &msg, ServerState &server) {
  string sender;
  msg >> sender;

  if (!checker(msg, 2, sender, server)) return;

  string action;
  msg >> action;
  string senderName = server.getUserName(sender);

  if (action != "stop" and action != "logout" and !checker(msg, 3, sender, server))
    return;

  if (action == "login") {
    login(msg, sender, server);
  } else if (action == "logout") {
    logout(sender, senderName, server);
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
  } else if (action == "callTo") {
    callRequest(msg, sender, senderName, server);
  } else if (action == "calling") {
    recordTo(msg, sender, senderName, server, true);
  } else if (action == "callGroup") {
    recordGroup(msg, sender, senderName, server, true);
  } else if (action == "accept") {
    acceptCall(msg, sender, senderName, server);
  } else if (action == "stop") {
    stopCall(msg, sender, senderName, server);
  } else if (action == "removeFriend") {
    removeFriend(msg, sender, senderName, server);
  } else if (action == "leaveGroup") {
    leaveGroup(msg, sender, senderName, server);
  } else if (action == "warning") {
    warning(msg, sender, senderName, server);
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

  return EXIT_SUCCESS;
}
