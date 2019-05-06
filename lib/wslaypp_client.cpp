
//
//  wslay.cpp
//  wslaypp
//
//  Created by Nik Wipper on 28.04.19.
//  Copyright © 2019 Nikolas Wipper. All rights reserved.
//

#include "wslaypp_client.hpp"

WebSocketClient::WebSocketClient(std::string host, std::string path, uint32_t port) :
dev_urand_("/dev/urandom")
{
    
    struct wslay_event_callbacks * wev = new wslay_event_callbacks;
    wev->recv_callback = event_recv_callback;
    wev->send_callback = event_send_callback;
    wev->genmask_callback = event_genmask_callback;
    wev->on_frame_recv_start_callback = event_on_frame_recv_start_callback;
    wev->on_frame_recv_chunk_callback = event_on_frame_recv_chunk_callback;
    wev->on_frame_recv_end_callback = event_on_frame_recv_end_callback;
    wev->on_msg_recv_callback = event_on_msg_recv_callback;
    wslay_event_context_client_init(&this->ctx, wev, this);
    // Connect to host
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res;
    int r = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (r != 0)
    {
        std::cerr << "Failed function 'getaddrinfo': " << gai_strerror(r) << std::endl;
        return;
    }
    for (struct addrinfo *rp = res; rp; rp = rp->ai_next)
    {
        this->fd_ = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (this->fd_ == -1)
            continue;
        while((r = connect(this->fd_, rp->ai_addr, rp->ai_addrlen)) == -1 && errno == EINTR);
        if(r == 0)
            break;
        this->close();
        this->fd_ = -1;
    }
    freeaddrinfo(res);
    // do http handshake
    if (!doHandshake(host, path, port))
    {
        std::cerr << "Failed HTTP handshake with server" << std::endl;
        this->close();
        return;
    }
}

ssize_t WebSocketClient::onReceiveWanted(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags)
{
    ssize_t r = this->receive(buf, len, flags);
    if(r == -1) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
        } else {
            wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        }
    } else if(r == 0) {
        wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
        r = -1;
    }
    return r;
}

ssize_t WebSocketClient::onSendWanted(wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags)
{
    ssize_t r = this->send(data, len, flags);
    if(r == -1)
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            wslay_event_set_error(ctx, WSLAY_ERR_WOULDBLOCK);
        else
            wslay_event_set_error(ctx, WSLAY_ERR_CALLBACK_FAILURE);
    }
    return r;
}

int WebSocketClient::genMask(wslay_event_context_ptr ctx, uint8_t *buf, size_t len)
{
    dev_urand_.read((char*)buf, len);
    return 0;
}

void WebSocketClient::onFrameReceiveStart(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg)
{
    // Empty
    return;
}

void WebSocketClient::onFrameReceiveChunk(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg)
{
    // Empty
    return;
}

void WebSocketClient::onFrameReceiveEnd(wslay_event_context_ptr ctx)
{
    // Empty
    return;
}

void WebSocketClient::onMessageReceive(wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg *arg)
{
    //Empty
    return;
}

ssize_t WebSocketClient::receive(uint8_t *data, size_t len, int flags)
{
    ssize_t r;
    while((r = recv(this->fd_, data, len, 0)) == -1 && errno == EINTR);
    return r;
}

ssize_t WebSocketClient::send(const uint8_t *data, size_t len, int flags)
{
    ssize_t r;
    // :: because send is already a function in this class (actually it is this function)
    while((r = ::send(this->fd_, data, len, 0)) == -1 && errno == EINTR);
    return r;
}

int WebSocketClient::close()
{
    // :: because close is already a function in this class (actually it is this function)
    return ::close(this->fd_);
}

bool WebSocketClient::doHandshake(std::string host, std::string path, uint32_t port)
{
    char buff[1024];
    
    char char_nonce[166];
    dev_urand_.read(char_nonce, 16);
    
    unsigned char const * char_nonce2 = (unsigned char const *)std::string(char_nonce).c_str();
    std::string nonce = base64_encode(char_nonce2, strlen(std::string(char_nonce).c_str()));
    
    snprintf(buff, sizeof(buff),
             "GET %s HTTPS/1.1\r\n"
             "Host: %s:%u\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Key: %s\r\n"
             "Sec-WebSocket-Version: 13\r\n",
             path.c_str(), host.c_str(), port, nonce.c_str());
    
    this->send(reinterpret_cast<const uint8_t *>(buff), strlen(buff), 0);
    
    unsigned char response[4096];
    this->receive(response, 4095, 0);
    
    std::vector<std::string> tokens;
    std::istringstream iss(std::string(reinterpret_cast<char*>(response)));
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(),
              std::back_inserter(tokens));
    
    if (tokens[1] != "101")
    {
        std::cerr << "Invalid response code" << std::endl;
        return false;
    }
    
    std::vector<std::string>::iterator i;
    bool hasUpgrade = false;
    bool hasConnection = false;
    bool hasWebSocketAccept = false;
    bool hasRightVersionNumber = false;
    for (i = tokens.begin(); i < tokens.end(); i++)
    {
        std::transform((*i).begin(), (*i).end(), (*i).begin(), ::tolower);
        if ((*i).compare("Upgrade:"))
        {
            i++;
            std::transform((*i).begin(), (*i).end(), (*i).begin(), ::tolower);
            if ((*i).compare("websocket"))
                hasUpgrade = true;
        }
        else if ((*i).compare("Connection:"))
        {
            i++;
            std::transform((*i).begin(), (*i).end(), (*i).begin(), ::tolower);
            if ((*i).compare("upgrade"))
                hasConnection = true;
        }
        else if ((*i).compare("Sec-WebSocket-Accept:"))
        {
            i++;
            if ((*i).compare(base64_encode(reinterpret_cast<unsigned const char*>(sha1(nonce).c_str()), sha1(nonce).length())))
                hasWebSocketAccept = true;
        }
        else if ((*i).compare("Sec-WebSocket-Protocol:"))
        {
            i++;
            if ((*i).compare("13"))
                hasRightVersionNumber = true;
        }
    }
    
    return hasUpgrade && hasConnection && hasWebSocketAccept && hasRightVersionNumber;
}

// define callbacks

ssize_t event_recv_callback(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, int flags, void *user_data)
{
    WebSocketClient * wsc = (WebSocketClient *)user_data;
    return wsc->onReceiveWanted(ctx, buf, len, flags);
}

ssize_t event_send_callback(wslay_event_context_ptr ctx, const uint8_t *data, size_t len, int flags, void *user_data)
{
    WebSocketClient * wsc = (WebSocketClient *)user_data;
    return wsc->onSendWanted(ctx, data, len, flags);
}

int event_genmask_callback(wslay_event_context_ptr ctx, uint8_t *buf, size_t len, void *user_data)
{
    WebSocketClient * wsc = (WebSocketClient *)user_data;
    return wsc->genMask(ctx, buf, len);
}

void event_on_frame_recv_start_callback(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_start_arg *arg, void *user_data)
{
    WebSocketClient * wsc = (WebSocketClient *)user_data;
    return wsc->onFrameReceiveStart(ctx, arg);
}

void event_on_frame_recv_chunk_callback(wslay_event_context_ptr ctx, const struct wslay_event_on_frame_recv_chunk_arg *arg, void *user_data)
{
    WebSocketClient * wsc = (WebSocketClient *)user_data;
    return wsc->onFrameReceiveChunk(ctx, arg);
}

void event_on_frame_recv_end_callback(wslay_event_context_ptr ctx, void *user_data)
{
    WebSocketClient * wsc = (WebSocketClient *)user_data;
    return wsc->onFrameReceiveEnd(ctx);
}

void event_on_msg_recv_callback(wslay_event_context_ptr ctx, const struct wslay_event_on_msg_recv_arg *arg, void *user_data)
{
    WebSocketClient * wsc = (WebSocketClient *)user_data;
    return wsc->onMessageReceive(ctx, arg);
}
