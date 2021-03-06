//
// Created by Christopher Wood on 3/5/17.
//

#include <vector>
#include <iostream>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../name.h"
#include "../timer.h"
#include "attack_client.h"

int
AttackClient::LoadNames(NameReader *reader)
{
    while (nameReader_HasNext(reader)) {
        Name *name = nameReader_Next(reader);
        names.push_back(name);
    }
    return names.size();
}

void
AttackClient::Run()
{
    uint8_t sizeBuffer[2];
    int index = 0;
    for (std::vector<Name *>::iterator itr = names.begin(); itr != names.end(); itr++) {
        Name *name = *itr;

        // Serialize and send the name
        PARCBuffer *nameWireFormat = name_GetWireFormat(name, name_GetSegmentCount(name));
        uint8_t *nameBuffer = (uint8_t *) parcBuffer_Overlay(nameWireFormat, 0);
        size_t nameLength = parcBuffer_Remaining(nameWireFormat);

        sizeBuffer[0] = (nameLength >> 8) & 0xFF;
        sizeBuffer[1] = (nameLength >> 0) & 0xFF;

        // Record the time it was sent
        struct timespec start = timerStart();
        times.push_back(start);

        // Send it
        if (send(sockfd, (void *) sizeBuffer, sizeof(sizeBuffer), 0) != sizeof(sizeBuffer)) {
//            std::cerr << "failed to send packet " << index << std::endl;
        }
        if (send(sockfd, (void *) nameBuffer, nameLength, 0) != nameLength) {
//            std::cerr << "failed to send packet " << index << std::endl;
//            perror("Error!");
        }
        index++;
    }
}

void *
runClient(void *arg)
{
    AttackClient *client = (AttackClient *) arg;
    client->Run();
    return NULL;
}