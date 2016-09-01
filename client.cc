// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <SFML/Audio.hpp>
#include <vector>
#include <thread>
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

message record(const string &act, const string &friendName, bool isRecord = true) {
  message msg;
  sf::SoundBufferRecorder recorder;
  unsigned int sampleRate = 44100;

  if (isRecord) {
    cout << "Press enter to record" << endl;
    cin.ignore(10000, '\n');
    recorder.start(sampleRate);

    cout << "Recording... press enter to stop" << endl;
    cin.ignore(10000, '\n');
    recorder.stop();
  } else {
    recorder.start(sampleRate);
    this_thread::sleep_for(chrono::milliseconds(500));
    recorder.stop();
  }

  const sf::SoundBuffer& buffer = recorder.getBuffer();
  const int16_t* samples = buffer.getSamples();
  int sampleCount = buffer.getSampleCount();
  vector<int16_t> buffer_msg(samples, samples + sampleCount);
  int channelCount = buffer.getChannelCount();

  msg << act << friendName << buffer_msg << sampleCount << channelCount << sampleRate;
  return msg;
}

void recordCallSend(bool &t, const string &act, const string &friendName, socket &s) {
  while(!t) {
    message msg = record(act, friendName, false);
    s.send(msg);
  }
}

bool soundCapture(vector<string> &tokens, socket &s) {
  if (tokens.size() > 1 and (tokens[0] == "recordTo" or tokens[0] == "recordGroup")) {
    message msg = record(tokens[0], tokens[1]);
    s.send(msg);
  }
  return false;
}

// Recibir sound como parametro por referencia desde el main a play
void play(sf::Sound &mysound, sf::SoundBuffer &sb, vector<int16_t> &samples, int sampleCount, int channelCount, const unsigned int sampleRate) {
  int16_t *buffer = &samples[0];

  if(!sb.loadFromSamples(buffer, sampleCount, channelCount, sampleRate)) {
    cout << "Problems playing sound" << endl;
    return;
  }

  mysound.setBuffer(sb);
  // cout << "Press enter to play the voice message" << endl;
  // cin.ignore(1, '\n');

  mysound.play();
  // while (mysound.getStatus() == sf::Sound::Playing) {
  // cout << "Playing..." << endl;
  // }
  // cout << endl << "End of message" << endl;
}

bool attends(message &rep, sf::Sound &mysound, sf::SoundBuffer &sb, socket &s, thread *&recorder, bool &onPlay) {
  string act;
  rep >> act;

  if (act == "receive") {
    string senderName;
    rep >> senderName;
    string textContent;
    rep >> textContent;
    cout << "*" << senderName << " says: " << textContent << endl;
  } else if (act == "groupReceive") {
    string groupName;
    rep >> groupName;
    string senderName;
    rep >> senderName;
    string text;
    rep >> text;
    cout << "[" << groupName << "] " << senderName << " says: " << text << endl;
  } else if (act == "recordReceive") {
    string senderName;
    rep >> senderName;
    vector<int16_t> samples;
    rep >> samples;
    int sampleCount;
    rep >> sampleCount;
    int channelCount;
    rep >> channelCount;
    int sampleRate;
    rep >> sampleRate;
    //cout << senderName << " records to you" << endl;
    play(mysound, sb, samples, sampleCount, channelCount, sampleRate);
  } else if (act == "recordReceiveGroup") {
    string groupName;
    rep >> groupName;
    string senderName;
    rep >> senderName;
    vector<int16_t> samples;
    rep >> samples;
    int sampleCount;
    rep >> sampleCount;
    int channelCount;
    rep >> channelCount;
    int sampleRate;
    rep >> sampleRate;
    cout << "[" << groupName << "] " << senderName << " records to you" << endl;
    play(mysound, sb, samples, sampleCount, channelCount, sampleRate);
  } else if (act == "callRequest") {
    string friendName;
    rep >> friendName;
    string txt;
    rep >> txt;
    cout << friendName << txt << endl;
    char c;
    message req;

    while (cin >> c and c != 'y' and c != 'Y' and c != 'N' and c != 'n') {
      cin.ignore();
    }

    if (tolower(c) == 'y') {
      req << "accept" << friendName << true;
    } else {
      req << "accept" << friendName << false;
    }
    s.send(req);
  } else if (act == "callResponse") {
    string txt;
    rep >> txt;
    bool callReady;
    rep >> callReady;
    if (callReady) {

    } else {
      cout << "The call could not be performed" << endl;
    }
  } else if (act == "stop") {
    if (!onPlay) {
      cout << "There is not an active call." << endl;
    } else {
      onPlay = false;
      recorder->join();
      recorder = nullptr;
      string friendName;
      rep >> friendName;
      cout << "The call with " << friendName << " has finished" << endl;
    }
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
  // TODO evitar logue varias veces una vez que ya esta logueado
  // reutilizar record
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

  bool onPlay = false;
  sf::SoundBuffer sb;
  sf::Sound mysound;
  thread *recorder = nullptr;

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
        if (!attends(msg, mysound, sb, s, recorder, onPlay))
          return EXIT_FAILURE;
      }
      if (poll.has_input(console)) {
        // Handle input from console
        string input;
        getline(cin, input);
        vector<string> tokens = tokenize(input);
        // if (tokens[0] == "callTo" and tokens.size() > 1) {

          // t = false;
          // th = new thread(recordCallSend, ref(t), ref(tokens[0]), ref(tokens[1]), ref(s));
          // th->join();
          // th = nullptr;
        // }
        if (!soundCapture(tokens, s)) {
          message msg;
          for (const auto &str : tokens) {
            msg << str;
          }
          s.send(msg);
        }
      }
    }
  }

  return EXIT_SUCCESS;
}
