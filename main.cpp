// Copyright (C) 2016 Christopher Beck
//
// This code is available under the terms of the WTFPL 2.0,
// as distributed by Sam Hocevar. (www.wtfpl.net)

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <unordered_map>

class mo_parser {

  std::unordered_map<std::string, std::string> hashmap_;

  void store_message(std::string && msgid, std::string && msgstr) {
    hashmap_.emplace(std::move(msgid), std::move(msgstr));
  }

  ///////////////
  // MO PARSER //
  ///////////////

  //header structure of the MO file format, as described on
  //http://www.gnu.org/software/hello/manual/gettext/MO-Files.html
  struct mo_header {
    uint32_t magic;
    uint32_t version;
    uint32_t number;   // number of strings
    uint32_t o_offset; // offset of original string table
    uint32_t t_offset; // offset of translated string table
  };

  //the original and translated string table consists of
  //a number of string length / file offset pairs:
  struct mo_entry {
    uint32_t length;
    uint32_t offset;
  };

  //handle the contents of an mo file
  void process_mo_contents(const std::string & content) {
    size_t size = content.size();
    if (size < sizeof(mo_header)) {
      std::cerr << "content too small: " << size << " bytes found, expected " << sizeof(mo_header) << " at least\n";
      return;
    }
    mo_header* header = (mo_header*) content.c_str();
    if (header->magic != 0x950412de) {
      std::cerr << "magic number mismatch\n";
      return;
    }
    if (header->version != 0 && header->version != 1) {
      std::cerr << " header version is wrong (not 0 or 1): " << header->version << std::endl;
      return;
    }
    if (header->o_offset + 8*header->number > size ||
      header->t_offset + 8*header->number > size) {
      std::cerr << " header indicates more messages than file has space for...\n";
      return;
    }
    mo_entry* original = (mo_entry*) (content.c_str() + header->o_offset);
    mo_entry* translated = (mo_entry*) (content.c_str() + header->t_offset);

    for (unsigned i = 0; i < header->number; ++i) {
      if (original[i].offset + original[i].length > size ||
        translated[i].offset + translated[i].length > size) {
        std::cerr << " file ended prematurely\n";
        return;
      }
      std::string msgid = content.substr(original[i].offset, original[i].length);
      std::string msgstr = content.substr(translated[i].offset, translated[i].length);

      store_message(std::move(msgid), std::move(msgstr));
    }
  }

public:
  explicit mo_parser(const std::string & filename)
    : hashmap_()
  {
    std::ifstream ifs{filename};
    if (ifs) {
      std::string contents{std::istreambuf_iterator<char>{ifs}, std::istreambuf_iterator<char>{}};
      process_mo_contents(contents);
    }
  }

  const std::unordered_map<std::string, std::string> & get_map() { return hashmap_; }
};

std::string quote_escape_string(const std::string & str) {
  std::string result{"\""};
  for (const char c : str) {
    switch (c) {
      case '\n': {
        result += "\\n";
        break;
      }
      case '\t': {
        result += "\\t";
        break;
      }
      case '\0': {
        result += "\\0";
        break;
      }
      case '"': {
        result += "\\\"";
        break;
      }
      case '\\': {
        result += "\\\\";
        break;
      }
      default:
        result += c;
    }
  }
  result += '\"';
  return result;
}

const char * prog_name;

void print_usage() {
  const char * prog = (prog_name ? prog_name : "mo_dump");
  std::cerr << "Usage:\n  " << prog << " mo-filename keys\n  "  << prog << " mo-filename pairs\n\n";
}

int main(int argc, char* argv[] ) {
  prog_name = argv[0];
  if (argc < 3) {
    print_usage();
    return 1;
  }
  std::string filename{argv[1]};
  if (!std::ifstream(filename)) {
    print_usage();
    std::cerr << "Could not open file '"  << filename << "'\n";
    return 1;
  }

  mo_parser mo{filename};

  std::cout << "Read " << mo.get_map().size() << " entries:\n";

  std::string action{argv[2]};

  if (action == "keys") {
    for (const auto & p : mo.get_map()) {
      std::cout << "  " << quote_escape_string(p.first) << std::endl;
    }
  } else if (action == "pairs") {
    for (const auto & p : mo.get_map()) {
      std::cout << "  " << quote_escape_string(p.first) << " -> " << quote_escape_string(p.second) << std::endl;
    }   
  } else {
    print_usage();
  }

  std::cout << std::endl;

}
