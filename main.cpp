#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

// напишите эту функцию
bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    ifstream in(in_file);
    if (!in) {
        return false;
    }
    ofstream out(out_file, ios_base::app);

    regex inc_file("\\s*#\\s*include\\s*\"([^\"]*)\"\\s*"s);
    regex inc_lib("\\s*#\\s*include\\s*<([^>]*)>\\s*"s);
    string line;
    int line_num = 0;
    while (getline(in, line)) {
        ++line_num;
        if (regex_match(line, inc_file)) {
            string_view str_way = line;
            str_way = str_way.substr(str_way.find_first_of('"') + 1, str_way.find_last_of('"') - str_way.find_first_of('"') - 1);
            path file = path(str_way).lexically_normal();
            if (filesystem::exists(in_file.parent_path() / file)) {
                path p = in_file.parent_path() / file;
                if (!Preprocess(p, out_file, include_directories)) {
                    return false;
                }
            }
            else {

                bool end = false;
                for (const auto &dir : include_directories) {
                    if (filesystem::exists(dir / file)) {
                        end = true;
                        if (!Preprocess(dir / file, out_file, include_directories)) {
                            return false;
                        }
                    }
                }
                if (!end) {
                    cout << "unknown include file " << str_way << " at file " << in_file.string()
                         << " at line " << line_num << endl;
                    return false;
                }
            }
        }
        else if (regex_match(line, inc_lib)) {
            string_view str_way = line;
            str_way = str_way.substr(str_way.find('<') + 1, str_way.find('>') - str_way.find('<') - 1);
            path file = path(str_way).lexically_normal();

            bool end = false;
            for (const auto &dir : include_directories) {
                if (filesystem::exists(dir / file)) {
                    end = true;
                    if (!Preprocess(dir / file, out_file, include_directories)) {
                        return false;
                    }
                }
            }
            if (!end) {
                cout << "unknown include file " << str_way << " at file " << in_file.string()
                     << " at line " << line_num << endl;
                return false;
            }
        }
        else {
            out << line << endl;
        }
    }
    return true;
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"sv;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"sv;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"sv;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"sv;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"sv;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"sv;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                        {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"sv;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}
