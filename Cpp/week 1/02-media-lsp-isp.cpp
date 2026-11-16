// 02-media-lsp-isp.cpp
// Messy starter: Fat interface + LSP surprises (violates ISP + LSP)

#include <iostream>
#include <string>
#include <stdexcept>

using namespace std;

class Player {
public:
    virtual ~Player() = default;
    virtual void play(const string& source) = 0;
    virtual void pause() = 0;
    virtual void record(const string& destination) = 0;  // many can't
    virtual void streamLive(const string& url) = 0;       // many can't
    virtual void download(const string& sourceUrl) = 0;   // many can't
};

class AudioPlayer : public Player {
    bool playing{false};
public:
    void play(const string& source) override { (void)source; playing = true; }
    void pause() override { playing = false; }
    void record(const string&) override { throw logic_error("AudioPlayer cannot record"); }       // LSP break
    void streamLive(const string&) override { throw logic_error("AudioPlayer cannot streamLive"); } // LSP break
    void download(const string& url) override { (void)url; /* pretend */ }
    bool isPlaying() const { return playing; }
};

class CameraStreamPlayer : public Player {
    bool liveStarted{false};
    bool playing{false};
public:
    void play(const string& src) override {
        (void)src;
        // Surprise: needs streamLive first for “real” play
        if (!liveStarted) {
            cout << "[WARN] playing without live stream started.\n";
        }
        playing = true;
    }
    void pause() override { playing = false; }
    void record(const string& dest) override { (void)dest; /* pretend */ }
    void streamLive(const string& url) override { (void)url; liveStarted = true; }
    void download(const string&) override { throw logic_error("CameraStreamPlayer cannot download"); } // LSP break
    bool isPlaying() const { return playing; }
    bool isLive() const { return liveStarted; }
};

int main() {
    AudioPlayer ap;
    ap.play("song.mp3");
    cout << "Audio playing: " << boolalpha << ap.isPlaying() << "\n";
    ap.pause();

    CameraStreamPlayer cam;
    cam.play("rtsp://camera");       // warning surprise
    cam.streamLive("rtsp://camera"); // required order
    cam.play("rtsp://camera");
    try { cam.download("http://file"); } catch (const exception& e) {
        cout << "[EXC] " << e.what() << "\n";
    }
    return 0;
}

// Time Complexity: O(1)
// Space Complexity: O(1)
// Pattern: Design - Player
// explanation:
/*
Player declares play, pause, record, streamLive, and download on every subclass.
AudioPlayer and CameraStreamPlayer implement each one with a flag write, a print, or an immediate throw.
No method walks a collection, so every call is constant time.
Each object stores only its playing flag and, for the camera, whether the live stream has started.
*/
