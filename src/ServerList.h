// $Id$

#ifndef SERVERLIST_H
#define SERVERLIST_H

#include <vector>
#include <string>
#include <memory>

class QAbstractSocket;

void collectServers(std::vector<std::string>& servers);
std::auto_ptr<QAbstractSocket> openSocket(const std::string& socketName);

#endif
