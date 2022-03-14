//main file for testing
#include <iostream>
#include "main.h"

volatile sig_atomic_t signalStatus = 0;
void signalHandler(int signum);

int main(int argc, char** argv){

    //portaudio initialization
    PaError err = Pa_Initialize();
    if (err != paNoError){
        std::cout << "PortAudio initialization eror!";
        return 1;
    }

    pisurround::streamInfo playerInfo;
    playerInfo.sampleRate = 44100;
    playerInfo.bitWidth = 16;
    playerInfo.frameSize = 128;
    playerInfo.channelLayout = 2;

    //test port is 8090
    int port = 8090;
    try{
        auto mainPlayer = std::make_unique<pisurround::player>(port, playerInfo);

        //preload number of frames before stream is 3
        int preload = 2, sizeRead = 0;
        for (int i=0;i<preload;++i){
            sizeRead = mainPlayer->receive();
        }
        //stream start
        err = mainPlayer->openStream();
        if (err != paNoError){
            std::cout << "failed opening stream";
            return 1;
        }

        err = mainPlayer->startStream();
        if (err != paNoError){
            std::cout << "failed starting stream";
            return 1;
        }

        for (;;){
            if (signalStatus!=0) break;
            sizeRead = mainPlayer->receive();
        }
        
        //termination

        err = mainPlayer->stopStream();
        if (err != paNoError){
            std::cout << "failed starting stream";
            return 1;
        }

        err = mainPlayer->closeStream();
        if (err != paNoError){
            std::cout << "failed starting stream";
            return 1;
        }

        mainPlayer.reset();
    }
    catch(std::runtime_error& err){
        std::cerr << err.what() << "\n";
    }
    
    PaError err = Pa_Terminate();
    if (err != paNoError){
        std::cout << "PortAudio termination error!";
        return 1;
    }

    return 0;
}

//main functions

void signalHandler(int signum){
    signalStatus = signum;
    std::cout << signum << "signal received, terminating\n";
}
