// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <zmqpp/zmqpp.hpp>

#include "Util/Serializer.hpp"
#include "Util/ServerState.hpp"

using namespace std;
using namespace zmqpp;

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

void callRequestGroup(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string groupName;
  msg >> groupName;
  server.acceptCallGroup(sender, senderName, groupName);
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
  // cout << "PARTS: " << msg.parts() << endl;
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

void stopCallGroup(message &msg, const string &sender, const string &senderName, ServerState &server) {
  string groupName;
  msg >> groupName;
  server.stopCallGroup(sender, senderName, groupName);
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

void listGroup(const string &sender, const string &senderName, ServerState &server) {
  server.listGroup(senderName);
}

void dispatch(message &msg, ServerState &server) {
  string sender;
  msg >> sender;

  if (!checker(msg, 2, sender, server)) return;

  string action;
  msg >> action;
  string senderName = server.getUserName(sender);

  if (action != "stop" and action != "logout" and action != "listGroup" and !checker(msg, 3, sender, server))
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
  } else if (action == "callingGroup") {
    recordGroup(msg, sender, senderName, server, true);
  } else if (action == "callGroup") {
    callRequestGroup(msg, sender, senderName, server);
  } else if (action == "accept") {
    acceptCall(msg, sender, senderName, server);
  } else if (action == "stop") {
    stopCall(msg, sender, senderName, server);
  } else if (action == "stopGroup") {
    stopCallGroup(msg, sender, senderName, server);
  } else if (action == "removeFriend") {
    removeFriend(msg, sender, senderName, server);
  } else if (action == "leaveGroup") {
    leaveGroup(msg, sender, senderName, server);
  } else if (action == "warning") {
    warning(msg, sender, senderName, server);
  } else if (action == "listGroup") {
    listGroup(sender, senderName, server);
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
