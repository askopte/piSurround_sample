//Project is using namespace pisurround
#pragma once
#include <vector>
#include "portaudio.h"

namespace pisurround{

	//class declaration
	class lockFreeQueue;
	class player;

	struct streamInfo{

		int frameSize;
		int sampleRate;
		int bitWidth;
		int channelLayout;

	};

	class lockFreeQueue{

		int32_t **data;
		int queueSize;
		int frameSize;
		int channelLayout;
		std::atomic<int> count;
		int32_t **head;
		int32_t **back;
		int32_t *tmp;

		lockFreeQueue& operator=(lockFreeQueue& other);
		friend class pisurround::player;
		
	public:
		lockFreeQueue();
		lockFreeQueue(int queueSize, int frameSize, int channelLayout);
		~lockFreeQueue();

		void push(std::vector<int32_t>& inputFrame);
		int32_t* pop();
	};

	class player{

		player& operator=(player& other);
		streamInfo playerInfo;
		int sockfdData;
		std::unique_ptr<lockFreeQueue> frameQueue;
		std::vector<int32_t> sockBuffer;
		PaStream *stream;

	public:
		player();
		player(int port, streamInfo playerInfo);
		virtual ~player();

		int receive();
		PaError openStream();
		PaError startStream();
		PaError stopStream();
		PaError closeStream();

		static int PaPlayerCallBack(
			const void *inputBuffer,
			void *outputBuffer,
			unsigned long frameCount,
			const PaStreamCallbackTimeInfo* timeInfo,
			PaStreamCallbackFlags statusFlags, 
			void* userData
		);

	};
}