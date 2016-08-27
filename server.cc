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

  bool addContact(const string &name) {
    if (!isFriend(name)) {
      contacts.push_back(name);
      return true;
    } else {
      return false;
    }
  }

  bool isFriend(const string &user) {
    for (const auto &contact : contacts) {
      if (contact == user)
        return true;
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

  void send(message &msg) {
    sckt.send(msg);
  }

  void newUser(const string &name, const string &pwd, const string &id) {
    if (users.count(name)) {
      cerr << "User " << name << " is already registered" << endl;
    } else {
      users[name] = User(name, pwd, id);
      cout << "User " << name << " has successfully registered" << endl;
    }
  }

  bool login(const string &name, const string &pwd, const string &id) {
    if (users.count(name)) {
      // User is registered
      bool ok = users[name].isPassword(pwd);
      if (ok)
        users[name].connect(id);
      return ok;
    }
    return false;
  }

  string chatTo(const string &senderName, const string &friendName) {
    if (users.count(friendName) and users[friendName].isConnected() and users[senderName].isFriend(friendName)) {
      return users[friendName].getId();
    } else
      return "";
  }

  void addFriend(const string &from, const string &to) {
    if (users.count(to)) {
      if (users[from].addContact(to)) {
        users[to].addContact(from);
        cout << "Friend added" << endl;
      } else {
        cerr << "The user " << to << " is already your friend" << endl;
      }
    } else
      cerr << "User not found" << endl;
  }

  void createGroup(const string &groupName, const string &name) {
    if (groups.count(groupName)) {
      cerr << "Group " << groupName << " is already created" << endl;
    } else {
      groups[groupName].push_back(name);
      cout << "Group created" << endl;
    }
  }

  bool belongsGroup(const string &groupName, const string &name) {
    if (!groups.count(groupName))
      return false;
    for (const auto &it : groups[groupName]) {
      if (it == name)
        return true;
    }
    return false;
  }

  string getId(const string &name) {
    if (users.count(name)) {
      return users[name].getId();
    } else
      return "";
  }

  string getUserName(const string &id) {
    for (const auto &user : users) {
      if (user.second.getId() == id)
        return user.first;
    }
    return "";
  }

  bool addGroup(const string &groupName, const string &friendName, const string &senderName) {
    if (!groups.count(groupName)) {
      cerr << "Group " << groupName << " does not exist" << endl;
      return false;
    } else if (belongsGroup(groupName, friendName)) {
      cerr << friendName << " already belongs to the group " << groupName << endl;
      return false;
    } else if (users.count(friendName) and users[senderName].isFriend(friendName) and users[friendName].isConnected()) {
      groups[groupName].push_back(friendName);
      cout << friendName << " has been added to the group " << groupName << endl;
      return true;
    } else {
      cerr << "User not found/offline/not is your friend" << endl;
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
          sckt.send(m);
          cout << "The message has been sent to the group" << endl;
        }
      }
    } else {
      cerr << "Group not found" << endl;
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

void login(message &msg, const string &sender, ServerState &server) {
  string userName;
  msg >> userName;
  string password;
  msg >> password;

  if (server.login(userName, password, sender)) {
    cout << "User " << userName << " joins the chat server" << endl;
  } else {
    cerr << "Wrong user/password" << endl;
  }
}

void newUser(message &msg, const string &sender, ServerState &server) {
  string userName;
  msg >> userName;

  string password;
  msg >> password;
  server.newUser(userName, password, sender);
}

void chatTo(message &msg, const string &senderName, ServerState &server) {
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
    cout << "The user " << friendName << " is offline/not exist/not your friend" << endl;
  }
}

void createGroup(message &msg, const string &senderName, ServerState &server) {
  string groupName;
  msg >> groupName;
  server.createGroup(groupName, senderName);
}

void addGroup(message &msg, const string &senderName, ServerState &server) {
  string groupName;
  msg >> groupName;
  string friendName;
  msg >> friendName;

  if (server.addGroup(groupName, friendName, senderName)) {
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
  string groupName;
  msg >> groupName;
  string textContent = join(msg, 3);
  server.groupChat(groupName, sender, senderName, textContent);
}

void addFriend(message &msg, const string &senderName, ServerState &server) {
  string friendName;
  msg >> friendName;
  server.addFriend(senderName, friendName);
}

void dispatch(message &msg, ServerState &server) {
  // TODO mandar mensajes de cada accion al cliente
  assert(msg.parts() > 2);
  string sender;
  msg >> sender;

  string action;
  msg >> action;
  string senderName = server.getUserName(sender);

  if (action == "login") {
    login(msg, sender, server);
  } else if (action == "register") {
    newUser(msg, sender, server);
  } else if (action == "chatTo") {
    chatTo(msg, senderName, server);
  } else if (action == "addFriend") {
    addFriend(msg, senderName, server);
  } else if (action == "createGroup") {
    createGroup(msg, senderName, server);
  } else if (action == "addGroup") {
    addGroup(msg, senderName, server);
  } else if (action == "groupChat") {
    groupChat(msg, sender, senderName, server);
  } else {
    cerr << "Action not supported/implemented" << endl;
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
