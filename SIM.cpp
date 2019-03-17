/* On my honor, I have neither given nor received unauthorized aid on this assignment */
#include <algorithm>
#include <deque>
#include <functional>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <numeric>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define ORIGINAL_BINARY "000"
#define REAL_LENGTH_ENCODING "001"
#define BITMASK "010"
#define MISMATCH_1b "011"
#define MISMATCH_2b_C "100"
#define MISMATCH_4b_C "101"
#define MISMATCH_2b_A "110"
#define DIRECT "111"

using namespace std;

class Compression {
public:
    // Constructor
    Compression(const string &out, const bool compress = true) {
        if (compress) {
            formatDictionary("original.txt");
            init_compression();
        } else {
            getDictionary("compressed.txt");
            init_decompression();
        }
    }
    // Build a string buffer with all the compressed code
    string formatCompressedCode() {
        string str{};
        std::stringstream ss;
        int charCount = 0, itr = 0;

        // Load compressed code to string stream buffer
        for (const auto& ins : compressed_data)
            str.append(ins);

        while (itr != str.length()) {
            if (str[itr] == '-') {
                itr++;
                continue;
            }
            ss << str[itr++];
            charCount++;
            if (charCount == 32) {
                ss << endl;
                charCount = 0;
            }
        }

        while (charCount != 32) {
            ss << 0; charCount++;
        }

        // Add separator of data
        ss << endl << "xxxx" << endl;

        // Load all dictionary entries to the string stream
        for (const auto& entry : dict)
            ss << entry.first << endl;

        return ss.str();
    }
    // Build a string buffer with all the decompressed code
    string formatDecompressedCode() {
        std::stringstream ss;

        // Load decompressed code to the string stream buffer
        int charCount = 0;
        int itr = 0;

        while (itr != decompressed_data.length()) {
            ss << decompressed_data[itr++];
            charCount++;
            if (charCount == 32) {
                ss << endl;
                charCount = 0;
            }
        }
        return ss.str();
    }
    // Create a file or overwrite to write data of string stream buffer
    void writeOutputFile(const string &out, bool isCompressed) {
        std::ofstream ofs(out);

        if (isCompressed) {
            ofs << formatCompressedCode();
            ofs.close();
        } else {
            ofs << formatDecompressedCode();
            ofs.close();
        }
    }

private:
    enum _Format_ { Original, Run_Length, Bitmask_Based,
        Mismatch_1b, Mismatch_2b_C, Mismatch_4b_C, Mismatch_2b_A,
        Direct_Match } comp_types;
    vector<string> compressed_data;
    vector<string> _data_;
    string decompressed_data;
    vector<pair<string, int>> dict;
    string serialized_data;
    map<int, string, greater<int>> sorted_dict;

    // Numeric calculations
    int compareStrings(const string &str1, const string &str2) const {
        return inner_product(str1.begin(), str1.end(), str2.begin(), 0, std::plus<unsigned int>(),
                             std::not2(std::equal_to<std::string::value_type>())
        );
    }
    int getWidth(const string &str1, const string &str2, pair<int, int> &limits, const int width = 32) {
        int left{}, right{};
        auto str1_itr = str1.cbegin();
        auto str2_itr = str2.cbegin();

        while (str1_itr != str1.cend()) {
            if (*str1_itr == *str2_itr)
                left++;
            else
                break;
            str1_itr++;
            str2_itr++;
        }
        auto str1_right_itr = str1.crbegin();
        auto str2_right_itr = str2.crbegin();

        while (str1_right_itr != str1.crend()) {
            if (*str1_right_itr == *str2_right_itr)
                right++;
            else
                break;
            str1_right_itr++; str2_right_itr++;
        }
        limits.first = left;
        limits.second = (32 - right) - 1;
        return abs(limits.first - limits.second);
    }

    // String manipulations
    string compressCode(string ins_code, int _format, int &line, int &tracker) {
        auto format = (_Format_)_format;
        switch (format) {

            case _Format_::Original: {
                tracker = 1;
                return (ORIGINAL_BINARY + ins_code);
            }

            case _Format_::Run_Length: {
                if (line == _data_.size() - 1)
                    return string();
                if (_data_[line] != _data_[line + 1])
                    return string();

                int counts{};
                int lineCount = line;

                while ((lineCount != _data_.size() - 1) && (_data_[lineCount] == _data_[lineCount + 1])) {
                    counts++;
                    lineCount++;
                    if (counts == 8)
                        break;
                } counts++;

                // Calculate optimal compression
                string temp_ins_c{};
                int rleCount;
                map<int, string, less<int>> ins_list_c;

                for (const auto& method : { 0,2,3,4,5,6,7 }) {
                    int temp_line = line;
                    int ins_size{};
                    temp_ins_c = compressCode(ins_code, method, temp_line, rleCount);
                    ins_size = static_cast<int>(temp_ins_c.empty() ? numeric_limits<int>::max() : temp_ins_c.length());
                    auto comp_entry = make_pair(ins_size, temp_ins_c);
                    ins_list_c.insert(comp_entry);
                }

                // Choose entry that came first that has equal popularity
                auto equal_range_c = ins_list_c.equal_range(ins_list_c.begin()->first);

                if (distance(equal_range_c.first, equal_range_c.second) == 1) {
                    // For one
                    temp_ins_c = ins_list_c.begin()->second;
                } else {
                    // For many
                    string code{};
                    code += equal_range_c.first->second[0];
                    code += equal_range_c.first->second[1];
                    code += equal_range_c.first->second[2];
                    int methodCode = stoi(code);

                    for (const auto& ins_c : ins_list_c) {
                        string str{};
                        str += ins_c.second[0];
                        str += ins_c.second[1];
                        str += ins_c.second[2];

                        if (methodCode > stoi(str))
                            methodCode = stoi(str);
                    }

                    for (const auto& ins_c : ins_list_c) {
                        string str{};
                        str += ins_c.second[0];
                        str += ins_c.second[1];
                        str += ins_c.second[2];

                        if (methodCode == stoi(str)) {
                            temp_ins_c = ins_c.second;
                            break;
                        }
                    }
                }
                tracker += (counts);
                return (temp_ins_c + "-" + REAL_LENGTH_ENCODING + padLeftSide(toBinary(
                        static_cast<unsigned int>(counts - 2)), 3));
            }

            case _Format_::Bitmask_Based: {
                string chunk_mask = "0000";
                string str_index, dict_index;
                bool flag = false;
                int ch_compResult{}, w_diff_result{};

                for (auto ins : dict) {
                    if (ins.first == ins_code)
                        return string();

                    pair<int, int> range;
                    ch_compResult = compareStrings(ins_code, ins.first);
                    w_diff_result = getWidth(ins_code, ins.first, range);

                    if (((ch_compResult == 2) && (w_diff_result > 1) &&(w_diff_result < 4)) ||
                        ((ch_compResult==3)) &&  (w_diff_result ==2)|| w_diff_result == 3) {
                        chunk_mask[0] = (ins_code[range.first] == ins.first[range.first]) ? '0' : '1';
                        chunk_mask[1] = (ins_code[range.first+1] == ins.first[range.first+1]) ? '0' : '1';
                        chunk_mask[2] = (ins_code[range.first+2] == ins.first[range.first+2]) ? '0' : '1';
                        chunk_mask[3] = (ins_code[range.first+3] == ins.first[range.first+3]) ? '0' : '1';
                        str_index = padLeftSide(toBinary(static_cast<unsigned int>(range.first)), 5);
                        dict_index = padLeftSide(toBinary(static_cast<unsigned int>(ins.second)), 4);
                        flag = true;
                        break;
                    }

                }
                if (!flag)
                    return string();

                tracker = 1;
                return (BITMASK + str_index + padLeftSide(chunk_mask, 4) + dict_index);
            }

            case _Format_::Mismatch_1b: {
                pair<int, int> str_index;
                int dict_index{};
                bool flag = false;

                for (const auto &ins : dict) {
                    if (ins.first == ins_code)
                        return string();
                    if (compareStrings(ins_code, ins.first) == 1) {
                        getWidth(ins_code, ins.first, str_index);
                        dict_index = ins.second;
                        flag = true;
                        break;
                    }
                }
                if (!flag)
                    return string();

                tracker = 1;
                return (MISMATCH_1b +
                    padLeftSide(toBinary(static_cast<unsigned int>(str_index.first)), 5) +
                    padLeftSide(toBinary(static_cast<unsigned int>(dict_index)), 4));
            }

            case _Format_::Mismatch_2b_C: {
                pair<int, int> str_index;
                int dict_index{};
                bool flag = false;

                for (const auto &ins : dict) {
                    if (ins.first == ins_code)
                        return string();
                    if (compareStrings(ins_code, ins.first) == 2) {
                        if(getWidth(ins_code, ins.first, str_index) != 1)
                            break;
                        dict_index = ins.second;
                        flag = true;
                        break;
                    }
                }
                if (!flag)
                    return string();

                tracker = 1;
                return (MISMATCH_2b_C +
                    padLeftSide(toBinary(static_cast<unsigned int>(str_index.first)), 5) +
                    padLeftSide(toBinary(static_cast<unsigned int>(dict_index)), 4));
            }

            case _Format_::Mismatch_4b_C: {
                pair<int, int> str_index;
                int dict_index{};
                bool flag = false;

                for (const auto &ins : dict) {
                    if (ins.first == ins_code)
                        return string();
                    if (compareStrings(ins_code, ins.first) == 4) {
                        if(getWidth(ins_code, ins.first, str_index) != 3)
                            break;
                        dict_index = ins.second;
                        flag = true;
                        break;
                    }
                }
                if (!flag)
                    return string();

                tracker = 1;
                return (MISMATCH_4b_C +
                        padLeftSide(toBinary(static_cast<unsigned int>(str_index.first)), 5) +
                        padLeftSide(toBinary(static_cast<unsigned int>(dict_index)), 4));
            }

            case _Format_::Mismatch_2b_A: {
                pair<int, int> str_index;
                int dict_index{};
                bool flag = false;

                for (const auto &ins : dict) {
                    if (ins.first == ins_code)
                        return string();
                    if (compareStrings(ins_code, ins.first) == 2) {
                        getWidth(ins_code, ins.first, str_index);
                        dict_index = ins.second;
                        flag = true;
                        break;
                    }
                }
                if (!flag)
                    return string();

                tracker = 1;
                return (MISMATCH_2b_A +
                        padLeftSide(toBinary(static_cast<unsigned int>(str_index.first)), 5) +
                        padLeftSide(toBinary(static_cast<unsigned int>(str_index.second)), 5) +
                        padLeftSide(toBinary(static_cast<unsigned int>(dict_index)), 4));
            }

            case _Format_::Direct_Match: {
                tracker = 1;

                for (const auto &ins : dict)
                    if (ins.first == ins_code)
                        return (DIRECT + padLeftSide(toBinary(static_cast<unsigned int>(ins.second)), 4));

                return string();
            }
        }
    }
    string getInstruction(const string &ins, int &line) {
        multimap<int, pair<string, int>, less<int>> ins_list_c;
        string temp_ins_c{};
        int size_i{};
        int tracker{};

        // Make all compressed bits possible
        for(const auto& method: {0,1,2,3,4,5,6,7}) {
            temp_ins_c = compressCode(ins, method, line, tracker);
            size_i = static_cast<int>(temp_ins_c.empty() ? numeric_limits<int>::max() : temp_ins_c.length());
            auto ins_entry_c = make_pair(size_i, make_pair(temp_ins_c, tracker));
            ins_list_c.insert(ins_entry_c);
            tracker = 0;
        }

        // Choose priority format from results
        for (const auto& format_result : ins_list_c) {
            if (format_result.second.first.find('-') != string::npos) {
                line += format_result.second.second;
                return format_result.second.first;
            }
        }
        auto range = ins_list_c.equal_range(ins_list_c.begin()->first);
        if ((distance(range.first,range.second)==1)) {
            // For one
            line += ins_list_c.begin()->second.second;
            return ins_list_c.begin()->second.first;
        } else {
            // For two
            string str{};
            str += range.first->second.first[0];
            str += range.first->second.first[1];
            str += range.first->second.first[2];
            int priority = stoi(str);

            for (const auto& _ins_ : ins_list_c) {
                string _str{};
                _str += _ins_.second.first[0];
                _str += _ins_.second.first[1];
                _str += _ins_.second.first[2];
                if (priority > stoi(_str))
                    priority = stoi(_str);
            }

            for (const auto& _ins_ : ins_list_c) {
                string _str_{};
                _str_ += _ins_.second.first[0];
                _str_ += _ins_.second.first[1];
                _str_ += _ins_.second.first[2];
                if (priority == stoi(_str_)) {
                    line += _ins_.second.second;
                    return _ins_.second.first;
                }
            }
        }
    }
    string toBinary(unsigned num) {
        string str;
        do { str.push_back(static_cast<char>('0' + (num & 1))); } while (num >>= 1);
        std::reverse(str.begin(), str.end());
        return str;
    }
    string padLeftSide(string str, int len) {
        str.insert(str.begin(), len - str.size(), '0');
        return str;
    }

    // Character manipulations
    char digitXOR(char ch1, char ch2) {
        if (((ch1 == '0') && (ch2 == '0')) || ((ch1 == '1') && (ch2 == '1')))
            return '0';
        if (((ch1 == '1') && (ch2 == '0')) || ((ch1 == '0') && (ch2 == '1')))
            return '1';
    }
    char invertDigit(char ch) {
        if (ch == '0')
            return '1';
        else
            return '0';
    }

    // Non-returning helper functions
    void formatDictionary(const string &sParam) {
        fstream fs(sParam);
        string line;
        int count{};
        unordered_map<string, pair<int, int>> temp;
        _data_.push_back(string());

        while (getline(fs, line)) {
            _data_.push_back(line);
            auto possible = temp.find(line);
            if (possible != temp.end())
                possible->second.first++;
            else
                temp.insert(make_pair(line, make_pair(1, count)));
            count++;
        }
        multimap<int, pair<string, int>, greater<int>> temp_dict;
        vector<string> pretty_dict;

        for (auto& entry : temp) {
            temp_dict.insert(make_pair(entry.second.first,
                                         make_pair(entry.first, entry.second.second)));
        }
        int entryCount{};

        while(true) {
            auto range = temp_dict.equal_range(temp_dict.begin()->first);
            map<int, pair<string, int>> temp_map;

            for (auto itr = range.first; itr != range.second; itr++)
                temp_map.insert(make_pair(itr->second.second, make_pair(itr->second.first, itr->first)));

            for (const auto& entry : temp_map) {
                pretty_dict.push_back(entry.second.first);
                entryCount++;
                if (entryCount == 16) {
                    int cnt{};

                    for (auto& thing : pretty_dict)
                        dict.emplace_back(make_pair(thing, cnt++));
                    return;
                }
            }

            temp_map.clear();
            temp_dict.erase(temp_dict.begin()->first);
        }
    }
    void getDictionary(const string &sParam) {
        fstream infile(sParam);
        int count{};
        bool found = false;
        string line;

        while (getline(infile, line)) {
            if (line == "xxxx") {
                found = true;
                continue;
            }
            if (!found) {
                serialized_data = serialized_data + line;
                continue;
            }
            dict.emplace_back(make_pair(line, count++));
        }
    }
    void init_compression() {
        for (int i = 1; i < _data_.size();)
            compressed_data.push_back(getInstruction(_data_[i], i));
    }
    void init_decompression() {
        int code_c{}, line_index{}, dict_index{};
        string prev_ins{}, curr_ins{}, dict_entry{};
        code_c = serialized_data.length();

        for (int i = 0; i < serialized_data.length();) {
            code_c = stoi(serialized_data.substr(static_cast<unsigned long>(i), 3), nullptr, 2);
            i += 3;
            switch (code_c) {
                case (_Format_::Original): {
                    int trail = static_cast<int>(serialized_data.length() - i);
                    if (trail <= 32)
                        if ((stoi(serialized_data.substr(static_cast<unsigned long>(i))) == 0))
                            return;
                    curr_ins = prev_ins = serialized_data.substr(static_cast<unsigned long>(i), 32);
                    decompressed_data += curr_ins;
                    i += 32;
                    line_index++;
                    break;
                }

                case (_Format_::Run_Length): {
                    int var{};
                    int ocurrences = var = stoi(
                            serialized_data.substr(static_cast<unsigned long>(i), 3), nullptr, 2) + 1;

                    while (var) {
                        decompressed_data += prev_ins;
                        line_index++;
                        var--;
                    }
                    prev_ins = "";
                    i += 3;
                    break;
                }

                case (_Format_::Bitmask_Based): {
                    int _str_index{};
                    string ins_d{}, bm{};
                    _str_index=stoi(serialized_data.substr(static_cast<unsigned long>(i), 5), nullptr, 2);
                    i += 5;
                    bm += serialized_data[i];
                    bm += serialized_data[i + 1];
                    bm += serialized_data[i + 2];
                    bm += serialized_data[i + 3];
                    i += 4;
                    dict_index = stoi(serialized_data.substr(static_cast<unsigned long>(i), 4), nullptr, 2);
                    i += 4;
                    dict_entry = curr_ins = dict[dict_index].first;
                    curr_ins[_str_index] = digitXOR(bm[0], dict_entry[_str_index]);
                    curr_ins[_str_index+1] = digitXOR(bm[1], dict_entry[_str_index + 1]);
                    curr_ins[_str_index+2] = digitXOR(bm[2], dict_entry[_str_index + 2]);
                    curr_ins[_str_index+3] = digitXOR(bm[3], dict_entry[_str_index + 3]);
                    prev_ins = curr_ins;
                    decompressed_data += curr_ins;
                    line_index++;
                    break;
                }

                case (_Format_::Mismatch_1b): {
                    int miss_index{};
                    miss_index = stoi(serialized_data.substr(static_cast<unsigned long>(i), 5), nullptr, 2);
                    i += 5;
                    dict_index = stoi(serialized_data.substr(static_cast<unsigned long>(i), 4), nullptr, 2);
                    i += 4;
                    dict_entry = curr_ins = dict[dict_index].first;
                    curr_ins[miss_index] = invertDigit(curr_ins[miss_index]);
                    decompressed_data += curr_ins;
                    prev_ins = curr_ins;
                    line_index++;
                    break;
                }

                case (_Format_::Mismatch_2b_C): {
                    int miss_index_{};
                    miss_index_ = stoi(serialized_data.substr(static_cast<unsigned long>(i), 5), nullptr, 2);
                    i += 5;
                    dict_index = stoi(serialized_data.substr(static_cast<unsigned long>(i), 4), nullptr, 2);
                    i += 4;
                    dict_entry = curr_ins = dict[dict_index].first;
                    curr_ins[miss_index_] = invertDigit(curr_ins[miss_index_]);
                    curr_ins[miss_index_+1] = invertDigit(curr_ins[miss_index_ + 1]);
                    decompressed_data += curr_ins;
                    prev_ins = curr_ins;
                    line_index++;
                    break;
                }

                case (_Format_::Mismatch_4b_C): {
                    int _miss_index_{};
                    _miss_index_ = stoi(serialized_data.substr(static_cast<unsigned long>(i), 5), nullptr, 2);
                    i += 5;
                    dict_index = stoi(serialized_data.substr(static_cast<unsigned long>(i), 4), nullptr, 2);
                    i += 4;
                    dict_entry = curr_ins = dict[dict_index].first;
                    curr_ins[_miss_index_] = invertDigit(curr_ins[_miss_index_]);
                    curr_ins[_miss_index_ + 1] = invertDigit(curr_ins[_miss_index_ + 1]);
                    curr_ins[_miss_index_ + 2] = invertDigit(curr_ins[_miss_index_ + 2]);
                    curr_ins[_miss_index_ + 3] = invertDigit(curr_ins[_miss_index_ + 3]);
                    decompressed_data += curr_ins;
                    prev_ins = curr_ins;
                    line_index++;
                    break;
                }

                case (_Format_::Mismatch_2b_A): {
                    int _miss_1{}, _miss_2{};
                    _miss_1 = stoi(serialized_data.substr(static_cast<unsigned long>(i), 5), nullptr, 2);
                    i += 5;
                    _miss_2 = stoi(serialized_data.substr(static_cast<unsigned long>(i), 5), nullptr, 2);
                    i += 5;
                    dict_index = stoi(serialized_data.substr(static_cast<unsigned long>(i), 4), nullptr, 2);
                    i += 4;
                    dict_entry = curr_ins = dict[dict_index].first;
                    curr_ins[_miss_1] = invertDigit(curr_ins[_miss_1]);
                    curr_ins[_miss_2] = invertDigit(curr_ins[_miss_2]);
                    decompressed_data += curr_ins;
                    prev_ins = curr_ins;
                    line_index++;
                    break;
                }

                case (_Format_::Direct_Match): {
                    dict_index = stoi(serialized_data.substr(i, 4), nullptr, 2);
                    i += 4;
                    curr_ins = dict[dict_index].first;
                    decompressed_data += curr_ins;
                    prev_ins = curr_ins;
                    line_index++;
                    break;
                }
            }
        }
    }
};

int main(int argc, char** argv) {
    // Check if second command-line argument was passed in
    if (argc < 2) {
        cout << "Missing command-line argument." << endl;
        return 0;
    }
    // We have a command-line argument. Checking to see if it's a valid option. Else finish gracefully.
    if (stoi(argv[1]) == 1) {
        Compression CompUtil("cout.txt", true);
        CompUtil.writeOutputFile("cout.txt", true);
    } else if (stoi(argv[1]) == 2) {
        Compression CompUtil("dout.txt", false);
        CompUtil.writeOutputFile("dout.txt", false);
    } else
        cout << "Command-line argument is invalid." << endl;
    return 0;
}