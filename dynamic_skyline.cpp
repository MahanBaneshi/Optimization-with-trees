#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <stdexcept>

using namespace std;

class DynamicSkyline {
private:
    struct Point {
        vector<double> coordinates;
        int id;
        Point(vector<double> coords, int id) : coordinates(move(coords)), id(id) {}
    };

    struct BJRNode {
        Point point;
        shared_ptr<BJRNode> parent;
        vector<shared_ptr<BJRNode>> children;
        BJRNode(Point p) : point(move(p)), parent(nullptr) {}
    };

    shared_ptr<BJRNode> root;
    int dimensions;
    unordered_map<int, shared_ptr<BJRNode>> active_nodes;
    map<int, vector<shared_ptr<BJRNode>>> injection_schedule;
    map<int, vector<int>> ejection_schedule;
    ofstream output_file;

    bool dominates(const Point &a, const Point &b) const {
        bool strictly_better = false;
        for (int d = 0; d < dimensions; ++d) {
            if (a.coordinates[d] > b.coordinates[d])
                return false;
            if (a.coordinates[d] < b.coordinates[d])
                strictly_better = true;
        }
        return strictly_better;
    }

    void inject_node(shared_ptr<BJRNode> node) {
        inject_helper(root, node);
    }

    void inject_helper(shared_ptr<BJRNode> parent, shared_ptr<BJRNode> new_node) {
        bool dominated = false;

        for (auto it = parent->children.begin(); it != parent->children.end();) {
            if (dominates((*it)->point, new_node->point)) {
                inject_helper(*it, new_node);
                dominated = true;
                break;
            }
            else if (dominates(new_node->point, (*it)->point)) {
                auto dominated_child = *it;
                it = parent->children.erase(it);
                new_node->children.push_back(dominated_child);
                dominated_child->parent = new_node;
            }
            else {
                ++it;
            }
        }

        if (!dominated) {
            new_node->parent = parent;
            parent->children.push_back(new_node);
        }
    }

    void eject_node(int point_id) {
        auto it = active_nodes.find(point_id);
        if (it == active_nodes.end())
            return;

        auto node = it->second;
        auto parent = node->parent;

        parent->children.erase(
            remove(parent->children.begin(), parent->children.end(), node),
            parent->children.end());

        for (auto &child : node->children) {
            child->parent = nullptr;
            inject_node(child);
        }

        active_nodes.erase(it);
    }

public:
    DynamicSkyline(int dim) : dimensions(dim) {
        root = make_shared<BJRNode>(Point(vector<double>{}, -1));
    }

    void load_data(const string &coords_file, const string &times_file) {
        ifstream coord_stream(coords_file);
        if (!coord_stream) {
            throw runtime_error("Cannot open coordinates file: " + coords_file);
        }

        vector<vector<double>> coordinates;
        string line;
        int id = 1;

        while (getline(coord_stream, line)) {
            if (line.empty()) continue;

            vector<double> coords;
            istringstream iss(line);
            string token;

            while (iss >> token) {
                try {
                    coords.push_back(stod(token));
                    if (coords.size() > static_cast<size_t>(dimensions)) {
                        throw runtime_error("Too many coordinates in line");
                    }
                } catch (...) {
                    cerr << "Error parsing coordinates in line " << id << endl;
                    exit(1);
                }
            }

            if (coords.size() != static_cast<size_t>(dimensions)) {
                cerr << "Invalid number of coordinates in line " << id << endl;
                exit(1);
            }

            coordinates.push_back(move(coords));
            id++;
        }

        ifstream time_stream(times_file);
        if (!time_stream) {
            throw runtime_error("Cannot open timestamps file: " + times_file);
        }

        int point_idx = 0;
        int line_num = 1;

        while (getline(time_stream, line)) {
            if (line.empty()) continue;

            istringstream iss(line);
            int ins_t, ej_t;

            if (!(iss >> ins_t >> ej_t)) {
                cerr << "Invalid timestamp format at line " << line_num << endl;
                exit(1);
            }

            if (point_idx >= coordinates.size()) {
                throw runtime_error("More timestamps than coordinates");
            }

            auto point = Point(coordinates[point_idx], point_idx + 1);
            auto node = make_shared<BJRNode>(move(point));

            injection_schedule[ins_t].push_back(node);
            ejection_schedule[ej_t].push_back(point_idx + 1);

            point_idx++;
            line_num++;
        }
    }

    void process_time_steps(const string &output_filename = "small.out") {
        output_file.open(output_filename);
        if (!output_file) {
            throw runtime_error("Cannot open output file: " + output_filename);
        }

        if (injection_schedule.empty() && ejection_schedule.empty()) {
            return;
        }

        int start_time = injection_schedule.begin()->first;
        int end_time = ejection_schedule.rbegin()->first;

        for (int t = start_time; t <= end_time; ++t) {
            if (injection_schedule.count(t)) {
                for (auto &node : injection_schedule[t]) {
                    inject_node(node);
                    active_nodes[node->point.id] = node;
                }
            }

            if (ejection_schedule.count(t)) {
                for (int id : ejection_schedule[t]) {
                    eject_node(id);
                }
            }

            vector<int> skyline_ids;
            for (const auto &child : root->children) {
                skyline_ids.push_back(child->point.id - 1);
            }

            sort(skyline_ids.begin(), skyline_ids.end());

            for (size_t i = 0; i < skyline_ids.size(); ++i) {
                output_file << skyline_ids[i];
                if (i < skyline_ids.size() - 1) {
                    output_file << " ";
                }
            }
            output_file << endl;
        }

        output_file.close();
    }
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <coordinates_file> <timestamps_file>" << endl;
        return 1;
    }

    try {
        string coords_file = argv[1];
        int dimensions = 4;

        if (coords_file.find("medium") != string::npos) {
            dimensions = 5;
        } else if (coords_file.find("large") != string::npos) {
            dimensions = 7;
        }

        DynamicSkyline processor(dimensions);
        processor.load_data(argv[1], argv[2]);
        processor.process_time_steps("small.out");
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}