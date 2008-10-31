// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MediaPlayerPrivateChromium_h
#define MediaPlayerPrivateChromium_h

#if ENABLE(VIDEO)

#include "MediaPlayer.h"

namespace WebCore {

    class MediaPlayerPrivate : public Noncopyable {
    public:
        MediaPlayerPrivate(MediaPlayer*);
        ~MediaPlayerPrivate();

        IntSize naturalSize() const;
        bool hasVideo() const;

        void load(const String& url);
        void cancelLoad();

        void play();
        void pause();    

        bool paused() const;
        bool seeking() const;

        float duration() const;
        float currentTime() const;
        void seek(float time);
        void setEndTime(float);

        void setRate(float);
        void setVolume(float);

        int dataRate() const;

        MediaPlayer::NetworkState networkState() const;
        MediaPlayer::ReadyState readyState() const;

        float maxTimeBuffered() const;
        float maxTimeSeekable() const;
        unsigned bytesLoaded() const;
        bool totalBytesKnown() const;
        unsigned totalBytes() const;

        void setVisible(bool);
        void setRect(const IntRect&);

        void loadStateChanged();
        void didEnd();

        void paint(GraphicsContext*, const IntRect&);

        static void getSupportedTypes(HashSet<String>& types);
        static bool isAvailable();

    private:
        MediaPlayer* m_player;
        MediaPlayer::NetworkState m_networkState;
        MediaPlayer::ReadyState m_readyState;
    };

}

#endif

#endif // MediaPlayerPrivateChromium_h
