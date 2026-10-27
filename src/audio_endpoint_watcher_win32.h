#pragma once

#include <memory>

namespace axv {

// All methods, including destruction, belong to the owning GUI/COM thread.
class AudioEndpointWatcher {
public:
    AudioEndpointWatcher();
    ~AudioEndpointWatcher();
    AudioEndpointWatcher(const AudioEndpointWatcher&) = delete;
    AudioEndpointWatcher& operator=(const AudioEndpointWatcher&) = delete;

    bool start();
    void stop();
    bool take_changed();
    bool active() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace axv
