//functions for player class and associated lockFreeQueue
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include "main.h"

//class lockFreeQueue

pisurround::lockFreeQueue::lockFreeQueue(){
    throw std::runtime_error("lockFreeQueue Constructor receive no stream info");
}

pisurround::lockFreeQueue::lockFreeQueue(int queueSize, int frameSize, int channelLayout){
    this->queueSize = queueSize;
    this->frameSize = frameSize;
    this->channelLayout = channelLayout;
    data = new int32_t*[queueSize];
    for (int i=0;i<queueSize;++i){
        data[i] = new int32_t[frameSize * channelLayout];
    }
    count == 0;
    this->head = &data[0];
    this->back = &data[0];
}

pisurround::lockFreeQueue::~lockFreeQueue(){
    for (int i=0;i<queueSize;++i){
        delete[] data[i];
    }
    delete[] data;
}

void pisurround::lockFreeQueue::push(std::vector<int32_t>& inputFrame){
    if (inputFrame.size() != frameSize){
        throw std::runtime_error("framesize mismatch");
    }
    if (count == queueSize){
        throw std::runtime_error("input frame queue full");
    }
    for (int i=0;i<frameSize;++i){
        for (int j=0;j<channelLayout; ++j){
            int index = 2*i+j;
            (*back)[index] = inputFrame[index];
        }
    }
    ++back;
    if (back == data + queueSize) back = data;
    count += 1;
    return;
}

int32_t* pisurround::lockFreeQueue::pop(){
    if (count == 0){
        throw std::runtime_error("empty frame queue");
    }
    count -= 1;
    tmp = *head;
    ++head;
    if (head == data + queueSize) head = data;
    return tmp;
}

//class player

pisurround::player::player(){

    throw std::runtime_error("Player Constructor receive no stream info");

};

pisurround::player::player(int port, streamInfo playerInfo){

    this->playerInfo = playerInfo;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == 0){
        throw std::runtime_error("socket initialization failure");
    }

    sockaddr_in addr;
    int addrLen = sizeof(addr);
    memset(&addr, 0, addrLen);
    addr.sin_family = AF_INET;
    addr.sin_port = port;

    if (bind(sockfd, (const sockaddr*)&addr, addrLen) == -1){
        throw std::runtime_error("bind error");
    }

    std::cout << "Listening at port " << port << ".\n";

    if (listen(sockfd, 2) == -1){
        throw std::runtime_error("socket listen error");
    }

    sockfdData = accept(sockfd, (sockaddr*)&addr, (socklen_t *)&addrLen);
    if (sockfdData == 0){
        throw std::runtime_error("data socket initialization error");
    } 

    //initialize lockFreeQueue
    frameQueue = std::make_unique<lockFreeQueue>(10, playerInfo.frameSize * playerInfo.channelLayout);

    //initialize sockBuffer
    sockBuffer = std::vector<int32_t>(playerInfo.frameSize * playerInfo.channelLayout, 0);
}

pisurround::player::~player(){
    shutdown(sockfdData, 2);
}

int pisurround::player::receive(){
    int sizeRead = recv(sockfdData, sockBuffer.data(), sizeof(int32_t) * playerInfo.frameSize * playerInfo.channelLayout, 0);
    frameQueue->push(sockBuffer);
    return sizeRead;
}

PaError pisurround::player::openStream(){
    PaError err = Pa_OpenDefaultStream(
        &stream, 
        0, 
        2, 
        paInt32,
        playerInfo.sampleRate, 
        playerInfo.frameSize, 
        pisurround::player::PaPlayerCallBack,
        &frameQueue
    );
    return err;
}

PaError pisurround::player::startStream(){
    PaError err = Pa_StartStream(stream);
    return err;
}

PaError pisurround::player::stopStream(){
    PaError err = Pa_StopStream(stream);
    return err;
}

PaError pisurround::player::closeStream(){
    PaError err = Pa_CloseStream(stream);
    return err;
}

int pisurround::player::PaPlayerCallBack(
    const void *inputBuffer,
	void *outputBuffer,
	unsigned long frameCount,
	const PaStreamCallbackTimeInfo* timeInfo,
	PaStreamCallbackFlags statusFlags, 
	void* userData
){

    (void) inputBuffer;
    int32_t *out = (int*) outputBuffer;
    lockFreeQueue* outputQueue = (lockFreeQueue*)userData;
    int32_t* queueData = outputQueue->pop();
    int channelSize = outputQueue->channelLayout;
    
    for (int i=0;i<frameCount;++i){
        for (int j=0;j<channelSize;++i){
            out[channelSize*i+j] = queueData[channelSize*i+j];
        }
    }

    return 0;

}

