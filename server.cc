// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <cassert>
#include <iostream>
#include <string>
#include <unordered_map>
#include <zmqpp/zmqpp.hpp>

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
  User(const string &name, const string &pwd, const string &id)
      : name(name), password(pwd), netId(id), connected(false) {}

  bool isPassword(const string &pwd) const { return password == pwd; }

  string getId() const { return netId; }

  void connect(const string &id) {
    connected = true;
    netId = id;
  }

  bool isConnected() const { return connected; }
  void addContact(const string &name) {
    if (!isFriend(name))
      contacts.push_back(name);
    else {
      cerr << "The user " << name << " is already your friend" << endl;
    }
  }

  bool isFriend(const string &contact) {
    for (const auto &it : contacts) {
      if (it == contact) return true;
    }
    return false;
  }
};

class ServerState {
private:
  // conenected users
  unordered_map<string, User> users;
  unordered_map<string, list<string>> groups;

public:
  ServerState() {}

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
      users[from].addContact(to);
      users[to].addContact(from);
      cout << "Friend added" << endl;
    }
    else cerr << "User not found" << endl;
  }

  void createGroup(const string &groupName, const string &name) {
    if (groups.count(groupName)) {
      cerr << "Group " << groupName << " is already created" << endl;
    }
    else {
      groups[groupName].push_back(name);
    }
  }

  bool belongsGroup(const string &groupName, const string &name) {
    if (!groups.count(groupName)) return false;
    for (const auto &it : groups[groupName]) {
      if (it == name) return true;
    }
    return false;
  }

  string getId(const string &name) {
    if (users.count(name)) {
      return users[name].getId();
    }
    else return "";
  }

  bool addGroup(const string &groupName, const string &name, const string &senderName) {
    if (!groups.count(groupName)) {
      cerr << "Group " << groupName << " does not exist" << endl;
      return false;
    }
    else if (belongsGroup(groupName, name)) {
      cerr << name << " already belongs to the group " << groupName << endl;
      return false;
    }
    else if (users.count(name) and users[senderName].isFriend(name)) {
      groups[groupName].push_back(name);
      return true;
    }
    else {
      cerr << "User not found/not is your friend" << endl;
      return false;
    }
  }

  void groupChat(const string &groupName, const string &senderName, const string &text, socket &s) {
    if (groups.count(groupName)) {
      for (const auto &it : groups[groupName]) {
        string id = getId(it);
        if (id.size()) {
          message m;
          m << id << "groupReceive" << senderName << text;
          s.send(m);
        }
      }
    }
    else {
      cerr << "Group not found" << endl;
    }
  }
};

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

void chatTo(message &msg, ServerState &server, socket &s) {
  string nameSender;
  msg >> nameSender;

  string nameFriend;
  msg >> nameFriend;

  string text;
  msg >> text;

  // Extract the id of the user
  string id = server.chatTo(nameSender, nameFriend);

  if (id.size()) {
    message m;
    m << id << "receive" << nameSender << text;
    s.send(m);
  } else {
    cout << "The user " << nameFriend << " is offline/not exist/not your friend" << endl;
  }
}

void createGroup(message &msg, ServerState &server) {
  string groupName;
  msg >> groupName;
  string senderName;
  msg >> senderName;
  server.createGroup(groupName, senderName);
}

void addGroup(message &msg, ServerState &server, socket &s) {
  string groupName;
  msg >> groupName;
  string senderName;
  msg >> senderName;
  string friendName;
  msg >> friendName;

  if (server.addGroup(groupName, friendName, senderName)) {
    string id = server.chatTo(senderName, friendName);
    if (id.size()) {
      message m;
      m << id << "addedGroup" << "Your added in the group " << groupName;
      s.send(m);
    } else {
      cout << "The user " << friendName << " is offline/not exist/not your friend" << endl;
    }
  }
}

void groupChat(message &msg, ServerState &server, socket &s) {
  string groupName;
  msg >> groupName;
  string senderName;
  msg >> senderName;
  string text;
  msg >> text;
  server.groupChat(groupName, senderName, text, s);
}

void addFriend(message &msg, ServerState &server) {
  string nameSender;
  msg >> nameSender;
  string nameFriend;
  msg >> nameFriend;
  server.addFriend(nameSender, nameFriend);
}

void dispatch(message &msg, ServerState &server, socket &s) {
  // TODO validar id con sendername, mensajes a todo
  assert(msg.parts() > 2);
  string sender;
  msg >> sender;

  string action;
  msg >> action;

  if (action == "login") {
    login(msg, sender, server);
  } else if (action == "register") {
    newUser(msg, sender, server);
  } else if (action == "chatTo") {
    chatTo(msg, server, s);
  } else if (action == "addFriend") {
    addFriend(msg, server);
  } else if (action == "createGroup") {
    createGroup(msg, server);
  } else if (action == "addGroup") {
    addGroup(msg, server, s);
  } else if (action == "groupChat") {
    groupChat(msg, server, s);
  } else {
    cerr << "Action not supported/implemented" << endl;
  }
}

int main(int argc, char *argv[]) {
  const string endpoint = "tcp://*:4242";
  context ctx;

  socket s(ctx, socket_type::xreply);
  s.bind(endpoint);

  ServerState state;
  state.newUser("sebas", "123", "");
  state.newUser("caro", "123", "");

  while (true) {
    message req;
    s.receive(req);
    dispatch(req, state, s);
  }

  cout << "Finished." << endl;
}
