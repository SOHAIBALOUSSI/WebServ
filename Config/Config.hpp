#pragma once

#include <bits/stdc++.h>


class Route {
public:
    std::string root; // a directory or a file from where the file should be searched
    std::set<std::string> allowed_methods;
    std::string redirect;
    std::string default_file; // default file to answer if the request is a directory
    bool dir_listing;
    unsigned long long max_body_size;
    std::vector<std::string> cgi_extensions;
    std::string upload_dir;

public:
    Route();
};


class Config
{
public:
    std::set<int> ports;
    std::string host;
    std::set<std::string> allowed_methods;
    unsigned long long max_body_size;
    std::vector<std::string> server_names;
    std::map<int, std::string> error_pages;
    std::map<std::string, Route> routes; 

public:
    std::string config_content;
    Config();
    void    printConfig();
    void    parse_config(std::string confing_content);

    void    insertPort(std::string value);
    void    insertHost(std::string value);
    void    insertAllowedMethods(std::string value);
    void    insertMaxBodySize(std::string value);
    void    insertServerNames(std::string value);
    void    insertErrorPages(std::string value);
    void    insertRoute(std::string value);

};

std::string readConfigFile(std::string path);

#endif