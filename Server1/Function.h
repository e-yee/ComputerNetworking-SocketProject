#pragma once

#include "stdafx.h"
#include <afxsock.h>

using namespace std;

struct File {
	string name;
	string size;
};

void signalHandler(int signum);
bool checkConnection(int error_code);
void getFileList(vector<File>& file_list, string filename);
void sendFileList(vector<File>& file_list, CSocket& sConnector);
void uploadFile(string filename, CSocket& sConnector, bool& connection);
void uploadProcess(vector<File>& file_list, CSocket& sConnector);