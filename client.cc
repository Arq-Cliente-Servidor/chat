// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <zmqpp/zmqpp.hpp>

#include "Util/Serializer.hpp"

using namespace std;
using namespace zmqpp;

vector<string> tokenize(string &input) {
  stringstream ss(input);
  vector<string> result;
  string s;
  while (ss >> s)
    result.push_back(s);
  return result;
}

bool attends(message &rep) {
  string act;
  rep >> act;

  if (act == "receive") {
    string nameSender;
    rep >> nameSender;
    string textContent;
    rep >> textContent;
    cout << "*" << nameSender << " say: " << textContent << endl;
  } else if (act == "groupReceive") {
    string groupName;
    rep >> groupName;
    string senderName;
    rep >> senderName;
    string text;
    rep >> text;
    cout << "[" <<groupName << "] " << senderName << " say: " << text << endl;
  } else if (act == "addGroup") {
    string text;
    rep >> text;
    string groupName;
    rep >> groupName;
    cout << text << "[" << groupName << "]" << endl;
  } else if (act == "addFriend") {
    string name;
    rep >> name;
    cout << name << " and you are friends now" << endl;
  } else if (act == "warning") {
    string txt;
    rep >> txt;
    cout << txt << endl;
    bool ok;
    rep >> ok;
    return ok;
  }
  return true;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    cerr << "Invalid arguments" << endl;
    return EXIT_FAILURE;
  }

  string address(argv[1]);
  string action(argv[2]);
  string userName(argv[3]);
  string password(argv[4]);
  string sckt("tcp://");
  sckt += address;

  context ctx;
  socket s(ctx, socket_type::xrequest);

  if (action != "login" and action != "register") {
    cerr << "invalid operation, usage: <address> <action> <username> <password>" << endl;
    return EXIT_FAILURE;
  }

  cout << "Connecting to: " << sckt << endl;
  s.connect(sckt);

  message entry;
  entry << action << userName << password;
  s.send(entry);

  int console = fileno(stdin);
  poller poll;
  poll.add(s, poller::poll_in);
  poll.add(console, poller::poll_in);

  while (true) {
    if (poll.poll()) { // There are events in at least one of the sockets
      if (poll.has_input(s)) {
        // Handle input in socket
        message msg;
        s.receive(msg);
        if (!attends(msg))
          return EXIT_FAILURE;
      }
      if (poll.has_input(console)) {
        // Handle input from console
        string input;
        getline(cin, input);
        vector<string> tokens = tokenize(input);

        message msg;
        for (const auto &str : tokens) {
          msg << str;
        }
        s.send(msg);
      }
    }
  }

  return EXIT_SUCCESS;
}
