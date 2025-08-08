#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

using namespace std;

string triming(const string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    size_t end = str.find_last_not_of(" \t\r\n");
    if (start == string::npos || end == string::npos)
        return "";
    return str.substr(start, end - start + 1);
}

bool compare_files(const string& file1, const string& file2) {
    ifstream f1(file1), f2(file2);
    if (!f1 || !f2) {
        cerr << "ERROR! We can't open one of the files: " << file1 << " or " << file2 << endl;
        return false;
    }

    string line1, line2;
    int line_number = 1;
    while (getline(f1, line1) && getline(f2, line2)) {
        if (triming(line1) != triming(line2)) {
            cerr << "ERROR! There is a difference between files at line " << line_number << ":\n";
            cerr << "   " << file1 << ": " << line1 << '\n';
            cerr << "   " << file2 << ": " << line2 << '\n';
            return false;
        }
        ++line_number;
    }

    // Checking for extra lines in one of the files
    while (getline(f1, line1)) {
        if (!triming(line1).empty()) {
            cerr << "ERROR! There is an Extra non-empty line in " << file1 << " at line " << line_number << endl;
            return false;
        }
        ++line_number;
    }
    while (getline(f2, line2)) {
        if (!triming(line2).empty()) {
            cerr << "ERROR! There is an Extra non-empty line in " << file2 << " at line " << line_number << endl;
            return false;
        }
        ++line_number;
    }

    return true;
}

int main() {
    string ref_file = "small.refout";
    string out_standard = "small.out";
    string out_with_lazy = "smalllazy.out";

    cout << " Comparing outputs with reference file: " << ref_file;

    bool std_ok = compare_files(out_standard, ref_file);
    bool lazy_ok = compare_files(out_with_lazy, ref_file);

    cout << "\n=== Comparison Summary ===" << endl;
    if (std_ok)
        cout << "Successful " << out_standard << " matches " << ref_file << " Absolutely" << endl;
    else
        cout << "ERROR! " << out_standard << " does NOT match " << ref_file << " Absolutely" << endl;

    if (lazy_ok)
        cout << "Successful " << out_with_lazy << " matches " << ref_file << " Absolutely" << endl;
    else
        cout << "ERROR! " << out_with_lazy << " does NOT match " << ref_file << " Absolutely" << endl;

    return 0;
}
