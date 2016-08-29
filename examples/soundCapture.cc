#include <SFML/Audio.hpp>
#include <iostream>

using namespace std;

void record(sf::SoundBufferRecorder &recorder, const unsigned int &sampleRate) {
  // Audio capture is done in a separate thread, so we can block the main thread while it is capturing
  recorder.start(sampleRate);
  cout << "Recording... press enter to stop";
  cin.ignore(10000, '\n');
  recorder.stop();

  // Get the buffer containing the captured data
  const sf::SoundBuffer& buffer = recorder.getBuffer();

  // Display captured sound informations
  cout << "Sound information:" << endl;
  cout << " " << buffer.getDuration().asSeconds() << " seconds"           << endl;
  cout << " " << buffer.getSampleRate()           << " samples / seconds" << endl;
  cout << " " << buffer.getChannelCount()         << " channels"          << endl;
}

void soundCapture() {
  // Choose the sample rate
  unsigned int sampleRate;
  cout << "Please choose the sample rate for sound capture (44100) is CD quality: ";
  cin >> sampleRate;
  cin.ignore(10000, '\n');

  // Wait for user input
  cout << "Press enter to start recording audio";
  cin.ignore(1000, '\n');

  // Here we'll use an integrated custom recorder, which saves the captured data into a SoundBuffer
  sf::SoundBufferRecorder recorder;
  record(recorder, sampleRate);
  const sf::SoundBuffer& buffer = recorder.getBuffer();

  // Choose what to do with the recorded sound data
  char choice;
  cout << "What do you want to do with captured sound (p = play, s = save) ?";
  cin >> choice;
  cin.ignore(10000, '\n');

  if (choice == 's') {
    // Choose the filename
    string filename;
    cout << "Choose the file to create: ";
    getline(cin, filename);

    // Save the buffer
    buffer.saveToFile(filename);
  } else {
    // Create a sound instance and play it
    sf::Sound sound(buffer);
    sound.play();

    // Wait until finished
    while (sound.getStatus() == sf::Sound::Playing) {
      // Leave some CPU time for other threads
      sf::sleep(sf::milliseconds(100));

      // Display the playing position
      cout << "\rPlaying... " << sound.getPlayingOffset().asSeconds() << " sec        ";
      cout << flush;
    }
  }
  // finished
  cout << endl << "Done!" << endl;
}

int main() {
  // Check that the device can capture audio
  if (!sf::SoundRecorder::isAvailable()) {
    cerr << "Sorry, audio capture is not supported by your system" << endl;
    return EXIT_FAILURE;
  }

  soundCapture();

  // Wait until the user presses 'enter' key
  cout << "Press enter to exit..." << endl;
  cin.ignore(10000,'\n');

  return EXIT_SUCCESS;
}
