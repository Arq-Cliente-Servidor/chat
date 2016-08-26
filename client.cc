// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include "json.hpp"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;

int main(int argc, char *argv[]) {
  if (argc < 5) {
    cerr << "Invalid arguments" << endl;
    return EXIT_FAILURE;
  }

  string address(argv[1]);
  string action(argv[2]);
  string sckt("tcp://");
  sckt += address;

  context ctx;
  socket s(ctx, socket_type::xrequest);

  cout << "Connecting to: " << sckt << endl;
  s.connect(sckt);
  message msg;

  if (action == "chatTo") {
    string text, line;
    string from(argv[3]);
    string to(argv[4]);

    for (int i = 5; i < argc; i++) {
      line = string(argv[i]);
      text += line + " ";
    }

    msg << action << from << to << text;
  } else if (action == "addFriend") {
    string senderName(argv[3]);
    string friendName(argv[4]);
    msg << action << senderName << friendName;
  } else if (action == "createGroup") {
    string groupName(argv[3]);
    string senderName(argv[4]);
    msg << action << groupName << senderName;
  } else if (action == "addGroup") {
    string groupName(argv[3]);
    string senderName(argv[4]);
    string friendName(argv[5]);
    msg << action << groupName << senderName << friendName;
  } else if (action == "groupChat") {
    string text, line;
    string groupName(argv[3]);
    string senderName(argv[4]);

    for (int i = 5; i < argc; i++) {
      line = string(argv[i]);
      text += line + " ";
    }

    msg << action << groupName << senderName << text;
  } else if (action == "login" or action == "register") {
    string userName(argv[3]);
    string password(argv[4]);
    msg << action << userName << password;
  } else {
    cerr << "Wrong action" << endl;
    return EXIT_FAILURE;
  }

  s.send(msg);

  int console = fileno(stdin);
  poller poll;
  poll.add(s, poller::poll_in);
  poll.add(console, poller::poll_in);
  while (true) {
    if (poll.poll()) { // There are events in at least one of the sockets
      if (poll.has_input(s)) {
        // Handle input in socket
        message m;
        s.receive(m);
        cout << "Socket> " << m.parts() << endl;
      }
      if (poll.has_input(console)) {
        // Handle input from console
        string input;
        getline(cin, input);
        cout << "Input> " << input << endl;
      }
    }
  }

  // while (true) {
  //   message rep;
  //   s.receive(rep);
  //
  //   string act;
  //   rep >> act;
  //
  //   if (act == "receive") {
  //     string nameSender;
  //     rep >> nameSender;
  //     string textContent;
  //     rep >> textContent;
  //     cout << nameSender << " say: " << textContent << endl;
  //   } else if (act == "groupReceive") {
  //     string groupName;
  //     rep >> groupName;
  //     string senderName;
  //     rep >> senderName;
  //     string text;
  //     rep >> text;
  //     cout << "[" <<groupName << "] " << senderName << " say: " << text <<
  //     endl;
  //   } else if (act == "addedGroup") {
  //     string text;
  //     rep >> text;
  //     string groupName;
  //     rep >> groupName;
  //     cout << text << "[" << groupName << "]" << endl;
  //   }
  // }

  return EXIT_SUCCESS;
}
