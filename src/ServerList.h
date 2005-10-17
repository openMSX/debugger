// $Id$

#ifndef SERVERLIST_H
#define SERVERLIST_H

#include <vector>
#include <string>

void collectServers(std::vector<std::string>& servers);
int openSocket(const std::string& socketName);

#endif
