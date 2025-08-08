#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <map>
#include <memory>
using namespace std;

struct Point {
    vector<double> coords;
    int id;
    Point(vector<double> c, int i) : coords(move(c)), id(i) {}
};

struct Node {
    Point point;
    Node* parent = nullptr;
    vector<Node*> children;
    Node(Point p) : point(move(p)) {}
};

bool dominates(const Point& a, const Point& b) {
    bool strictly_better = false;
    for (size_t i = 0; i < a.coords.size(); ++i) {
        if (a.coords[i] > b.coords[i]) return false;
        if (a.coords[i] < b.coords[i]) strictly_better = true;
    }
    return strictly_better;
}

string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

bool compare_files(const string& file1, const string& file2) {
    ifstream f1(file1), f2(file2);
    if (!f1 || !f2) return false;
    string line1, line2;
    while (getline(f1, line1) && getline(f2, line2)) {
        if (trim(line1) != trim(line2)) return false;
    }
    return true;
}

void inject_node(Node* root, Node* new_node) {
    bool dominated = false;
    for (auto it = root->children.begin(); it != root->children.end();) {
        if (dominates((*it)->point, new_node->point)) {
            inject_node(*it, new_node);
            dominated = true;
            return;
        } else if (dominates(new_node->point, (*it)->point)) {
            Node* child = *it;
            it = root->children.erase(it);
            new_node->children.push_back(child);
            child->parent = new_node;
        } else {
            ++it;
        }
    }
    if (!dominated) {
        new_node->parent = root;
        root->children.push_back(new_node);
    }
}

void eject_node(Node* root, map<int, Node*>& active_nodes, int id) {
    auto it = active_nodes.find(id);
    if (it == active_nodes.end()) return;
    Node* node = it->second;
    Node* parent = node->parent;
    if (parent) {
        parent->children.erase(remove(parent->children.begin(), parent->children.end(), node), parent->children.end());
    }
    for (Node* child : node->children) {
        child->parent = nullptr;
        inject_node(root, child);
    }
    active_nodes.erase(it);
    delete node;
}

void collect_skyline_ids(Node* root, vector<int>& result) {
    for (Node* child : root->children) {
        result.push_back(child->point.id - 1);
    }
    sort(result.begin(), result.end());
}

int main() {
    ifstream setup("small.setup");
    if (!setup) throw runtime_error("Cannot open setup file");

    int n, d, T, maxout; string prefix;
    setup >> n >> d >> T >> maxout >> prefix;

    ifstream input(prefix + ".input");
    if (!input) throw runtime_error("Cannot open input file");

    vector<Point> points;
    string line; int id = 1;
    while (getline(input, line)) {
        istringstream iss(line);
        vector<double> coords(d);
        for (int i = 0; i < d; ++i) iss >> coords[i];
        points.emplace_back(coords, id++);
    }

    ifstream times(prefix + ".times");
    if (!times) throw runtime_error("Cannot open times file");

    map<int, vector<int>> insert_at, eject_at;
    for (int i = 0; i < n; ++i) {
        int ins, ej;
        times >> ins >> ej;
        insert_at[ins].push_back(i);
        eject_at[ej].push_back(i);
    }

    Node* root = new Node(Point({}, -1));
    map<int, Node*> active;

    ofstream out(prefix + ".ndcache.out");
    if (!out) throw runtime_error("Cannot open output file");

    for (int t = 0; t <= T; ++t) {
        if (insert_at.count(t)) {
            for (int idx : insert_at[t]) {
                Node* node = new Node(points[idx]);
                inject_node(root, node);
                active[points[idx].id] = node;
            }
        }
        if (eject_at.count(t)) {
            for (int idx : eject_at[t]) {
                eject_node(root, active, points[idx].id);
            }
        }
        vector<int> ids;
        collect_skyline_ids(root, ids);
        for (size_t i = 0; i < ids.size(); ++i) {
            out << ids[i];
            if (i < ids.size() - 1) out << " ";
        }
        out << "\n";
    }
    out.close();

    bool same = compare_files(prefix + ".ndcache.out", prefix + ".refout");
    cout << (same ? " Output matches reference." : " Output does NOT match reference!") << endl;

    return 0;
}
