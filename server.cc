// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <cmath>
#include <iostream>
#include <sstream>
#include <stack>
#include <string>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;

int main(int argc, char *argv[]) {
  const string endpoint = "tcp://*:4242";
  context ctx;
  socket s(ctx, socket_type::xreply);
  s.bind(endpoint);
  while (true) {
    cout << "Receiving message..." << endl;
    message req;
    s.receive(req);
    cout << "Parts: " << req.parts() << endl;
    string identity;
    int data;
    req >> identity >> data;
    cout << "Identity: " << identity << endl;
    cout << "Data: " << data << endl;
  }
  cout << "Finished." << endl;
}
