// Coded by: Sebastian Duque Restrepo - Carolina Gomez Trejos
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <SFML/Audio.hpp>
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

vector<int16_t> to_int16(vector<uint8_t> &samples) {
  vector<int16_t> buffer(samples.size() / 2);
  int j = 0;
  for (int i = 0; i < samples.size(); i += 2) {
    int16_t tmp = (samples[i] << 8) | samples[i + 1];
    // cout << tmp << " ";
    buffer[j++] = tmp;
  }
  return buffer;
}

vector<uint8_t> to_uint8(const int16_t *buffer, int bufferSize) {
  vector<uint8_t> samples(bufferSize * 2);
  int j = 0;
  for (int i = 0; i < bufferSize; i++) {
    samples[j++] = (buffer[i] >> 8);
    samples[j++] = buffer[i];
  }
  return samples;
}

message record() {
  message msg;
  sf::SoundBufferRecorder recorder;
  unsigned int sampleRate = 44100;
  cout << "Press enter to record";
  recorder.start(sampleRate);
  cout << "Recording... press enter to stop";
  cin.ignore(10000, '\n');
  recorder.stop();
  const sf::SoundBuffer& buffer = recorder.getBuffer();
  const int16_t* samples = buffer.getSamples();
  int sampleCount = buffer.getSampleCount();
  vector<uint8_t> buffer_msg = to_uint8(samples, sampleCount);
  int channelCount = buffer.getChannelCount();
  msg << buffer_msg << sampleCount << channelCount << sampleRate;
  return msg;
}

bool soundCapture(string &act, socket &s) {
  if (act == "recordTo") {
    message msg = record();
    s.send(msg);
  }
  return false;
}

void play(vector<uint8_t> &samples, int sampleCount, int channelCount, const unsigned int sampleRate) {
  vector<int16_t> samples_int16 = to_int16(samples);
  int16_t *buffer = &samples_int16[0];
  sf::SoundBuffer sb;

  if(!sb.loadFromSamples(buffer, sampleCount, channelCount, sampleRate)) {
    cout << "Problems playing sound" << endl;
    return;
  }

  sf::Sound mysound(sb);
  cout << "Press enter to sound";
  cin.ignore(10000, '\n');
  mysound.play();
}

bool attends(message &rep) {
  string act;
  rep >> act;

  if (act == "receive") {
    string senderName;
    rep >> senderName;
    string textContent;
    rep >> textContent;
    cout << "*" << senderName << " say: " << textContent << endl;
  } else if (act == "groupReceive") {
    string groupName;
    rep >> groupName;
    string senderName;
    rep >> senderName;
    string text;
    rep >> text;
    cout << "[" << groupName << "] " << senderName << " say: " << text << endl;
  } else if (act == "recordReceive") {
    string senderName;
    rep >> senderName;
    vector<uint8_t> samples;
    rep >> samples;
    int sampleCount;
    rep >> sampleCount;
    int channelCount;
    rep >> channelCount;
    int sampleRate;
    rep >> sampleRate;
    cout << senderName << " records to you" << endl;
    play(samples, sampleCount, channelCount, sampleRate);
  } else if (act == "recordReceiveGroup") {
    string groupName;
    rep >> groupName;
    string senderName;
    rep >> senderName;
    vector<uint8_t> samples;
    rep >> samples;
    int sampleCount;
    rep >> sampleCount;
    int channelCount;
    rep >> channelCount;
    int sampleRate;
    rep >> sampleRate;
    cout << "[" << groupName << "] " << senderName << " records to you" << endl;
    play(samples, sampleCount, channelCount, sampleRate);
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
  // int16_t v2[] = {1, 2, 3, 4, 5, 6, 7};
  // vector<uint8_t> v = to_uint8(v2, 7);
  // vector<int16_t> cpy = to_int16(v);
  // int16_t *ans = &cpy[0];
  // for (int i = 0; i < 7; i++) {
  //   cout << ans[i] << " ";
  // }
  // cout << endl;

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
        if (!soundCapture(tokens[0], s)) {
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
