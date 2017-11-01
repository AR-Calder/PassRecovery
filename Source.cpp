#include <boost/iostreams/device/mapped_file.hpp> // for mmap
#include <string>
#include <chrono>
#include "tasker.h"
#include <openssl/md5.h>
#include <atomic>

using std::string;
using namespace std;

int global_threads = 1;

std::mutex mx;
std::atomic_bool task_complete = false;

//Ignore warning that sprintf is outdated
#pragma warning(disable: 4996)

class md5_hash {
public:
	std::string hash(std::string & password) {
		MD5_Init(&context);
		cstring = password.c_str();
		MD5_Update(&context, cstring, strlen(cstring));
		MD5_Final(digest, &context);

		for (int i = 0; i < 16; i++)
			sprintf(&md5String[i * 2], "%02x", (unsigned int)digest[i]);

		return md5String;
	}

private:
	unsigned char digest[16];
	const char * cstring;
	char md5String[33];
	MD5_CTX context;	
};

vector<unsigned char>get_types() {
	std::vector<unsigned char> types = { 97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122 };	 //a-z
	std::vector<std::string> cout_types = { "Uppercase Letters", "Numbers", "Special Characters", "Extended ASCII (128 - 255)" };
	std::vector<std::vector<unsigned char>> types_v = {
		{ 65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90 },							 //A-Z
		{ 48,49,50,51,52,53,54,55,56,57 },																			 //0-9
		{ 32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,58,59,60,61,62,63,64,91,92,93,94,95,96,123,124,125,126,127 },//Special characters
		{ 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255 } //extended ascii
	};
	char temp = 0;
	std::cout << "Add aditional types to bruteforce, default is just lowercase.\n";
	for (auto i = 0; i < 4; ++i) {
		cout << cout_types[i] << " y/n ?\n";
		//if the user does not enter y or n loop
		while (temp != 'Y' && temp != 'N') {
			cin >> temp;
			temp = toupper(temp);
		}
		if (temp == 'Y') {
			//if yes add this type
			types.insert(types.end(), types_v[i].begin(), types_v[i].end());
		}
		temp = 0;
	}
	return types;
}


std::string & make_string(std::string & s, std::vector<int> & itrs, std::vector<unsigned char> & charset)
{
	char * out = const_cast<char *>(s.data());
	for (int i : itrs)
		*out++ = charset[i];
	return s;
}

bool cmp_hash(std::string & password, std::string & hash) {
	md5_hash md5;
	if (md5.hash(password) == hash) {
		std::cout << "\npassword: " << password << "\n\npassword hash: \n" << md5.hash(password) << "\n\nMatches entered hash: \n" << hash << "\n\n";
		task_complete = true;
		return true;
	}
	return false;
}

void brute(int min, volatile unsigned int top, std::vector<int> curr, std::vector<int> last, std::vector<unsigned char> & alphabet, std::string & hash) {
	std::string s(curr.size(), 'a');

	//add all up to the last
	while (curr != last && !task_complete) {

		if (cmp_hash(make_string(s, curr, alphabet), hash))
			return;

		//add 1 to low digit
		curr[0]++;

		//if low digit has looped past max
		if (curr[0] == top + 1) {
			//iterate through all digits
			for (unsigned int i = 0; i < curr.size() && (curr[i] == top + 1); ++i) {
				//set it to min and increase value of next digit up by 1
				curr[i] = min;
				if (i != curr.size() - 1) {
					++curr[i + 1];
				}
			}
		}
	} 

	//and add the last 
	if (cmp_hash(make_string(s,last, alphabet), hash))
		return;

}

void split_bruteforce(int size, std::vector<unsigned char> & alphabet, std::string & hash) {
	
	//divide like 0 - 6, 7 - 12, 13 - 18, 18 - 26 etc... using method from dict attack to assign equal 

	cerr << "Bruteforcing\n";
	Tasker brute_task(global_threads);
	auto alpha_size = alphabet.size() - 1;

	for (auto sizeit = size; sizeit > 0; --sizeit) {
		{
			std::vector<int> start,
							 end;

			for (int f = 0; f < sizeit; ++f) {
				start.emplace_back(0);
				end.emplace_back(alphabet.size() - 1);
			}
			end[sizeit - 1] = start[sizeit - 1];

			//create tasks for each character
			for (int i = 0; i < alphabet.size(); ++i) {
				brute_task.add_task([sizeit, start, alpha_size, end, &alphabet, &hash]() {brute(start[sizeit - 1], alpha_size, start, end, std::ref(alphabet), std::ref(hash)); });
				start[sizeit - 1]++;
				end[sizeit - 1]++;
			}
		}
	}

	brute_task.wait();
}

#include <fstream>

int main()
{
	//EXAMPLE 
	string filename = "rockyou.txt";
	string hash = "95ebc3c7b3b9f1d2c40fec14415d3cb8";
	int bf_chars = 5;

	auto alphabet = get_types();

	global_threads = k;

	auto start = std::chrono::high_resolution_clock::now();
	
	//not included in this version
	//split_dictionary(get_passwords(filename), hash);

	if (!task_complete) {
		//cout << "\nNo match found in dictionary.\n";
		//std::cout << "\nTesting up to " << bf_chars << " character long passwords\n";
		split_bruteforce(bf_chars, alphabet, hash);
	}
	
	auto finish = std::chrono::high_resolution_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(finish - start).count() << "\n";
	task_complete = false;		
	
	return 0; 
}
