// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
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
  } else {
    string userName(argv[3]);
    string password(argv[4]);
    msg << action << userName << password;
  }

  s.send(msg);

  while (true) {
    message rep;
    s.receive(rep);

    string act;
    rep >> act;

    if (act == "receive") {
      string nameSender;
      rep >> nameSender;
      string textContent;
      rep >> textContent;

      cout << nameSender << " say: " << textContent << endl;
    }
  }

  return 0;
}
