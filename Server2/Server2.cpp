#include "stdafx.h"
#include "Server2.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CWinApp theApp;

using namespace std;
using std::cout;

CSocket sServer;

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

		DWORD thread_ID;
		HANDLE thread_status;
		CSocket sConnector;

		cout << "Waiting connection from Client\n";
		while (1) {
			if (sServer.Accept(sConnector)) {
				cout << "Client connected to Server\n";

				SOCKET* h_connected = new SOCKET();
				*h_connected = sConnector.Detach();
				thread_status = CreateThread(NULL, 0, uploadProcess, h_connected, 0, &thread_ID);
			}
		}
		sServer.Close();
	}

	return n_ret_code;
}