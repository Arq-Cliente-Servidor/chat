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
};

class ServerState {
private:
  // conenected users
  unordered_map<string, User> users;
  unordered_map<string, list<User>> groups;

public:
  ServerState() {}

  void newUser(const string &name, const string &pwd, const string &id) {
    if (users.count(name)) {
      cout << "User is already registered" << endl;
    } else {
      users[name] = User(name, pwd, id);
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

  string chatTo(const string &name) {
    if (users.count(name) and users[name].isConnected()) {
      return users[name].getId();
    } else
      return "";
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
    cerr << "Wrong user/password " << endl;
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
  string id = server.chatTo(nameFriend);

  if (id.size()) {
    msg << "receive" << id << nameSender << text;
    s.send(msg);
  } else {
    cout << "The user " << nameFriend << " is offline/not exist" << endl;
  }
}

void dispatch(message &msg, ServerState &server, socket &s) {
  assert(msg.parts() > 2);
  string sender;
  msg >> sender;

  string action;
  msg >> action;

  if (action == "login") {
    login(msg, sender, server);
  }
  if (action == "register") {
    newUser(msg, sender, server);
  }
  if (action == "chatTo") {
    chatTo(msg, server, s);
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

  while (true) {
    message req;
    s.receive(req);
    dispatch(req, state, s);
  }

  cout << "Finished." << endl;
}
