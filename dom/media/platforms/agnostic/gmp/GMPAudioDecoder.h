/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if !defined(GMPAudioDecoder_h_)
#define GMPAudioDecoder_h_

#include "GMPAudioDecoderProxy.h"
#include "MediaDataDecoderProxy.h"
#include "PlatformDecoderModule.h"
#include "mozIGeckoMediaPluginService.h"

namespace mozilla {

class AudioCallbackAdapter : public GMPAudioDecoderCallbackProxy {
public:
  explicit AudioCallbackAdapter(MediaDataDecoderCallbackProxy* aCallback)
   : mCallback(aCallback)
   , mLastStreamOffset(0)
   , mAudioFrameSum(0)
   , mAudioFrameOffset(0)
   , mMustRecaptureAudioPosition(true)
  {}

  // GMPAudioDecoderCallbackProxy
  void Decoded(const nsTArray<int16_t>& aPCM, uint64_t aTimeStamp, uint32_t aChannels, uint32_t aRate) override;
  void InputDataExhausted() override;
  void DrainComplete() override;
  void ResetComplete() override;
  void Error(GMPErr aErr) override;
  void Terminated() override;

  void SetLastStreamOffset(int64_t aStreamOffset) {
    mLastStreamOffset = aStreamOffset;
  }

private:
  MediaDataDecoderCallbackProxy* mCallback;
  int64_t mLastStreamOffset;

  int64_t mAudioFrameSum;
  int64_t mAudioFrameOffset;
  bool mMustRecaptureAudioPosition;
};

class GMPAudioDecoder : public MediaDataDecoder {
protected:
  GMPAudioDecoder(const AudioInfo& aConfig,
                  TaskQueue* aTaskQueue,
                  MediaDataDecoderCallbackProxy* aCallback,
                  AudioCallbackAdapter* aAdapter)
   : mConfig(aConfig)
   , mCallback(aCallback)
   , mGMP(nullptr)
   , mAdapter(aAdapter)
  {
  }

public:
  GMPAudioDecoder(const AudioInfo& aConfig,
                  TaskQueue* aTaskQueue,
                  MediaDataDecoderCallbackProxy* aCallback)
   : mConfig(aConfig)
   , mCallback(aCallback)
   , mGMP(nullptr)
   , mAdapter(new AudioCallbackAdapter(aCallback))
  {
  }

  RefPtr<InitPromise> Init() override;
  nsresult Input(MediaRawData* aSample) override;
  nsresult Flush() override;
  nsresult Drain() override;
  nsresult Shutdown() override;

protected:
  virtual void InitTags(nsTArray<nsCString>& aTags);
  virtual nsCString GetNodeId();

private:
  class GMPInitDoneRunnable : public nsRunnable
  {
  public:
    GMPInitDoneRunnable()
      : mInitDone(false),
        mThread(do_GetCurrentThread())
    {
    }

    NS_IMETHOD Run()
    {
      mInitDone = true;
      return NS_OK;
    }

    void Dispatch()
    {
      mThread->Dispatch(this, NS_DISPATCH_NORMAL);
    }

    bool IsDone()
    {
      MOZ_ASSERT(nsCOMPtr<nsIThread>(do_GetCurrentThread()) == mThread);
      return mInitDone;
    }

  private:
    bool mInitDone;
    nsCOMPtr<nsIThread> mThread;
  };

  void GetGMPAPI(GMPInitDoneRunnable* aInitDone);

  class GMPInitDoneCallback : public GetGMPAudioDecoderCallback
  {
  public:
    GMPInitDoneCallback(GMPAudioDecoder* aDecoder,
                        GMPInitDoneRunnable* aGMPInitDone)
      : mDecoder(aDecoder)
      , mGMPInitDone(aGMPInitDone)
    {
    }

    void Done(GMPAudioDecoderProxy* aGMP) override
    {
      if (aGMP) {
        mDecoder->GMPInitDone(aGMP);
      }
      mGMPInitDone->Dispatch();
    }

  private:
    RefPtr<GMPAudioDecoder> mDecoder;
    RefPtr<GMPInitDoneRunnable> mGMPInitDone;
  };
  void GMPInitDone(GMPAudioDecoderProxy* aGMP);

  const AudioInfo& mConfig;
  MediaDataDecoderCallbackProxy* mCallback;
  nsCOMPtr<mozIGeckoMediaPluginService> mMPS;
  GMPAudioDecoderProxy* mGMP;
  nsAutoPtr<AudioCallbackAdapter> mAdapter;
};

} // namespace mozilla

#endif // GMPAudioDecoder_h_
