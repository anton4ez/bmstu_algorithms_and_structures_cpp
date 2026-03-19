#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <iomanip>

using namespace std;

struct ModeSet {
    bool control = false;
    bool table = false;
    bool numeric = false;
    bool memory = false;
};

class WasmParser {
private:
    vector<uint8_t> data;
    size_t offset = 0;
    ModeSet mode;

    uint8_t read_u8() {
        if (offset >= data.size()) throw runtime_error("Unexpected EOF while reading u8");
        return data[offset++];
    }

    uint32_t read_leb_u32() {
        uint32_t result = 0, shift = 0;
        while (true) {
            uint8_t byte = read_u8();
            result |= (byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
            if (shift >= 35) throw runtime_error("LEB_u32 too large");
        }
        return result;
    }

    int32_t read_leb_s32() {
        int32_t result = 0;
        uint32_t shift = 0;
        uint8_t byte = 0;
        while (true) {
            byte = read_u8();
            result |= (byte & 0x7F) << shift;
            shift += 7;
            if ((byte & 0x80) == 0) break;
            if (shift >= 35) throw runtime_error("LEB_s32 too large");
        }
        if ((shift < 32) && (byte & 0x40)) result |= (~0u << shift);
        return result;
    }

    uint64_t read_leb_u64() {
        uint64_t result = 0;
        uint32_t shift = 0;
        while (true) {
            uint8_t byte = read_u8();
            result |= static_cast<uint64_t>(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0) break;
            shift += 7;
            if (shift >= 70) throw runtime_error("LEB_u64 too large");
        }
        return result;
    }

    float read_f32_ieee() {
        if (offset + 4 > data.size()) throw runtime_error("EOF while reading f32");
        uint32_t u = data[offset] | (data[offset+1] << 8) | (data[offset+2] << 16) | (data[offset+3] << 24);
        offset += 4;
        float f;
        memcpy(&f, &u, sizeof(f));
        return f;
    }

    double read_f64_ieee() {
        if (offset + 8 > data.size()) throw runtime_error("EOF while reading f64");
        uint64_t u = 0;
        for (int i = 0; i < 8; ++i) u |= static_cast<uint64_t>(data[offset+i]) << (8 * i);
        offset += 8;
        double d;
        memcpy(&d, &u, sizeof(d));
        return d;
    }

    string valtype_name(uint8_t t) {
        switch (t) {
            case 0x7F: return "i32";
            case 0x7E: return "i64";
            case 0x7D: return "f32";
            case 0x7C: return "f64";
            case 0x7B: return "v128";
            case 0x70: return "funcref";
            case 0x6F: return "externref";
            case 0x40: return "empty";
            default: return "unknown";
        }
    }

    string blocktype_to_string() {
        size_t save = offset;
        uint8_t peek = read_u8();
        if (peek == 0x40) return "empty";
        if (peek == 0x7F || peek == 0x7E || peek == 0x7D || peek == 0x7C || peek == 0x7B || peek == 0x70 || peek == 0x6F) {
            return valtype_name(peek);
        }
        offset = save;
        int32_t s = read_leb_s32();
        return "typeidx(" + to_string(s) + ")";
    }

    void print_indent(int indent) {
        for (int i = 0; i < indent; ++i) cout << "  ";
    }

public:
    WasmParser(const string& filename, ModeSet m) : mode(m) {
        ifstream file(filename, ios::binary | ios::ate);
        if (!file.is_open()) throw runtime_error("Cannot open file: " + filename);
        streamsize size = file.tellg();
        file.seekg(0, ios::beg);
        data.resize(size);
        if (file.read(reinterpret_cast<char*>(data.data()), size)) {
            offset = 0;
        } else {
            throw runtime_error("Error reading file");
        }
    }

    void verify_header() {
        if (data.size() < 8) throw runtime_error("File too small");
        if (!(data[0] == 0x00 && data[1] == 0x61 && data[2] == 0x73 && data[3] == 0x6D)) {
            throw runtime_error("Invalid WASM magic number");
        }
        offset = 8;
    }

    bool find_section(uint8_t wanted_id, size_t& payload_off, uint32_t& payload_len) {
        offset = 8;
        while (offset < data.size()) {
            uint8_t id = read_u8();
            uint32_t len = read_leb_u32();
            if (offset + len > data.size()) throw runtime_error("Section length out of bounds");
            if (id == wanted_id) {
                payload_off = offset;
                payload_len = len;
                return true;
            }
            offset += len;
        }
        return false;
    }

    void parse_code_section() {
        size_t payload_off;
        uint32_t payload_len;
        if (!find_section(10, payload_off, payload_len)) {
            cout << "Section ID=10 (Code) not found.\n";
            return;
        }

        offset = payload_off;
        size_t end = payload_off + payload_len;
        uint32_t func_count = read_leb_u32();
        cout << "CodeSection (id=10), functions = " << func_count << "\n";

        for (uint32_t fi = 0; fi < func_count; ++fi) {
            uint32_t body_size = read_leb_u32();
            size_t body_start = offset;
            size_t body_end = offset + body_size;

            if (body_end > end) throw runtime_error("Function body overruns section");
            cout << "\nfunc [" << fi << "]: body_size=" << body_size << "\n";
            
            uint32_t local_decl_cnt = read_leb_u32();
            cout << "  locals_decls=" << local_decl_cnt << "\n";
            for (uint32_t i = 0; i < local_decl_cnt; ++i) {
                uint32_t n = read_leb_u32();
                uint8_t t = read_u8();
                cout << "    local_decl: count=" << n << " type=" << valtype_name(t) << "\n";
            }

            int indent = 1;
            vector<uint8_t> block_stack;
            block_stack.push_back(255);

            while (offset < body_end) {
                uint8_t op = read_u8();
                
                if (op == 0x0B) {
                    indent--;
                    print_indent(indent); cout << "end\n";
                    if (!block_stack.empty()) block_stack.pop_back();
                    if (block_stack.empty() || offset == body_end) break;
                    continue;
                }

                if (op == 0x04) {
                    string bt = blocktype_to_string();
                    print_indent(indent); cout << "if " << bt << "\n";
                    block_stack.push_back(3);
                    indent++;
                    continue;
                }
                
                if (op == 0x05) {
                    indent--;
                    print_indent(indent); cout << "else\n";
                    indent++;
                    continue;
                }

                if (mode.control && (op <= 0x1F || op == 0xFB || op == 0xD5 || op == 0xD6)) {
                    print_indent(indent); cout << "control_op 0x" << hex << (int)op << dec << "\n";
                } else if (mode.table && (op == 0x25 || op == 0x26)) {
                    uint32_t x = read_leb_u32();
                    print_indent(indent); cout << "table.op " << x << "\n";
                } else if (mode.numeric && ((op >= 0x41 && op <= 0x44) || (op >= 0x45 && op <= 0x66))) {
                    if (op == 0x41) {
                        uint32_t n = read_leb_u32();
                        print_indent(indent); cout << "i32.const " << n << "\n";
                    } else {
                        print_indent(indent); cout << "numeric_op 0x" << hex << (int)op << dec << "\n";
                    }
                } else if (mode.memory && ((op >= 0x28 && op <= 0x3E) || op == 0x3F || op == 0x40)) {
                    print_indent(indent); cout << "memory_op 0x" << hex << (int)op << dec << "\n";
                }
            }
            offset = body_end;
        }
    }
};

int main(int argc, char** argv) {
    if (argc < 2) {
        cout << "Usage: wasm_parser <file.wasm> [--mode 12 13 14 all]\n";
        return 1;
    }

    string filename = argv[1];
    ModeSet mode;
    mode.control = mode.table = mode.numeric = mode.memory = true;
    try {
        WasmParser parser(filename, mode);
        parser.verify_header();
        parser.parse_code_section();
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
