#ifndef AUDIOQUEUE_H
#define AUDIOQUEUE_H

#include <SDL.h>
#include "RingBuffer.h"
#include <algorithm>

class AudioQueue
{
public:
	AudioQueue(int sampleRate, int sampleBufferLength, int sampleBufferPeriods)
	{
		//Allocating space for multiple buffers to avoid hiccups. For a short time SDL can keep producing sound even if emulator can't keep up.
		SampleBuffersQueue = new RingBuffer<short>(sampleBufferLength * sampleBufferPeriods);

		AudioSpec.freq = sampleRate;
		AudioSpec.format = AUDIO_S16SYS;
		AudioSpec.channels = 2;
		AudioSpec.samples = sampleBufferLength / 2;
		AudioSpec.callback = SoundSyntesizer;
		AudioSpec.userdata = this;

		SDL_OpenAudio(&AudioSpec, NULL);

		Mutex = SDL_CreateMutex();
		Signal = SDL_CreateCond();

		SDL_PauseAudio(0);
	}

	~AudioQueue()
	{
		SDL_PauseAudio(1);

		SDL_DestroyMutex(Mutex);
		SDL_DestroyCond(Signal);

		delete SampleBuffersQueue;
	}

	//Storing sample buffers in ring buffer - buffers written one after another. SDL will grab samples when he needs them
	void Append(const short *buffer, int length, bool sync = true)
	{
		SDL_LockMutex(Mutex);
		if (SampleBuffersQueue->Available() < length && sync)
		{
			SDL_CondWait(Signal, Mutex);//This is used in case we reached the end of the ring buffer - emulator wait for SDL callback to grab samples and free some space. 
										//Usually in this case we should overwrite old data in ring buffer. That's how ring buffer works.
										//Because this is sound we are talking about overwriting will corrupt sound.

			//Also as a bonus this blocking works as frame rate lock at ~60 fps. We don't need explicit delays to keep emulator working at the right speed.
		}
		SampleBuffersQueue->Write((short*)buffer, length);
		SDL_UnlockMutex(Mutex);
	}

private:

	static void SoundSyntesizer(void *udata, unsigned char *stream, int len)
	{
		AudioQueue *self = (AudioQueue*)udata;

		SDL_LockMutex(self->Mutex);
	
		self->SampleBuffersQueue->Read((short*)stream, std::min<int>(len / sizeof(short), self->SampleBuffersQueue->Used()));
	
		SDL_UnlockMutex(self->Mutex);
	
		SDL_CondSignal(self->Signal);
	}

	SDL_AudioSpec AudioSpec;
	SDL_mutex *Mutex;
	SDL_cond *Signal;
	RingBuffer<short> *SampleBuffersQueue;
};


#endif