#include "Function.h"

bool checkDisconnection(int error_code) {
	if (error_code == 0 || error_code == -3) {
		cout << "Client disconnected from Server\n";
		return true;
	}
	
	return false;
}

void getDownloadableFiles(vector<File>& downloadable_list, string filename) {
	ifstream ifs(filename.c_str());
	if (!ifs.good()) {
		cout << "Fail to open " << filename << "!\n";
		return;
	}

	File f;
	while (!ifs.eof()) {
		getline(ifs, f.name, ' ');
		getline(ifs, f.size);

		downloadable_list.push_back(f);
	}

	ifs.close();
}

void sendDownloadableFiles(vector<File> downloadable_list, CSocket& sConnector) {
	string message = "";
	for (int i = 0; i < downloadable_list.size(); ++i) {
		if (i != downloadable_list.size() - 1)
			message += downloadable_list[i].name + " " + downloadable_list[i].size + "\n";
		else
			message += downloadable_list[i].name + " " + downloadable_list[i].size;
	}
	int message_size = message.size();

	sConnector.Send(&message_size, sizeof(int), 0);
	sConnector.Send(message.c_str(), message_size, 0);
}

void receiveRequestingFiles(vector<File>& requesting_list, CSocket& sConnector, int& start, bool& connection) {
	sConnector.Receive((char*)&start, sizeof(int), 0);

	int message_size = 0;
	sConnector.Receive((char*)&message_size, sizeof(int), 0);
	if (checkDisconnection(message_size)) {
		connection = false;
		return;
	}
	
	char* message = new char[message_size + 1];
	sConnector.Receive(message, message_size, 0);
	message[message_size] = '\0';

	stringstream ss(message);
	File f;
	while (ss.good()) {
		getline(ss, f.name, ' ');
		getline(ss, f.priority);

		requesting_list.push_back(f);
	}
}

void getListOfFileSize(vector<int>& file_size_list, vector<File> requesting_list, int start) {
	ifstream ifs;
	int file_size = 0;
	for (int i = start; i < requesting_list.size(); ++i) {
		string path = TESTING_FILES_PATH + requesting_list[i].name;
		ifs.open(path.c_str(), ios::binary);

		ifs.seekg(0, ios::end);
		file_size = ifs.tellg();
		ifs.seekg(0, ios::beg);

		ifs.close();

		file_size_list.push_back(file_size);
	}
}

void sendListOfFileSize(vector<int> file_size_list, CSocket& sConnector, int start) {
	int number_of_files = file_size_list.size() - start;
	sConnector.Send(&number_of_files, sizeof(int), 0);

	int file_size = 0;
	for (int i = start; i < file_size_list.size(); ++i) {
		file_size = file_size_list[i];

		sConnector.Send(&file_size, sizeof(int), 0);
	}
}

void getFileHeaders(queue<Header>& file_headers, string fname) {
	string path = TESTING_FILES_PATH + fname;
	ifstream ifs(path.c_str(), ios::binary);
	if (!ifs.good()) {
		cout << "Fail to open " << fname << "!\n";
		return;
	}

	ifs.seekg(0, ios::end);
	int file_size = ifs.tellg();
	ifs.seekg(0, ios::beg);

	int number_of_chunks = file_size / MAX_CHUNK_SIZE;
	int rest_data = file_size % MAX_CHUNK_SIZE;
	Header header;

	header.filename = fname;
	for (int i = 0; i < number_of_chunks; ++i) {
		if (i == 0) header.position = "start";
		else header.position = "middle";
		header.chunk_size = MAX_CHUNK_SIZE;
		file_headers.push(header);
	}

	if (rest_data != 0) {
		if (file_headers.empty()) {
			header.position = "start";
			header.chunk_size = rest_data;
		}
		else {
			header.position = "middle";
			header.chunk_size = rest_data;
		}
		file_headers.push(header);
	}

	header.position = "end";
	header.chunk_size = 0;
	file_headers.push(header);

	ifs.close();
}

void getFileHeadersList(vector<queue<Header>>& file_headers_list, vector<File> requesting_list, vector<File>& downloadable_list) {
	for (int i = 0; i < requesting_list.size(); ++i) 
		for (int j = 0; j <	downloadable_list.size(); ++j) 
			if (requesting_list[i].name == downloadable_list[j].name) {
				queue<Header> file_headers;
				getFileHeaders(file_headers, requesting_list[i].name);
				file_headers_list.push_back(file_headers);

				downloadable_list.erase(downloadable_list.begin() + j);
			}
}

void sendHeader(Header header, CSocket& sConnector) {
	int filename_length = header.filename.size();
	sConnector.Send(&filename_length, sizeof(int), 0);
	sConnector.Send(header.filename.c_str(), filename_length, 0);

	int position_length = header.position.size();
	sConnector.Send(&position_length, sizeof(int), 0);
	sConnector.Send(header.position.c_str(), position_length, 0);
}

void sendChunk(Header header, ifstream& ifs, CSocket& sConnector, bool& connection) {
	char* buffer = new char[header.chunk_size];
	ifs.read(buffer, header.chunk_size);

	int bytes_sent = header.chunk_size;
	sConnector.Send(buffer, bytes_sent, 0);

	int bytes_received = 0;
	sConnector.Receive((char*)&bytes_received, sizeof(int), 0);

	if (checkDisconnection(bytes_received)) {
		delete[] buffer;
		connection = false;
		return;
	}

	while (bytes_received < bytes_sent) {
		bytes_sent -= bytes_received;
		sConnector.Receive((char*)&bytes_received, sizeof(int), 0);

		if (checkDisconnection(bytes_received)) {
			delete[] buffer;
			connection = false;
			return;
		}
	}

	delete[] buffer;
}

void getNumberOfChunks(string priority, int& number_of_chunks, int file_headers_size) {
	if (priority == "NORMAL") number_of_chunks = 1;
	else if (priority == "HIGH") number_of_chunks = 4;
	else number_of_chunks = 10;

	number_of_chunks = file_headers_size < number_of_chunks ? file_headers_size : number_of_chunks;
}

DWORD WINAPI uploadProcess(LPVOID arg) {
	SOCKET* h_connected = (SOCKET*)arg;
	CSocket sConnector;
	sConnector.Attach(*h_connected);

	string filename = "download.txt";
	vector<File> downloadable_list;

	//Send downloadable files to Client
	getDownloadableFiles(downloadable_list, filename);
	sendDownloadableFiles(downloadable_list, sConnector);

	int downloadable_files = downloadable_list.size();
	sConnector.Send(&downloadable_files, sizeof(int), 0);

	int start = 0;
	bool connection = true;
	vector<File> requesting_list;
	vector<queue<Header>> file_headers_list;

	//Receive requesting files from Client
	receiveRequestingFiles(requesting_list, sConnector, start, connection);
	getFileHeadersList(file_headers_list, requesting_list, downloadable_list);

	//Check connection
	if (!connection) {
		delete h_connected;
		return 0;
	}

	vector<int> file_size_list;

	//Send list of file size to Client
	getListOfFileSize(file_size_list, requesting_list, start);
	sendListOfFileSize(file_size_list, sConnector, start);

	int number_of_chunks = 0;
	int chunk_count = 0;
	int difference = 0;
	int downloading_files = requesting_list.size();
	Header header;
	vector<ifstream> ifs_list;

	//Send data to Client
	do {
		for (int i = 0; i < requesting_list.size(); ++i) {
			if (file_headers_list[i].empty()) continue;

			getNumberOfChunks(requesting_list[i].priority, number_of_chunks, file_headers_list[i].size());
			sConnector.Send(&number_of_chunks, sizeof(int), 0);

			while (number_of_chunks != 0) {
				header = file_headers_list[i].front();
				file_headers_list[i].pop();
				sendHeader(header, sConnector);

				//Handle chunk position
				if (header.position == "start") {
					ifstream ifs(header.filename.c_str(), ios::binary);
					ifs_list.push_back(move(ifs));
				}
				else if (header.position == "end") {
					--downloading_files;
					++chunk_count;
					--number_of_chunks;
					--downloadable_files;
					ifs_list[i].close();

					int response = 1;
					//response = 1: uploaded file successfully
					//response = 2: uploaded all files

					if (downloadable_files == 0) {
						downloading_files = -1;
						response = 2;
						sConnector.Send(&response, sizeof(int), 0);
						break;
					}

					sConnector.Send(&response, sizeof(int), 0);

					continue;
				}

				sendChunk(header, ifs_list[i], sConnector, connection);

				//Check connection
				if (!connection) {
					delete h_connected;
					return 0;
				}

				++chunk_count;
				--number_of_chunks;

				//Receive difference after every chunk sent
				sConnector.Receive((char*)&difference, sizeof(int), 0);

				//Update requesting list
				if (difference == 1) {

					//Receive requesting files from Client
					receiveRequestingFiles(requesting_list, sConnector, start, connection);
					getFileHeadersList(file_headers_list, requesting_list, downloadable_list);

					//Check connection
					if (!connection) {
						delete h_connected;
						return 0;
					}

					downloading_files += requesting_list.size() - start;

					//Send list of file size to Client
					getListOfFileSize(file_size_list, requesting_list, start);
					sendListOfFileSize(file_size_list, sConnector, start);
				}
			}
		}

		if (downloading_files == 0) {
			Sleep(10000);

			//Receive difference after timeout
			sConnector.Receive((char*)&difference, sizeof(int), 0);

			if (difference == 1) {

				//Receive requesting files from Client
				receiveRequestingFiles(requesting_list, sConnector, start, connection);
				getFileHeadersList(file_headers_list, requesting_list, downloadable_list);

				//Check connection
				if (!connection) {
					delete h_connected;
					return 0;
				}

				downloading_files += requesting_list.size() - start;

				//Send list of file size to Client
				getListOfFileSize(file_size_list, requesting_list, start);
				sendListOfFileSize(file_size_list, sConnector, start);
			}
			else break;
		}
		else if (downloading_files == -1) break;
	} while (1);

	delete h_connected;
	return 0;
}