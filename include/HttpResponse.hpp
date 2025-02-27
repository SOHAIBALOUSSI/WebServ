#pragma once

#include <bits/stdc++.h>
#include "HttpRequest.hpp"



class HttpResponse
{
#define READ_BUFFER_SIZE 1024000

private:
    

public:
    Config     serverConfig;
    long        statusCode; // pair <code, msg>
    std::string responseHeaders;
    std::string responseBody; // for error code pages now
    std::string requestedContent;

    std::string contentType;
    size_t      contentLength;
    std::string Date;
    std::string Server;
    std::string Connection;
    static std::map<int, std::string> statusCodesMap;
    static std::map<std::string, std::string> mimeTypes;

    void           generateAutoIndex(std::string& path, HttpRequest& request);
    std::string    generateErrorPage(size_t code);
    void        setErrorPage(std::map<int, std::string>& ErrPages);
    void    generateResponse(HttpRequest& request);
    void    prepareHeaders(std::string& path, HttpRequest& request);
    void    setResponseStatusCode(unsigned code) { statusCode = code; }
    
    
    void    GET(HttpRequest& request);
    void    POST(HttpRequest& request);
    void    DELETE(HttpRequest& request);
    HttpResponse(Config& conf);
    void    reset()
    {
        requestedContent.clear();
        statusCode = 200;
        contentLength = 0;
        contentType = "text/html";
        responseHeaders.clear();
        responseBody.clear();
        Date.clear();
        Connection.clear();
    }
    std::string    combineHeaders();
};

unsigned    checkFilePerms(std::string& path);