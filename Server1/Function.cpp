#include "Function.h"

bool checkConnection(int error_code) {
	if (error_code == -3 || error_code == 0) {
		cout << "Client disconnected from Server\n";
		return false;
	}

	return true;
}

void getFileList(vector<File>& file_list, string filename) {
	ifstream ifs(filename.c_str());
	if (!ifs.is_open()) {
		cout << "Failed to open " << filename << "\n";
		return;
	}

	File f;
	while (!ifs.eof()) {
		getline(ifs, f.name, ' ');
		getline(ifs, f.size);

		file_list.push_back(f);
	}

	ifs.close();
}

void sendFileList(vector<File>& file_list, CSocket& sConnector) {
	string filename = "download.txt";
	getFileList(file_list, filename);

	string list = file_list[0].name + " " + file_list[0].size + "\n";
	for (int i = 1; i < file_list.size(); i++) {
		if (i < file_list.size() - 1)
			list += file_list[i].name + " " + file_list[i].size + "\n";
		else
			list += file_list[i].name + " " + file_list[i].size;
	}
	int list_size = list.size();

	sConnector.Send(&list_size, sizeof(int), 0);
	sConnector.Send(list.c_str(), list_size, 0);
}


void uploadFile(string filename, CSocket& sConnector, bool& connection) {
	ifstream ifs(filename.c_str(), ios::binary);
	if (!ifs.is_open()) {
		cout << "Failed to open " << filename << "\n";
		return;
	}

	ifs.seekg(0, ios::end);
	int file_size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	sConnector.Send(&file_size, sizeof(int), 0);

	int bytes_sent = 0;
	int bytes_received = 0;
	int bytes_offset = 0;
	int chunk_size = 32768;
	char chunk[32768] = {};
	while (file_size >= chunk_size) {
		ifs.read(chunk, chunk_size);

		bytes_sent = chunk_size;
		sConnector.Send(chunk, bytes_sent, 0);
		sConnector.Receive((char*)&bytes_received, sizeof(int), 0);
		
		if (!checkConnection(bytes_received)) {
			connection = false;
			ifs.close();
			return;
		}

		//Send the remaining data until Client receives full data
		while (bytes_received < bytes_sent) {
			bytes_sent -= bytes_received;
			sConnector.Receive((char*)&bytes_received, sizeof(int), 0);

			if (!checkConnection(bytes_received)) {
				connection = false;
				ifs.close();
				return;
			}
		}

		file_size -= chunk_size;
	}

	bytes_sent = bytes_received = 0;
	char* remaining_chunk;
	while (file_size > 0) {
		remaining_chunk = new char[file_size];

		bytes_offset = bytes_sent - bytes_received;
		ifs.seekg(-bytes_offset, ios::cur);
		ifs.read(remaining_chunk, file_size);

		bytes_sent = file_size;
		sConnector.Send(remaining_chunk, bytes_sent, 0);
		sConnector.Receive((char*)&bytes_received, sizeof(int), 0);

		if (!checkConnection(bytes_received)) {
			connection = false;

			delete[] remaining_chunk;
			ifs.close();
			return;
		}

		file_size -= bytes_received;
		delete[] remaining_chunk;
	}

	ifs.close();
}

void uploadProcess(vector<File>& file_list, CSocket& sConnector) {
	int response = -1;
	int message_size = 0;
	bool connection = true;
	char* message;
	do {
		sConnector.Receive((char*)&message_size, sizeof(int), 0);

		if (!checkConnection(message_size)) return;

		message = new char[message_size + 1];
		sConnector.Receive(message, message_size, 0);
		message[message_size] = '\0';

		response = -1;
		for (int i = 0; i < file_list.size(); ++i)
			if (strcmp(file_list[i].name.c_str(), message) == 0) {
				response = 1;

				uploadFile(file_list[i].name, sConnector, connection);
				if (!connection) return;

				file_list.erase(file_list.begin() + i);
				break;
			}

		if (file_list.empty()) {
			response = 0;

			sConnector.Send(&response, sizeof(int), 0);
			cout << "Client has downloaded all files\n";
			delete[] message;
			break;
		}

		sConnector.Send(&response, sizeof(int), 0);

		delete[] message;
	} while (1);
}