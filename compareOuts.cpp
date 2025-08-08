#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    if (start == string::npos || end == string::npos)
        return "";
    return str.substr(start, end - start + 1);
}

bool compare_files(const string& file1, const string& file2) {
    ifstream f1(file1), f2(file2);
    if (!f1 || !f2) {
        cerr << "ERROR! Cannot open one of the files: " << file1 << " or " << file2 << endl;
        return false;
    }

    string line1, line2;
    int line_number = 1;
    while (getline(f1, line1) && getline(f2, line2)) {
        if (trim(line1) != trim(line2)) {
            cerr << "ERROR! Mismatch at line " << line_number << ":\n";
            cerr << "   " << file1 << ": " << line1 << '\n';
            cerr << "   " << file2 << ": " << line2 << '\n';
            return false;
        }
        ++line_number;
    }

    // Check for extra lines
    while (getline(f1, line1)) {
        if (!trim(line1).empty()) {
            cerr << "ERROR! Extra non-empty line in " << file1 << " at line " << line_number << endl;
            return false;
        }
        ++line_number;
    }
    while (getline(f2, line2)) {
        if (!trim(line2).empty()) {
            cerr << "ERROR! Extra non-empty line in " << file2 << " at line " << line_number << endl;
            return false;
        }
        ++line_number;
    }

    return true;
}

int main() {
    string ref_file = "small.refout";
    string out_standard = "small.out";
    string out_lazy = "smalllazy.out";

    cout << " Comparing outputs with reference file: " << ref_file;

    bool std_ok = compare_files(out_standard, ref_file);
    bool lazy_ok = compare_files(out_lazy, ref_file);

    cout << "\n=== Comparison Summary ===" << endl;
    if (std_ok)
        cout << "Successful " << out_standard << " matches " << ref_file << endl;
    else
        cout << "ERROR! " << out_standard << " does NOT match " << ref_file << endl;

    if (lazy_ok)
        cout << "Successful " << out_lazy << " matches " << ref_file << endl;
    else
        cout << "ERROR! " << out_lazy << " does NOT match " << ref_file << endl;

    return 0;
}
