#include "Function.h"

void displayDownloadProgress(float progress, float total, string filename) {
	float percentage = progress / total;

	cout << "\rDownloading: " << "\"" << filename << "\"" << ": [";

	if (percentage >= 1) {
		for (int i = 0; i < 10; i++) {
			cout << "\xDB\xDB";
		}
		cout << "] 100%";
		return;
	}

	int progress_bar = percentage * 10;
	for (int i = 0; i < progress_bar; i++) {
		cout << "\xDB\xDB";
	}
	for (int i = progress_bar; i < 10; i++) {
		cout << "  ";
	}
	cout << "] " << percentage * 100 << setprecision(3) << "%          ";
}

void registerAvailableFile(CSocket& sClient) {
	int message_size = 0;
	sClient.Receive((char*)(&message_size), sizeof(int), 0);
	
	char* message;
	message = new char[message_size + 1];
	sClient.Receive(message, message_size, 0);
	message[message_size] = '\0';

	cout << "           <<< Available files >>>\n";
	cout << message << "\n";
}

void registerRequestingFiles(queue<string>& requesting_files) {
	string input = "input.txt";
	ifstream file(input);
	if (!file.good()) {
		cout << "           <<< Unable to open Input File! >>>\n";
		return;
	}

	string filename;
	queue<string> temp = requesting_files;
	while (getline(file, filename)) {
		requesting_files.push(filename);
	}

	file.close();
}

bool receiveDownloadData(string filename, CSocket& sClient) {
	string path = OUTPUT_PATH + filename;
	ofstream ofs(path.c_str(), ios::binary);

	int file_size = 0;
	sClient.Receive((char*)&file_size, sizeof(int), 0);
	int total = file_size;
	int downloading_progress = 0;

	int chunk_size = MAX_CHUNK_SIZE;
	int bytes_received = 0;
	int buffer_size = 0;
	char* buffer = new char[chunk_size];
	while (file_size >= chunk_size) {
		chunk_size = MAX_CHUNK_SIZE;

		bytes_received = sClient.Receive(buffer, chunk_size, 0);
		sClient.Send(&bytes_received, sizeof(int), 0);
		ofs.write(buffer, bytes_received);

		//Receive the remaining data of the data chunk
		while (bytes_received < chunk_size) {
			chunk_size -= bytes_received;

			bytes_received = sClient.Receive(buffer, chunk_size, 0);
			sClient.Send(&bytes_received, sizeof(int), 0);
			ofs.write(buffer, bytes_received);
		}

		file_size -= MAX_CHUNK_SIZE;
		downloading_progress += MAX_CHUNK_SIZE;
		displayDownloadProgress(downloading_progress, total, filename);
	}
	delete[] buffer;

	bytes_received = 0;
	char* remaining_data;
	while (file_size > 0) {
		remaining_data = new char[file_size];

		bytes_received = sClient.Receive(remaining_data, file_size, 0);
		sClient.Send(&bytes_received, sizeof(int), 0);
		ofs.write(remaining_data, bytes_received);

		file_size -= bytes_received;

		delete[] remaining_data;
	}

	displayDownloadProgress(1, 1, filename);
	ofs.close();
	return true;
}

bool processRequestingFile(queue<string> requesting_files, CSocket& sClient) {
	if (requesting_files.empty()) return false;

	string message = requesting_files.front();
	int message_size = message.size();

	sClient.Send(&message_size, sizeof(int), 0);
	sClient.Send(message.c_str(), message_size, 0);

	return true;
}