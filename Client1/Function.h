#pragma once

#include "stdafx.h"
#include <afxsock.h>

using namespace std;

void signalHandler(int signum);
void displayDownloadProgress(float progress, float total, string filename);
void registerAvailableFile(CSocket& sClient);
void registerRequestingFiles(queue<string>& requesting_files);
bool processRequestingFile(queue<string> requesting_files, CSocket& sClient);
bool receiveDownloadData(string filename, CSocket& sClient);