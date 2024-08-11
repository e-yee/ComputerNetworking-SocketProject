#include "stdafx.h"
#include "Server1.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;
CSocket sServer;

using namespace std;

void signalHandler(int signum) {
	if (sServer.m_hSocket != INVALID_SOCKET) {
		string termination_message = "404 - Server disconnection";
		int message_size = termination_message.size();

		sServer.Send(&message_size, sizeof(int), 0);
		sServer.Send(termination_message.c_str(), message_size, 0);
		sServer.Close();
	}
	exit(signum);
}

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[]) {
	signal(SIGINT, signalHandler);

	int n_ret_code = 0;

	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0)) {
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		n_ret_code = 1;
	}
	else {
		if (AfxSocketInit() == FALSE) {
			cout << "Socket Library initialization failed\n";
			return FALSE;
		}

		if (sServer.Create(1234, SOCK_STREAM, NULL) == 0) {
			cout << "Server initialization failed\n";
			cout << sServer.GetLastError();
			return FALSE;
		}
		else {
			cout << "Server initialization succeeded\n";

			if (sServer.Listen(1) == FALSE) {
				cout << "Server cannot listen on this port\n";
				sServer.Close();
				return FALSE;
			}
		}

		while (1) {
			CSocket sConnector;
			cout << "Waiting connection from Client\n";

			if (sServer.Accept(sConnector)) {
				cout << "Client connected to Server\n";

				vector<File> file_list;
				sendFileList(file_list, sConnector);

				uploadProcess(file_list, sConnector);
			}
			sConnector.Close();
		}
		sServer.Close();
	}

	return n_ret_code;
}
