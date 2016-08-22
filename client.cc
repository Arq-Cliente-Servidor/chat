// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <iostream>
#include <string>
#include <zmqpp/zmqpp.hpp>

using namespace std;
using namespace zmqpp;

int main(int argc, char *argv[]) {
  const string endpoint = "tcp://localhost:4242";
  context ctx;
  socket s(ctx, socket_type::xrequest);
  s.connect(endpoint);
  int n = atoi(argv[1]);
  for (int i = 0; i < 1000; i++) {
    message req;
    req << n * i;
    s.send(req);
  }
  int x;
  cin >> x;
  cout << "Finished." << endl;
  return 0;
}
