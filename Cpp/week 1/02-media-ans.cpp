// 02-media-ans.cpp
// Refactored to follow ISP and LSP.

#include <iostream>
#include <string>
#include <stdexcept>
#include <memory>

using namespace std;

// ISP: Split fat Player interface into smaller, role-based interfaces.
class IPlayable {
public:
    virtual ~IPlayable() = default;
    virtual void play(const string& source) = 0;
    virtual void pause() = 0;
};

class IDownloadable {
public:
    virtual ~IDownloadable() = default;
    virtual void download(const string& sourceUrl) = 0;
};

class IRecordable {
public:
    virtual ~IRecordable() = default;
    virtual void record(const string& destination) = 0;
};

class ILiveStreamable {
public:
    virtual ~ILiveStreamable() = default;
    virtual void startStreaming(const string& streamUrl) = 0;
    virtual void stopStreaming() = 0;
};

// Classes now implement only the interfaces they support.
class AudioPlayer : public IPlayable, public IDownloadable {
    bool playing{false};
public:
    void play(const string& source) override {
        cout << "Playing audio from " << source << "\n";
        playing = true;
    }
    void pause() override {
        cout << "Pausing audio.\n";
        playing = false;
    }
    void download(const string& url) override {
        cout << "Downloading audio from " << url << "\n";
        /* pretend */
        (void)url;
    }
    bool isPlaying() const { return playing; }
};

// LSP: This class now has a clear and unsurprising contract.
// It only handles live streaming and recording, not generic "playing".
class CameraStreamPlayer : public ILiveStreamable, public IRecordable {
    bool is_streaming{false};
public:
    // LSP Fix: No more surprising 'play'. The action is 'startStreaming'.
    void startStreaming(const string& url) override {
        cout << "Starting live stream from " << url << "\n";
        is_streaming = true;
        (void)url;
    }

    void stopStreaming() override {
        cout << "Stopping live stream.\n";
        is_streaming = false;
    }

    void record(const string& dest) override {
        if (!is_streaming) {
            // Fail predictably if a precondition is not met.
            throw runtime_error("Cannot record: stream is not active.");
        }
        cout << "Recording stream to " << dest << "\n";
        (void)dest;
    }

    bool isStreaming() const { return is_streaming; }
};

int main() {
    AudioPlayer ap;
    ap.play("song.mp3");
    cout << "Audio playing: " << boolalpha << ap.isPlaying() << "\n";
    ap.download("http://example.com/song.mp3");
    ap.pause();
    cout << "Audio playing: " << boolalpha << ap.isPlaying() << "\n\n";

    CameraStreamPlayer cam;
    // No more surprising behavior or required ordering for a generic 'play' method.
    cam.startStreaming("rtsp://camera");
    cout << "Camera streaming: " << boolalpha << cam.isStreaming() << "\n";
    cam.record("recording.mkv");
    cam.stopStreaming();
    cout << "Camera streaming: " << boolalpha << cam.isStreaming() << "\n";

    return 0;
}
