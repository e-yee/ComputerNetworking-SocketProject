#include "stdafx.h"
#include "Client1.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;

CSocket sClient;

void signalHandler(int signum) {
	if (sClient.m_hSocket != INVALID_SOCKET) {
		int termination_code = -3;

		sClient.Send(&termination_code, sizeof(int), 0);
		sClient.Close();
	}

	ofstream file("input.txt");
	file.close();

	exit(signum);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
	signal(SIGINT, signalHandler);

	int n_ret_code = 0;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("<<< Fatal Error: MFC initialization failed >>>\n"));
		n_ret_code = 1;
		return n_ret_code;
	}
	if (AfxSocketInit() == FALSE) {
		cout << "<<< Fatal Error: Socket Library initialization failed >>>\n";
		return FALSE;
	}

	sClient.Create();
	while (sClient.Connect(_T("127.0.0.1"), 1234) == 0) {
		cout << "\r<<< Server connection failed >>>\n";
		return n_ret_code;	
	}
	cout << "<<< Server connection succeeded >>>\n\n\n";

	registerAvailableFile(sClient);

	queue<string> requesting_files;
	vector<string> downloaded;
	string file_download;
	int response = -1;
	while (1) {
		registerRequestingFiles(requesting_files);

		//Update requesting list
		if (!requesting_files.empty()) {
			while (find(downloaded.begin(), downloaded.end(), requesting_files.front()) != downloaded.end()) {
				requesting_files.pop();

				if (requesting_files.empty()) break;
			}
		}

		while (!requesting_files.empty()) {
			processRequestingFile(requesting_files, sClient);

			if (receiveDownloadData(requesting_files.front(), sClient))
				downloaded.push_back(requesting_files.front());

			requesting_files.pop();

			sClient.Receive(&response, sizeof(int), 0);
		}

		if (response == 0) {
			cout << "\n           <<< All available files downloaded. Disconnecting... >>>\n\n\n";
			break;
		}
	}

	sClient.Close();
	cout << "           <<< Disconnected! >>>\n";

	return n_ret_code;
}