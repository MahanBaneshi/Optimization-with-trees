#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <limits>

using namespace std;

class DynamicSkyline
{
private:
    struct Point
    {
        vector<double> coordinates;
        int id;
        Point(vector<double> coords, int id) : coordinates(move(coords)), id(id) {}
    };

    struct BJRNode
    {
        Point point;
        shared_ptr<BJRNode> parent;
        vector<shared_ptr<BJRNode>> children;
        BJRNode(Point p) : point(move(p)), parent(nullptr) {}
    };

    shared_ptr<BJRNode> root;
    unordered_map<int, shared_ptr<BJRNode>> active_nodes;

    shared_ptr<BJRNode> lazy_root;
    unordered_map<int, shared_ptr<BJRNode>> lazy_active_nodes;

    int dimensions;
    int lazy_depth = 3;
    map<int, vector<shared_ptr<BJRNode>>> injection_schedule;
    map<int, vector<int>> ejection_schedule;

    bool dominates(const Point &a, const Point &b) const
    {
        bool strictly_better = false;
        for (int d = 0; d < dimensions; ++d)
        {
            if (a.coordinates[d] > b.coordinates[d])
                return false;
            if (a.coordinates[d] < b.coordinates[d])
                strictly_better = true;
        }
        return strictly_better;
    }

    void inject_node(shared_ptr<BJRNode> node)
    {
        inject_helper(root, node);
    }

    void inject_helper(shared_ptr<BJRNode> parent, shared_ptr<BJRNode> new_node)
    {
        bool dominated = false;

        for (auto it = parent->children.begin(); it != parent->children.end();)
        {
            if (dominates((*it)->point, new_node->point))
            {
                inject_helper(*it, new_node);
                dominated = true;
                break;
            }
            else if (dominates(new_node->point, (*it)->point))
            {
                auto dominated_child = *it;
                it = parent->children.erase(it);
                new_node->children.push_back(dominated_child);
                dominated_child->parent = new_node;
            }
            else
            {
                ++it;
            }
        }

        if (!dominated)
        {
            new_node->parent = parent;
            parent->children.push_back(new_node);
        }
    }

    void lazy_inject_node(shared_ptr<BJRNode> node)
    {
        lazy_inject_helper(lazy_root, node, 0);
    }

    void lazy_inject_helper(shared_ptr<BJRNode> parent, shared_ptr<BJRNode> new_node, int current_depth)
    {
        if (current_depth < lazy_depth)
        {
            shared_ptr<BJRNode> best_child = nullptr;
            size_t min_descendants = numeric_limits<size_t>::max();

            for (const auto &child : parent->children)
            {
                if (dominates(child->point, new_node->point))
                {
                    size_t descendants = count_descendants(child);
                    if (descendants < min_descendants)
                    {
                        min_descendants = descendants;
                        best_child = child;
                    }
                }
            }

            if (best_child)
            {
                lazy_inject_helper(best_child, new_node, current_depth + 1);
                return;
            }
        }

        new_node->parent = parent;
        parent->children.push_back(new_node);

        for (auto it = parent->children.begin(); it != parent->children.end();)
        {
            if (*it != new_node && dominates(new_node->point, (*it)->point))
            {
                auto dominated_child = *it;
                it = parent->children.erase(it);
                new_node->children.push_back(dominated_child);
                dominated_child->parent = new_node;
            }
            else
            {
                ++it;
            }
        }
    }

    size_t count_descendants(shared_ptr<BJRNode> node) const
    {
        if (!node)
            return 0;
        size_t count = node->children.size();
        for (const auto &child : node->children)
        {
            count += count_descendants(child);
        }
        return count;
    }

    void eject_node(int point_id, bool lazy_mode)
    {
        auto &nodes = lazy_mode ? lazy_active_nodes : active_nodes;
        auto &current_root = lazy_mode ? lazy_root : root;

        auto it = nodes.find(point_id);
        if (it == nodes.end())
            return;

        auto node = it->second;
        auto parent = node->parent;

        parent->children.erase(
            remove(parent->children.begin(), parent->children.end(), node),
            parent->children.end());

        for (auto &child : node->children)
        {
            child->parent = nullptr;
            if (lazy_mode)
                lazy_inject_node(child);
            else
                inject_node(child);
        }

        nodes.erase(it);
    }

    vector<int> get_skyline_ids(shared_ptr<BJRNode> current_root) const
    {
        vector<int> skyline_ids;
        for (const auto &child : current_root->children)
        {
            skyline_ids.push_back(child->point.id - 1); 
        }
        sort(skyline_ids.begin(), skyline_ids.end());
        return skyline_ids;
    }

    void write_output(const vector<int> &ids, ofstream &outfile)
    {
        for (size_t i = 0; i < ids.size(); ++i)
        {
            outfile << ids[i];
            if (i < ids.size() - 1)
                outfile << " ";
        }
        outfile << endl;
    }

public:
    DynamicSkyline(int dim) : dimensions(dim)
    {
        root = make_shared<BJRNode>(Point(vector<double>{}, -1));
        lazy_root = make_shared<BJRNode>(Point(vector<double>{}, -1));
    }

    void load_data(const string &coords_file, const string &times_file)
    {
        ifstream coord_stream(coords_file);
        if (!coord_stream)
        {
            throw runtime_error("Cannot open coordinates file: " + coords_file);
        }

        vector<vector<double>> coordinates;
        string line;
        int id = 1;

        while (getline(coord_stream, line))
        {
            if (line.empty())
                continue;

            vector<double> coords;
            istringstream iss(line);
            string token;

            while (iss >> token)
            {
                try
                {
                    coords.push_back(stod(token));
                    if (coords.size() > static_cast<size_t>(dimensions))
                    {
                        throw runtime_error("Too many coordinates in line");
                    }
                }
                catch (...)
                {
                    cerr << "Error parsing coordinates in line " << id << endl;
                    exit(1);
                }
            }

            if (coords.size() != static_cast<size_t>(dimensions))
            {
                cerr << "Invalid number of coordinates in line " << id << endl;
                exit(1);
            }

            coordinates.push_back(move(coords));
            id++;
        }

        ifstream time_stream(times_file);
        if (!time_stream)
        {
            throw runtime_error("Cannot open timestamps file: " + times_file);
        }

        int point_idx = 0;
        int line_num = 1;

        while (getline(time_stream, line))
        {
            if (line.empty())
                continue;

            istringstream iss(line);
            int ins_t, ej_t;

            if (!(iss >> ins_t >> ej_t))
            {
                cerr << "Invalid timestamp format at line " << line_num << endl;
                exit(1);
            }

            if (point_idx >= coordinates.size())
            {
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

    void process_time_steps()
    {
        ofstream standard_out("small.out");
        ofstream lazy_out("smalllazy.out");

        if (!standard_out.is_open() || !lazy_out.is_open())
        {
            throw runtime_error("Failed to open output files");
        }

        if (injection_schedule.empty() && ejection_schedule.empty())
        {
            return;
        }

        int start_time = injection_schedule.begin()->first;
        int end_time = ejection_schedule.rbegin()->first;

        for (int t = start_time; t <= end_time; ++t)
        {
            if (injection_schedule.count(t))
            {
                for (auto &node : injection_schedule[t])
                {
                    auto standard_node = make_shared<BJRNode>(node->point);
                    inject_node(standard_node);
                    active_nodes[standard_node->point.id] = standard_node;

                    auto lazy_node = make_shared<BJRNode>(node->point);
                    lazy_inject_node(lazy_node);
                    lazy_active_nodes[lazy_node->point.id] = lazy_node;
                }
            }

            if (ejection_schedule.count(t))
            {
                for (int id : ejection_schedule[t])
                {
                    eject_node(id, false);
                    eject_node(id, true);
                }
            }
            write_output(get_skyline_ids(root), standard_out);
            write_output(get_skyline_ids(lazy_root), lazy_out);
        }
        standard_out.close();
        lazy_out.close();
    }
};

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        cerr << "Usage: " << argv[0] << " <coordinates_file> <timestamps_file>" << endl;
        return 1;
    }

    try
    {
        string coords_file = argv[1];
        int dimensions = 4; 

        if (coords_file.find("medium") != string::npos)
        {
            dimensions = 5;
        }
        else if (coords_file.find("large") != string::npos)
        {
            dimensions = 7;
        }

        DynamicSkyline processor(dimensions);
        processor.load_data(argv[1], argv[2]);
        processor.process_time_steps();

        cout << "Standard skyline results written to small.out" << endl;
        cout << "Lazy evaluation skyline results written to smalllazy.out" << endl;
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}