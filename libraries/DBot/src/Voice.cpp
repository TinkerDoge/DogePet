#include "Voice.h"
#include "../../../audio.h"

void Voice::play(String sound) {
    sound.toLowerCase();
    sound.trim();

    if (sound == "blink") {
        Audio::sfxBlink();
    }
    else if (sound == "happy" || sound == "joy") {
        Audio::playCuteHello(150);
    }
    else if (sound == "sad" || sound == "tired") {
        Audio::playCuteSleep(200);
    }
    else if (sound == "angry" || sound == "grumpy") {
        Audio::playCuteNo(150);
    }
    else if (sound == "notify" || sound == "attention") {
        Audio::sfxNotify();
    }
}
