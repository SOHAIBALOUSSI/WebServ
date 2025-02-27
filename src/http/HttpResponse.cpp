#include "../../include/HttpResponse.hpp"
#include "../../include/HttpRequest.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#define CRLF "\r\n"


std::map<int, std::string> HttpResponse::statusCodesMap = {
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {204, "No Content"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {304, "Not Modified"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {413, "Request Entity Too Large"},
    {414, "URI Too Long"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {503, "Service Unavailable"},
    {505, "HTTP Version Not Supported"}
};

std::map<std::string, std::string> HttpResponse::mimeTypes = {
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".txt", "text/plain"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".bmp", "image/bmp"},
    {".ico", "image/x-icon"},
    {".svg", "image/svg+xml"},
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},
    {".mp3", "audio/mpeg"},
    {".mp4", "video/mp4"},
    {".mpeg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"}
};

HttpResponse::HttpResponse(Config& conf) : serverConfig(conf) , contentType("text/html"), contentLength(0) {}

std::string getContentType(std::string path) {
    size_t pos = path.find_last_of('.');
    if (pos == std::string::npos) {    
        return "application/octet-stream";
    }
    std::string ext = path.substr(pos);
    std::map<std::string, std::string>::iterator mimeIt; 
    if ((mimeIt = HttpResponse::mimeTypes.find(ext)) != HttpResponse::mimeTypes.end()) {
        return mimeIt->second;
    }
    return "application/octet-stream";
}

unsigned    checkFilePerms(std::string& path) {
    if (access(path.c_str(), F_OK) == 0) {
        if (access(path.c_str(), R_OK) != 0)
            return 403;
        return 200;
    }
    return 404;
}

long getFileContentLength(std::string& path) {
    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) != 0) {
        std::cerr << "cannot access file at " << path << std::endl;
        return -1;
    }
    return fileStat.st_size;
}


std::string getCurrentDateHeader() {
    std::time_t now = std::time(0);
    // Convert to GMT/UTC time
    std::tm* gmt = std::gmtime(&now);
    char buffer[100];
    // Format the date according to IMF-fixdate (RFC 7231)
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buffer);
}

std::string HttpResponse::combineHeaders() {
    std::stringstream ss;
    ss << "HTTP/1.1 " << statusCode << " " << statusCodesMap[statusCode] << CRLF
       << "Date: " << getCurrentDateHeader() << CRLF
       << "Content-Type: " << contentType << CRLF
       << "Content-Length: " << std::to_string(contentLength) << CRLF
       << "Server: EnginX" << CRLF
       << "Connection: " << Connection << CRLF << CRLF;
    return ss.str();
}

std::string getConnetionType(std::map<std::string, std::string>& headers) {
    auto it = headers.find("Connection");
    if (it == headers.end()) { return "close"; }
    if (it != headers.end()) {
        return it->second.empty() ? "close" : it->second;
    }
    return "close";
}

void    HttpResponse::prepareHeaders(std::string& path, HttpRequest& request) {
    contentType = getContentType(path);
    contentLength = getFileContentLength(path);
    if (contentLength == -1) { statusCode = 500; }
    responseHeaders = combineHeaders();
}

bool isDirectory(const std::string& path) {
    struct stat statbuf;
    if (stat(path.c_str(), &statbuf) != 0)
        return false;
    return S_ISDIR(statbuf.st_mode);
}


void HttpResponse::generateAutoIndex(std::string& path, HttpRequest& request) {
    std::string autoIndexContent = "<html><head><title>Index of " + path + "</title></head><body>";
    autoIndexContent += "<h1>Index of " + request.getOriginalUri() + "</h1><hr><pre>";
    
    DIR *dir = opendir(path.c_str());
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            std::string name = ent->d_name;
            if (name == ".") continue; // i can also skip ".."
            std::string fullPath = path + name;
            struct stat statbuf;
            if (stat(fullPath.c_str(), &statbuf) == 0) {
                autoIndexContent += "<a href=\"" + name + (S_ISDIR(statbuf.st_mode) ? "/" : "") + "\">" + name + "</a>\n";
            }
        }
        if (-1 == closedir(dir)) {
            std::cout << "not closed\n";
        }
    }
    
    autoIndexContent += "</pre><hr></body></html>";
    
    statusCode = 200;
    contentType = "text/html";
    contentLength = autoIndexContent.size();
    
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n"
       << "Date: " << getCurrentDateHeader() << "\r\n"
       << "Content-Type: text/html\r\n"
       << "Content-Length: " << contentLength << "\r\n"
       << "Connection: " << Connection << "\r\n\r\n"
       << autoIndexContent;
    
    responseHeaders = ss.str();
}

void HttpResponse::POST(HttpRequest& request) {
    if (statusCode == 200) {
        statusCode = 201;
        std::stringstream ss;
        ss << "HTTP/1.1 " << statusCode << " " << statusCodesMap[statusCode] << CRLF
            << "Server: EnginX" << CRLF
            << "Connection: " << Connection << CRLF << CRLF;
        responseHeaders = ss.str();

    }
    else {
        setErrorPage(request.getConfig().getErrorPages());
    }
}


void HttpResponse::GET(HttpRequest& request) {
    std::string &path = request.getUriPath();
    unsigned code = checkFilePerms(path);

    if (isDirectory(path)) {
        if (path.back() != '/') {
            statusCode = 301;
            responseHeaders = "HTTP/1.1 301 Moved Permanently\r\nLocation: " + request.getOriginalUri() + "/\r\n\r";
            responseBody.clear();
            return ;
        }
        
        std::set<std::string>& methods = request.getRouteConf().getAllowedMethods();

        if (methods.count(request.getMethod()) == 0) {
            statusCode = 405;
            setErrorPage(request.getConfig().getErrorPages());
            return ;
        }
        std::string defaultFile = request.getDefaultIndex();
        std::string pathToIndex = path + defaultFile;
        if (!defaultFile.empty() && checkFilePerms(pathToIndex) == 200)
        {
            request.setURIpath(pathToIndex);
            prepareHeaders(request.getUriPath(), request);
        }
        else {
            if (request.getautoIndex() == true) {
                generateAutoIndex(path, request);
            }
            else {
                statusCode = 403;
                setErrorPage(request.getConfig().getErrorPages());
            }   
        }
        return ;
    }

    if (code == 200) {
        prepareHeaders(path, request);
    }
    else {
        setErrorPage(request.getConfig().getErrorPages());
    }    
}


std::string    HttpResponse::generateErrorPage(size_t code) {
    std::stringstream ss;
    ss << "<h1> <center>" << code << " " <<  statusCodesMap[code] << " <center></h1>";
    return ss.str();
}

void    HttpResponse::setErrorPage(std::map<int, std::string>& ErrPages) {

    char buffer[4096];
    std::map<int, std::string>::iterator ErrPage = ErrPages.find(statusCode);
    
    if (ErrPage != ErrPages.end()) {
        std::string errPagePath = ErrPage->second;
        contentType = getContentType(errPagePath);
        contentLength = getFileContentLength(errPagePath);
        if (contentLength == -1) { statusCode = 500; }
        responseHeaders = combineHeaders();
        unsigned code = checkFilePerms(errPagePath);
        if (code == 200) {
            std::ifstream errfile(errPagePath, std::ios::binary);
            errfile.read(buffer, 4096);
            std::streamsize bytesRead = errfile.gcount();
            if (bytesRead > 0) {
                responseBody.append(buffer, bytesRead);
            }
            else if (errfile.eof() && responseBody.empty()) {
                errfile.close();
            }
        }
        else {
            responseBody = generateErrorPage(statusCode);
            contentLength = responseBody.size();
            responseHeaders = combineHeaders();
        }
    }
    else {
        responseBody = generateErrorPage(statusCode);
        contentLength = responseBody.size();    
        responseHeaders = combineHeaders();
    }
    return ;
}


void    HttpResponse::DELETE(HttpRequest& request) {
    if (statusCode == 200) {
        // cgi ??
    
        if (isDirectory(requestedContent) || requestedContent.find(request.getUploadDir()) != 0) {
            statusCode = 403;
            setErrorPage(request.getConfig().getErrorPages());
            return ;
        }
        if (remove(requestedContent.c_str()) != 0) {
            statusCode = 500;
            setErrorPage(request.getConfig().getErrorPages());
        }
        else {
            statusCode = 204;
            std::stringstream ss;
            ss << "HTTP/1.1 " << statusCode << " " << statusCodesMap[statusCode] << CRLF
                << "Server: EnginX" << CRLF
                << "Connection: " << Connection << CRLF << CRLF;
            responseHeaders = ss.str();
        }
    }
    else {
        setErrorPage(request.getConfig().getErrorPages());
    }
}

void  HttpResponse::generateResponse(HttpRequest& request) {
    statusCode = request.getStatusCode();
    
    Config& conf = request.getConfig();
    requestedContent = request.getUriPath();
    Connection = request.getHeaderValue("Connection") == "close" ? "close" : "keep-alive";
    if (statusCode >= 400) {
        return setErrorPage(conf.getErrorPages());
    }
    
    std::string& method = request.getMethod();
    if (method == "GET")
        GET(request);
    else if (method == "POST")
        POST(request);
    else if (method == "DELETE")
        DELETE(request);
}