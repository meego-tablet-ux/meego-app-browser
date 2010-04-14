// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/audio_renderer_impl.h"

#include <math.h>

#include "chrome/common/render_messages.h"
#include "chrome/renderer/audio_message_filter.h"
#include "chrome/renderer/render_view.h"
#include "chrome/renderer/render_thread.h"
#include "media/base/filter_host.h"

namespace {

// We will try to fill 200 ms worth of audio samples in each packet. A round
// trip latency for IPC messages are typically 10 ms, this should give us
// plenty of time to avoid clicks.
const int kMillisecondsPerPacket = 200;

// We have at most 3 packets in browser, i.e. 600 ms. This is a reasonable
// amount to avoid clicks.
const int kPacketsInBuffer = 3;

}  // namespace

AudioRendererImpl::AudioRendererImpl(AudioMessageFilter* filter)
    : AudioRendererBase(),
      channels_(0),
      sample_rate_(0),
      sample_bits_(0),
      bytes_per_second_(0),
      filter_(filter),
      stream_id_(0),
      shared_memory_(NULL),
      shared_memory_size_(0),
      io_loop_(filter->message_loop()),
      stopped_(false),
      pending_request_(false) {
  DCHECK(io_loop_);
}

AudioRendererImpl::~AudioRendererImpl() {
}

base::TimeDelta AudioRendererImpl::ConvertToDuration(int bytes) {
  if (bytes_per_second_) {
    return base::TimeDelta::FromMicroseconds(
        base::Time::kMicrosecondsPerSecond * bytes / bytes_per_second_);
  }
  return base::TimeDelta();
}

bool AudioRendererImpl::IsMediaFormatSupported(
    const media::MediaFormat& media_format) {
  int channels;
  int sample_rate;
  int sample_bits;
  return ParseMediaFormat(media_format, &channels, &sample_rate, &sample_bits);
}

bool AudioRendererImpl::OnInitialize(const media::MediaFormat& media_format) {
  // Parse integer values in MediaFormat.
  if (!ParseMediaFormat(media_format,
                        &channels_,
                        &sample_rate_,
                        &sample_bits_)) {
    return false;
  }

  // Create the audio output stream in browser process.
  bytes_per_second_ = sample_rate_ * channels_ * sample_bits_ / 8;
  uint32 packet_size = bytes_per_second_ * kMillisecondsPerPacket / 1000;
  uint32 buffer_capacity = packet_size * kPacketsInBuffer;

  io_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &AudioRendererImpl::CreateStreamTask,
          AudioManager::AUDIO_PCM_LINEAR, channels_, sample_rate_, sample_bits_,
          packet_size, buffer_capacity));
  return true;
}

void AudioRendererImpl::OnStop() {
  AutoLock auto_lock(lock_);
  if (stopped_)
    return;
  stopped_ = true;

  // We should never touch |io_loop_| after being stopped, so post our final
  // task to clean up.
  io_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &AudioRendererImpl::DestroyTask));
}

void AudioRendererImpl::OnReadComplete(media::Buffer* buffer_in) {
  AutoLock auto_lock(lock_);
  if (stopped_)
    return;

  // TODO(hclam): handle end of stream here.

  // Use the base class to queue the buffer.
  AudioRendererBase::OnReadComplete(buffer_in);

  // Post a task to render thread to notify a packet reception.
  io_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(this, &AudioRendererImpl::NotifyPacketReadyTask));
}

void AudioRendererImpl::SetPlaybackRate(float rate) {
  DCHECK(rate >= 0.0f);

  AutoLock auto_lock(lock_);
  // Handle the case where we stopped due to |io_loop_| dying.
  if (stopped_) {
    AudioRendererBase::SetPlaybackRate(rate);
    return;
  }

  // We have two cases here:
  // Play: GetPlaybackRate() == 0.0 && rate != 0.0
  // Pause: GetPlaybackRate() != 0.0 && rate == 0.0
  if (GetPlaybackRate() == 0.0f && rate != 0.0f) {
    io_loop_->PostTask(FROM_HERE,
                       NewRunnableMethod(this, &AudioRendererImpl::PlayTask));
  } else if (GetPlaybackRate() != 0.0f && rate == 0.0f) {
    // Pause is easy, we can always pause.
    io_loop_->PostTask(FROM_HERE,
                       NewRunnableMethod(this, &AudioRendererImpl::PauseTask));
  }
  AudioRendererBase::SetPlaybackRate(rate);

  // If we are playing, give a kick to try fulfilling the packet request as
  // the previous packet request may be stalled by a pause.
  if (rate > 0.0f) {
    io_loop_->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &AudioRendererImpl::NotifyPacketReadyTask));
  }
}

void AudioRendererImpl::Seek(base::TimeDelta time,
                             media::FilterCallback* callback) {
  AudioRendererBase::Seek(time, callback);

  AutoLock auto_lock(lock_);
  if (stopped_)
    return;

  io_loop_->PostTask(FROM_HERE,
    NewRunnableMethod(this, &AudioRendererImpl::SeekTask));
}

void AudioRendererImpl::SetVolume(float volume) {
  AutoLock auto_lock(lock_);
  if (stopped_)
    return;
  io_loop_->PostTask(FROM_HERE,
      NewRunnableMethod(
          this, &AudioRendererImpl::SetVolumeTask, volume));
}

void AudioRendererImpl::OnCreated(base::SharedMemoryHandle handle,
                                  uint32 length) {
  DCHECK(MessageLoop::current() == io_loop_);

  AutoLock auto_lock(lock_);
  if (stopped_)
    return;

  shared_memory_.reset(new base::SharedMemory(handle, false));
  shared_memory_->Map(length);
  shared_memory_size_ = length;
}

void AudioRendererImpl::OnLowLatencyCreated(base::SharedMemoryHandle,
                                            base::SyncSocket::Handle, uint32) {
  // AudioRenderer should not have a low-latency audio channel.
  NOTREACHED();
}

void AudioRendererImpl::OnRequestPacket(uint32 bytes_in_buffer,
                                        const base::Time& message_timestamp) {
  DCHECK(MessageLoop::current() == io_loop_);

  {
    AutoLock auto_lock(lock_);
    DCHECK(!pending_request_);
    pending_request_ = true;

    // Use the information provided by the IPC message to adjust the playback
    // delay.
    request_timestamp_ = message_timestamp;
    request_delay_ = ConvertToDuration(bytes_in_buffer);
  }

  // Try to fill in the fulfill the packet request.
  NotifyPacketReadyTask();
}

void AudioRendererImpl::OnStateChanged(
    const ViewMsg_AudioStreamState_Params& state) {
  DCHECK(MessageLoop::current() == io_loop_);

  AutoLock auto_lock(lock_);
  if (stopped_)
    return;

  switch (state.state) {
    case ViewMsg_AudioStreamState_Params::kError:
      // We receive this error if we counter an hardware error on the browser
      // side. We can proceed with ignoring the audio stream.
      // TODO(hclam): We need more handling of these kind of error. For example
      // re-try creating the audio output stream on the browser side or fail
      // nicely and report to demuxer that the whole audio stream is discarded.
      host()->BroadcastMessage(media::kMsgDisableAudio);
      break;
    // TODO(hclam): handle these events.
    case ViewMsg_AudioStreamState_Params::kPlaying:
    case ViewMsg_AudioStreamState_Params::kPaused:
      break;
    default:
      NOTREACHED();
      break;
  }
}

void AudioRendererImpl::OnVolume(double volume) {
  // TODO(hclam): decide whether we need to report the current volume to
  // pipeline.
}

void AudioRendererImpl::CreateStreamTask(
    AudioManager::Format format, int channels, int sample_rate,
    int bits_per_sample, uint32 packet_size, uint32 buffer_capacity) {
  DCHECK(MessageLoop::current() == io_loop_);

  AutoLock auto_lock(lock_);
  if (stopped_)
    return;

  // Make sure we don't call create more than once.
  DCHECK_EQ(0, stream_id_);
  stream_id_ = filter_->AddDelegate(this);
  io_loop_->AddDestructionObserver(this);

  ViewHostMsg_Audio_CreateStream_Params params;
  params.format = format;
  params.channels = channels;
  params.sample_rate = sample_rate;
  params.bits_per_sample = bits_per_sample;
  params.packet_size = packet_size;
  params.buffer_capacity = buffer_capacity;

  filter_->Send(new ViewHostMsg_CreateAudioStream(0, stream_id_, params,
                                                  false));
}

void AudioRendererImpl::PlayTask() {
  DCHECK(MessageLoop::current() == io_loop_);

  filter_->Send(new ViewHostMsg_PlayAudioStream(0, stream_id_));
}

void AudioRendererImpl::PauseTask() {
  DCHECK(MessageLoop::current() == io_loop_);

  filter_->Send(new ViewHostMsg_PauseAudioStream(0, stream_id_));
}

void AudioRendererImpl::SeekTask() {
  DCHECK(MessageLoop::current() == io_loop_);

  filter_->Send(new ViewHostMsg_FlushAudioStream(0, stream_id_));
}

void AudioRendererImpl::DestroyTask() {
  DCHECK(MessageLoop::current() == io_loop_);

  // Make sure we don't call destroy more than once.
  DCHECK_NE(0, stream_id_);
  filter_->RemoveDelegate(stream_id_);
  filter_->Send(new ViewHostMsg_CloseAudioStream(0, stream_id_));
  io_loop_->RemoveDestructionObserver(this);
  stream_id_ = 0;
}

void AudioRendererImpl::SetVolumeTask(double volume) {
  DCHECK(MessageLoop::current() == io_loop_);

  AutoLock auto_lock(lock_);
  if (stopped_)
    return;
  filter_->Send(new ViewHostMsg_SetAudioVolume(0, stream_id_, volume));
}

void AudioRendererImpl::NotifyPacketReadyTask() {
  DCHECK(MessageLoop::current() == io_loop_);

  AutoLock auto_lock(lock_);
  if (stopped_)
    return;
  if (pending_request_ && GetPlaybackRate() > 0.0f) {
    DCHECK(shared_memory_.get());

    // Adjust the playback delay.
    base::Time current_time = base::Time::Now();

    // Save a local copy of the request delay.
    base::TimeDelta request_delay = request_delay_;
    if (current_time > request_timestamp_) {
      base::TimeDelta receive_latency = current_time - request_timestamp_;

      // If the receive latency is too much it may offset all the delay.
      if (receive_latency >= request_delay) {
        request_delay = base::TimeDelta();
      } else {
        request_delay -= receive_latency;
      }
    }

    // Finally we need to adjust the delay according to playback rate.
    if (GetPlaybackRate() != 1.0f) {
      request_delay = base::TimeDelta::FromMicroseconds(
          static_cast<int64>(ceil(request_delay.InMicroseconds() *
                                  GetPlaybackRate())));
    }

    uint32 filled = FillBuffer(static_cast<uint8*>(shared_memory_->memory()),
                               shared_memory_size_,
                               request_delay);
    pending_request_ = false;
    request_delay_ = base::TimeDelta();
    request_timestamp_ = base::Time();
    // Then tell browser process we are done filling into the buffer.
    filter_->Send(
        new ViewHostMsg_NotifyAudioPacketReady(0, stream_id_, filled));
  }
}

void AudioRendererImpl::WillDestroyCurrentMessageLoop() {
  DCHECK(MessageLoop::current() == io_loop_);

  // We treat the IO loop going away the same as stopping.
  AutoLock auto_lock(lock_);
  if (stopped_)
    return;

  stopped_ = true;
  DestroyTask();
}

