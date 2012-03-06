#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main(int argc, char** argv) {
	if(argc < 5) {
		cerr << "USAGE: simple_templater <template file> <token> <replacement> <output file>" << endl;
		return 1;
	}

	size_t token_len = strlen(argv[2]);

	ifstream ifs(argv[1],fstream::in);
	ofstream ofs(argv[4],fstream::out);
	char line[1024];
	while(ifs.good()) {
		ifs.getline(line,1024);
		string line_s(line);
		size_t found = line_s.find(argv[2]);
		if(found != string::npos) {
			cout << "found token in line." << endl;
			line_s = line_s.replace(found,token_len,argv[3]);
		}
		ofs << line_s << endl;
	}
	ifs.close();
	ofs.close();

	return 0;
}