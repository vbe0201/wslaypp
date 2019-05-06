//
//  wslay.hpp
//  wslaypp
//
//  Created by Nikolas Wipper on 04.05.19.
//  Copyright © 2019 Nikolas Wipper. All rights reserved.
//

#ifndef wslaypp_hpp
#define wslaypp_hpp

#include <wslay/wslay.h>

#include <string>
#include <vector>
#include <fstream>

class WebSocketClient
{
    wslay_event_context_ptr ctx;
    int fd_ = -1;
    std::fstream dev_urand_;
public:
    /*
     * This does basic initialization stuff as well as a HTTP handshake wich is not included in the
     * original wslay library
     */
    WebSocketClient(std::string host, std::string path, uint32_t port = 80);
    
    /*
     * This is invoked by wslay_event_recv() when it wants to receive more data from peer. It reads data
     * at most len bytes from peer and store them in buf and returns the number of bytes read.
     */
    virtual ssize_t onReceiveWanted(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags);
    
    /*
     * This is invoked by wslay_event_send() when it wants to send more data to peer. It sends data at
     * most len bytes to peer and returns the number of bytes sent.
     */
    virtual ssize_t onSendWanted(wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags);
    
    /*
     * This is invoked by wslay_event_send() when it wants new mask key. It fills exactly len bytes of
     * data in buf and returns 0 on success.
     */
    virtual int genMask(wslay_event_context_ptr ctx, uint8_t *buf, size_t len);
    
    /*
     * This is invoked by wslay_event_recv() when a new frame starts to be received. This callback
     * function is only invoked once for each frame
     */
    virtual void onFrameReceiveStart(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg);
    
    /*
     * This is invoked by wslay_event_recv() when a chunk of frame payload is received.
     */
    virtual void onFrameReceiveChunk(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg);
    
    /*
     * This is invoked by wslay_event_recv() when a frame is completely received.
     */
    virtual void onFrameReceiveEnd(wslay_event_context_ptr ctx);
    
    /*
     * This is invoked by wslay_event_recv() when a message is completely received.
     */
    virtual void onMessageReceive(wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg *arg);
    
    /*
     * This receives data of max size len and writes it to data with given flags. It returns the length
     * of the written data on success or -1 on error (errno is then set to the corresponding error)
     */
    virtual ssize_t receive(uint8_t *data, size_t len, int flags);
    
    /*
     * This sends data of size len with given flags. It returns the length of the send data on success
     * or -1 on error (errno is then set to the corresponding error)
     */
    virtual ssize_t send(const uint8_t *data, size_t len, int flags);
    
    /*
     * This destroys the socket used by the client. Returns zero on success and -1 on error (errno
     * is then set to the corresponding error)
     */
    int close();
private:
    /*
     * This performs a HTTP opening handshake with the server
     * as descriped here -> https://tools.ietf.org/html/rfc6455
     */
    bool doHandshake(std::string host, std::string path, uint32_t port);
};

/*
 * These are the actual callback functions that get called by wslay. This is necessary because one can't
 * pass a pointer to a non-static member-function. wslay passes a pointer to an object to all callback
 * functions wich is defined on intialization. By setting this object to 'this' we can use the object to
 * call the users event handler
 */

ssize_t event_recv_callback(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags, void *user_data);

ssize_t event_send_callback(wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data);

int event_genmask_callback(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data);

void event_on_frame_recv_start_callback(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg, void *user_data);

void event_on_frame_recv_chunk_callback(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg, void *user_data);

void event_on_frame_recv_end_callback(wslay_event_context_ptr ctx, void *user_data);

void event_on_msg_recv_callback(wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg *arg, void *user_data);

#endif /* wslaypp_hpp */
