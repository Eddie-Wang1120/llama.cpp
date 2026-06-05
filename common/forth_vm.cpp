#include "forth_vm.h"
#include "common.h"
#include <sstream>
#include <iostream>
#include <cmath>
#include "cxxforth.h"
#include <filesystem>
#include <string>
ForthVM::ForthVM() {
}
std::vector<std::string> ForthVM::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}
int ForthVM::evaluate(int argc, char** argv, const std::string& command) const {
    std::cout << "  in ForthVM::evaluate()  ";
    forthvm_main(argc,argv,(char*) command.c_str());
    return string_stack_.empty() ? 0 : string_stack_.size();
}
bool ForthVM::execute(const std::string& command, llama_context* ctx) {
    auto tokens = tokenize(command);
    int i;
    std::cout << "  size() " << tokens.size() << " / ";
    for(i=0;i<tokens.size();i++)
    {
        std::cout << tokens[i] << " / ";
    }
    if (tokens[0]=="FORTH") {
      std::cout << "  is FORTH !!  msg_start: ";
      tokens[1]="_/hello.fs";
      std::cout << "Current working directory: "
          << std::filesystem::current_path() << std::endl;
      std::vector<char*> vec;
      std::transform(std::begin(tokens), std::end(tokens),
                     std::back_inserter(vec),
                     [](std::string& s){ s.push_back(0); return &s[0]; });
      vec.push_back(nullptr);
      char** carray = vec.data();
      evaluate(tokens.size(), carray, command);
    }
    for (const auto& token : tokens) {
        auto it = handlers_.find(token);
        if (it != handlers_.end()) {
            if (!it->second(*this, ctx)) {
                std::cerr << "[ForthVM] Error executing command: " << token << std::endl;
                return false;
            }
        } else {
            string_stack_.push(token);
            try {
                value_stack_.push(std::stod(token));
            } catch (...) {
            }
        }
    }
    return true;
}
bool ForthVM::cmd_add(ForthVM& vm, llama_context*) {
    if (vm.value_stack_.size() < 2) return false;
    double b = vm.value_stack_.top(); vm.value_stack_.pop();
    double a = vm.value_stack_.top(); vm.value_stack_.pop();
    vm.value_stack_.push(a + b);
    return true;
}
bool ForthVM::cmd_sub(ForthVM& vm, llama_context*) {
    if (vm.value_stack_.size() < 2) return false;
    double b = vm.value_stack_.top(); vm.value_stack_.pop();
    double a = vm.value_stack_.top(); vm.value_stack_.pop();
    vm.value_stack_.push(a - b);
    return true;
}
bool ForthVM::cmd_mul(ForthVM& vm, llama_context*) {
    if (vm.value_stack_.size() < 2) return false;
    double b = vm.value_stack_.top(); vm.value_stack_.pop();
    double a = vm.value_stack_.top(); vm.value_stack_.pop();
    vm.value_stack_.push(a * b);
    return true;
}
bool ForthVM::cmd_div(ForthVM& vm, llama_context*) {
    if (vm.value_stack_.size() < 2) return false;
    double b = vm.value_stack_.top(); vm.value_stack_.pop();
    double a = vm.value_stack_.top(); vm.value_stack_.pop();
    if (std::abs(b) < 1e-10) return false;
    vm.value_stack_.push(a / b);
    return true;
}
bool ForthVM::cmd_sym(ForthVM& vm, llama_context*) {
    if (vm.string_stack_.empty()) return false;
    std::string sym_name = vm.string_stack_.top(); vm.string_stack_.pop();
    std::cout << "[ForthVM] Registered symbol: " << sym_name << std::endl;
    return true;
}
bool ForthVM::cmd_temp(ForthVM& vm, llama_context* ctx) {
    if (vm.value_stack_.empty()) return false;
    double temp = vm.value_stack_.top(); vm.value_stack_.pop();
    if (ctx && temp >= 0.0 && temp <= 2.0) {
        std::cout << "[ForthVM] Temperature set to: " << temp << " (requires sampler re-init)" << std::endl;
        return true;
    }
    return false;
}
bool ForthVM::cmd_top_p(ForthVM& vm, llama_context* ctx) {
    if (vm.value_stack_.empty()) return false;
    double top_p = vm.value_stack_.top(); vm.value_stack_.pop();
    if (ctx && top_p >= 0.0 && top_p <= 1.0) {
        std::cout << "[ForthVM] Top-p set to: " << top_p << " (requires sampler re-init)" << std::endl;
        return true;
    }
    return false;
}
bool ForthVM::cmd_print(ForthVM& vm, llama_context*) {
    if (!vm.value_stack_.empty()) {
        std::cout << "[ForthVM] Value TOS: " << vm.value_stack_.top() << std::endl;
    } else if (!vm.string_stack_.empty()) {
        std::cout << "[ForthVM] String TOS: " << vm.string_stack_.top() << std::endl;
    } else {
        std::cout << "[ForthVM] Stacks empty" << std::endl;
    }
    return true;
}
std::string ForthVM::top_string() const {
    return string_stack_.empty() ? "" : string_stack_.top();
}
int ForthVM::size() const {
    return string_stack_.empty() ? 0 : string_stack_.size();
}
bool ForthVM::has_string() const { return !string_stack_.empty(); }
double ForthVM::top_value() const {
    return value_stack_.empty() ? 0.0 : value_stack_.top();
}
bool ForthVM::has_value() const { return !value_stack_.empty(); }
int ForthVM::forth(int argc, char** argv) const {
    cxxforth_main(argc,argv);
    return string_stack_.empty() ? 0 : string_stack_.size();
}
#include <typeindex>
using namespace std;
std::vector<std::string> PhosVM::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}
extern std::stack<std::any> S;
std::string anyToString(const std::any& value) {
    if (!value.has_value()) {
        return "[empty]";
    }
    if (value.type() == typeid(std::string)) {
        return std::any_cast<std::string>(value);
    }
    if (value.type() == typeid(const char*)) {
        return std::any_cast<const char*>(value);
    }
    if (value.type() == typeid(int)) {
        return std::to_string(std::any_cast<int>(value));
    }
    if (value.type() == typeid(double)) {
        return std::to_string(std::any_cast<double>(value));
    }
    if (value.type() == typeid(bool)) {
        return std::any_cast<bool>(value) ? "true" : "false";
    }
    return "[unsupported type]";
}
bool PhosVM::find(int ctx){
    try {
      gettype(ctx);
      string v_type = anyToString(S.top()); S.pop();
      int n;
      if (v_type=="string")
      {
        string s0=anyToString(S.top());
        std::cout << "  s0 " << s0 << std::endl;
        n = stoi(s0,nullptr,10);
        S.pop();
      }
      else if (v_type=="int") {
        n = any_cast<int>(S.top());
        S.pop();
      }
      string ptn = anyToString(S.top()); S.pop();
          std::cout.flush();
    int D=S.size(); int m,i;
    for(i=0;i<n;i++)
    {
        m = D-i-1;
        S.push(i);
        pick(ctx);
        string tok = anyToString(S.top());
        cout << tok << " " << i << " find" << endl;
        S.pop();
        S.pop();
        size_t pos = tok.find(ptn);
        if (pos != std::string::npos) {
            std::cout << "Found at position: " << pos << "  depth " << S.size() << std::endl;
            S.push((int)i); S.push((int)pos);
            return true;
        } else {
            std::cout << "Not found!" << std::endl;
        }
        {
          std::cout.flush();
        }
        S.push("FIND");
    }
    }
    catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool DBG = true;
#define dcout if (DBG) cout
std::string join(const std::vector<std::string> &lst, const std::string &delim)
{
    std::string ret;
    for(const auto &s : lst) {
        if(!ret.empty())
            ret += delim;
        ret += s;
    }
    return ret;
}
void PhosVM::execute(std::string cmd, int ctx) {
      std::deque<std::any>* P;
      P = (std::deque<std::any>*) &S;
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
      unordered_map<string,vector<string>> c;
      DBG = false;
      if (m.count("C")) dcout << "m has CDW" << endl;
      else dcout << "in m CDW not found" << endl;
      std::any container = m["C"];
      if (auto ptr = std::any_cast<int>(&container)) {
          dcout << "int value is " << *ptr << "\n";
      }
      else if (auto ptr = std::any_cast<unordered_map<string,vector<string>>>(&container)) {
          std::cout << "vector of string: " << (*ptr).count("fruits") << "\n";
          c = *ptr;
          for (const auto& str : c["fruits"]) {
              dcout << str << " ";
          }
      }
      else std::cout << "CDW not found" << endl;
      container = m["LR"];
      if (auto ptr = std::any_cast<int>(&container)) {
          dcout << "  LR value is " << *ptr << "\n";
      }
      container = m["LS"];
      if (auto ptr = std::any_cast<int>(&container)) {
          dcout << "  LS value is " << *ptr << "\n";
      }
      container = m["DBG"];
      if (auto ptr = std::any_cast<bool>(&container)) {
          std::cout << "  DBG value is " << *ptr << "\n";
      }
    auto tokens = tokenize(cmd);
    int i; std::string tok;
    if (tokens[1]=="DBG") std::cout << "  size() " << tokens.size() << " / ";
    for(i=0;i<tokens.size();i++)
    {
        if (tokens[1]=="DBG") std::cout << tokens[i] << " / ";
        tok=tokens[i];
        if (c.count(tok)) {
            for (const auto& str : c[tok]) {
                std::cout << str << " ";
            }
            execute(join(c[tok], " "),ctx);
            dcout << "AFTER colon def word" << endl;
        } else
        if (handlers.count(tok)) {
            Handler func = handlers[tok];
            (this->*func)(ctx);
        }
        else S.push(tok);
        std::cout.flush();
    }
    DBG = true;
    if (tokens[0]=="PHOS") {
        dcout << "\n  end PHOS !!  msg_start: ";
        if (handlers.count(cmd)) {
            Handler func = handlers[cmd];
            (this->*func)(ctx);
        }
    }
}
bool PhosVM::dup(int ctx) { std::cout << "dup ...\n";
    if (S.empty()) return false;
    std::any topElement = S.top();
    S.push(topElement);
    return true;
}
bool PhosVM::peekr(int ctx) { std::cout << "m n peekr ...\n";
    std::cout.flush();
    gettype(ctx);
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10);
      S.pop();
    }
    else if (v_type=="int") {
      n = any_cast<int>(S.top());
      S.pop();
    }
    gettype(ctx);
    v_type=anyToString(S.top()); S.pop();
    int m;
    if (v_type=="string")
    {
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      m = stoi(s0,nullptr,10);
      S.pop();
    }
    else if (v_type=="int") {
      m = any_cast<int>(S.top());
      S.pop();
    }
    int i;
    for(i=m; i<n; i++) {
      S.push(i); pick(ctx);
      gettype(ctx);
      v_type=anyToString(S.top()); S.pop();
      int m;
      if (v_type=="string")
      {
        string s0=anyToString(S.top());
        std::cout << "  s0 " << s0 << std::endl;
        S.pop();
      }
      else if (v_type=="int") {
        m = any_cast<int>(S.top());
        S.pop();
      }
    }
    return true;
}
bool compareTypeIndexWithString(const std::type_index& ti, const std::string& typeName) {
    if (typeName == "int") return ti == std::type_index(typeid(int));
    if (typeName == "double") return ti == std::type_index(typeid(double));
    if (typeName == "std::string") return ti == std::type_index(typeid(std::string));
    return false;
}
bool PhosVM::depth(int ctx) {
    if (S.empty()) return false;
    int b=S.size();
    S.push(b);
    std::type_index my_type = std::type_index(S.top().type());
    if (compareTypeIndexWithString(my_type, "std::string")) {
      std::cout << "type_index is std::string ";
    }
    else if (compareTypeIndexWithString(my_type, "int")) {
        std::cout << "type_index is int ";
    }
    else std::cout << "type_index " << type_index(S.top().type()).name() << " ";
    std::cout << "depth " << S.size() << endl;
    {
    }
    return true;
}
bool PhosVM::gettype(int ctx) {
    std::type_index my_type = std::type_index(S.top().type());
    if (compareTypeIndexWithString(my_type, "std::string")) {
      std::cout << "type_index is std::string! ";
      S.push("string");
    }
    else if (compareTypeIndexWithString(my_type, "int")) {
      std::cout << "type_index is int! ";
      S.push("int");
    }
    else {
      std::cout << "type_index " << type_index(S.top().type()).name() << " ";
      S.push(my_type);
    }
    return true;
}
bool PhosVM::gettype_any(int ctx) {
    std::any a = std::type_index(S.top().type());
    S.pop();
    S.push(a);
    return true;
}
bool PhosVM::tostr(int ctx) {
    std::cout << "  tostr ...\n";
    if ( std::any_cast<std::type_index>(S.top()) == std::type_index(typeid(int)) ) {
        S.pop();
        int b = std::any_cast<int>(S.top()); S.pop();
        S.push(std::to_string(b));
    }
    return true;
}
bool PhosVM::type(int ctx){
    std::cout << std::any_cast<std::string>(S.top()) ; S.pop();
    return true;
}
bool PhosVM::cr(int ctx){
    std::cout << std::endl ;
    return true;
}
bool PhosVM::showstack(int ctx){
try {
        std::deque<std::any>* P;
    P = (std::deque<std::any>*) &S;
            std::cout << "  P = &S ... OK  " << std::endl;
            std::cout << S.top().type().name() << std::endl;
    int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10);
    S.pop();
    std::cout << "  n is " << n << std::endl;
    std::cout << (*P)[n].type().name() << std::endl;
    if (typeid((*P)[n].type())==typeid(std::string))
        std::cout << "  showstack ... is string  " << std::endl;
    else std::cout << "  is not string  " << std::endl;
    std::cout << std::any_cast<std::string>((*P)[n]) << std::endl;
}
catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::peek_any(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
        std::cout << "  P = &S ... OK  " << std::endl;
        std::cout << S.top().type().name() << std::endl;
    int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10);
    S.pop();
    std::cout << "  depth " << S.size() << std::endl;
    std::cout << "  n is " << n << std::endl;
    std::cout << (*P)[n].type().name() << std::endl;
    if (typeid((*P)[n].type())==typeid(std::string))
        std::cout << "  showstack ... is string  " << std::endl;
    else std::cout << "  is not string  " << std::endl;
    std::cout << std::any_cast<std::string>((*P)[n]) << std::endl;
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
#include <concepts>
#include <type_traits>
template <typename T>
void processData(const T& data) {
    if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        std::cout << "Processing a vector of strings.\n";
    } else {
        std::cout << "Processing a different type.\n";
    }
}
bool PhosVM::peek(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    gettype(ctx);
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10);
      S.pop();
    }
    else if (v_type=="int") {
      n = any_cast<int>(S.top());
      S.pop();
    }
      std::cout << "  depth " << S.size();
      std::cout << "  n " << n;
      std::cout.flush();
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::k_M(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    std::cout << "  k_M m DEFINED   ";
    vector<string> keys;
    for (const auto& [key, value] : m) {
        std::cout << key << " ";
        keys.push_back(key);
    }
    S.push(keys);
    std::cout << "\ndepth " << S.size() << "  read test  ";
    vector<string> V0=any_cast<vector<string>>(S.top());
    for (const auto& key : V0) {
        std::cout << key << " ";
    }
    processData(V0);
    std::any container = S.top();
    if (auto ptr = std::any_cast<int>(&container)) {
        std::cout << "Success! Value is " << *ptr << "\n";
    }
    else if (auto ptr = std::any_cast<vector<string>>(&container)) {
        std::cout << "Success! ptr  ";
        for (const auto& key : *ptr) {
          std::cout << key << " ";
        }
    }
    else {
        std::cout << "Type mismatch or container is empty.\n";
    }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::pvs(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
        std::any container = S.top();
    if (auto ptr = std::any_cast<int>(&container)) {
        std::cout << "Success! Value is " << *ptr << "\n";
    }
    else if (auto ptr = std::any_cast<vector<string>>(&container)) {
        std::cout << "\npvs success ptr:  ";
        for (const auto& key : *ptr) {
          std::cout << key << " ";
        }
    }
    else {
        std::cout << "Type mismatch or container is empty.\n";
    }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::inV(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    std::any container;
    std::string target = "banana";
    int n=2;
    while (n-->0) {
      container = S.top(); S.pop();
      if (auto ptr = std::any_cast<int>(&container)) {
          std::cout << "Success! Value is " << *ptr << "\n";
      }
      else if (auto ptr = std::any_cast<vector<string>>(&container)) {
          std::cout << "\ninV success ptr  ";
          for (const auto& key : *ptr) {
            std::cout << key << " ";
          }
          container = *ptr;
      }
      else if (auto ptr = std::any_cast<string>(&container)) {
          target = *ptr;
      }
      else {
          std::cout << "Type mismatch or container is empty.\n";
      }
    }
    for (const auto& key : any_cast<vector<string>>(container)) {
          std::cout << key << " ";
    }
    std::vector<std::string> fruits = {"apple", "banana", "cherry"};
    vector<string> vec = any_cast<vector<string>>(container);
    auto it = std::find(vec.begin(), vec.end(), target);
    if (it != vec.end()) {
        int index = std::distance(vec.begin(), it);
        std::cout << target << " found at index: " << index << std::endl;
    } else {
        std::cout << target << " not found." << std::endl;
    }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::g_M(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    string key=anyToString(S.top()); S.pop();
    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    std::cout << "  g_M m DEFINED " << endl;
    S.push(m[key]);
    std::any container = m[key];
    if (auto ptr = std::any_cast<int>(&container)) {
        std::cout << "int value is " << *ptr << "\n";
    }
    else if (auto ptr = std::any_cast<bool>(&container)) {
        std::cout << "bool value is " << *ptr << "\n";
    }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::g_M_gettype(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    string key=anyToString(S.top()); S.pop();
    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    std::cout << "  g_M m DEFINED " << endl;
    S.push(m[key]);
    gettype(ctx);
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      m[key]=s0;
      std::cout << "  g_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<string>(m[key]) << std::endl;
      S.pop();
    }
    else if (v_type=="int") {
      n = any_cast<int>(S.top());
      m[key]=(int) n;
      S.pop();
      std::cout << "  g_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m[key]) << std::endl;
    }
    std::cout << "  g_M m ASSIGNED " << endl;
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::push(int ctx) {
    return true;
}
bool PhosVM::s_M(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    string key=anyToString(S.top()); S.pop();
    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    std::cout << "\ns_M m DEFINED " << endl;
    std::any container = S.top();
    if (auto ptr = std::any_cast<struct llama_model*>(&container)) {
        dcout << "llama_model value is " << "\n";
        m[key]=*ptr;
        S.pop();
    }
    else if (auto ptr = std::any_cast<std::vector<common_chat_msg>>(&container)) {
        dcout << "vector common_chat_msg value is ..." << "\n";
        m[key]=*ptr;
        S.pop();
    }
    else if (auto ptr = std::any_cast<common_params*>(&container)) {
        dcout << "common_params* key is " << key << "\n";
        m[key]=*ptr;
        S.pop();
    }
    else {
      gettype(ctx);
      string v_type=anyToString(S.top()); S.pop();
      int n;
      if (v_type=="string")
      {
        string s0=anyToString(S.top());
        std::cout << "  s0 " << s0 << std::endl;
        if (s0=="t_bool") {
          S.pop();
          s0=anyToString(S.top());
          if (s0=="true") m[key]=true;
          else if (s0=="false") m[key]=false;
          else m[key]=true;
          if (key=="DBG") DBG=any_cast<bool>(m[key]);
        }
        else m[key]=s0;
        S.pop();
      }
      else if (v_type=="int") {
        n = any_cast<int>(S.top());
        m[key]=(int) n;
        S.pop();
        std::cout << "  s_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m[key]) << std::endl;
      }
      std::cout << "  s_M m ASSIGNED " << endl;
    }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::g_M_debug(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    gettype(ctx);
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10);
      S.pop();
    }
    else if (v_type=="int") {
      n = any_cast<int>(S.top());
      S.pop();
    }
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
      std::cout << "  pick pM DEFINED " << endl;
      std::cout << "  pick pM ASSIGNED " << endl;
      std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m["test"]) << std::endl;
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::p_M(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    gettype(ctx);
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10);
      S.pop();
    }
    else if (v_type=="int") {
      n = any_cast<int>(S.top());
      S.pop();
    }
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
      std::cout << "  pick pM DEFINED " << endl;
      m["test"]=(int) n;
      std::cout << "  pick pM ASSIGNED " << endl;
      std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m["test"]) << std::endl;
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
bool PhosVM::pick(int ctx){
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S;
    gettype(ctx);
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10);
      S.pop();
    }
    else if (v_type=="int") {
      n = any_cast<int>(S.top());
      S.pop();
    }
      n = S.size() - n - 1;
      std::cout << "  depth " << S.size() << std::endl;
      std::cout << "  n is " << n << std::endl;
      std::cout << (*P)[n].type().name() << std::endl;
      std::cout << "type_index " << type_index((*P)[n].type()).name() << std::endl;
      std::type_index my_type = std::type_index(typeid(int));
      if (compareTypeIndexWithString(my_type, "int")) {
        std::cout << "The type_index is an int!\n";
      }
      my_type = std::type_index(typeid(std::string));
      if (compareTypeIndexWithString(my_type, "std::string")) {
        std::cout << "The type_index is an std::string!\n";
      }
      my_type = std::type_index((*P)[n].type());
      if (compareTypeIndexWithString(my_type, "std::string")) {
        std::cout << "The type_index is an std::string!\n";
      }
      std::cout.flush();
      std::cout << "  pick " << std::any_cast<std::string>((*P)[n]) << std::endl;
      S.push((*P)[n]);
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}
#include <regex>
static const std::regex INT_TYPE("[+-]?[0-9]+");
static const std::regex UNSIGNED_INT_TYPE("[+]?[0-9]+");
static const std::regex DOUBLE_TYPE("[+-]?[0-9]+[.]?[0-9]+");
static const std::regex UNSIGNED_DOUBLE_TYPE("[+]?[0-9]+[.]?[0-9]+");
bool isIntegerType(const std::string& str_)
{
  return std::regex_match(str_, INT_TYPE);
}
bool isUnsignedIntegerType(const std::string& str_)
{
  return std::regex_match(str_, UNSIGNED_INT_TYPE);
}
bool isDoubleType(const std::string& str_)
{
  return std::regex_match(str_, DOUBLE_TYPE);
}
bool isUnsignedDoubleType(const std::string& str_)
{
  return std::regex_match(str_, UNSIGNED_DOUBLE_TYPE);
}
bool PhosVM::add(int ctx) { dcout << "Adding...\n";
      int a, b, e; double c, d, f;
      vector<int> I;
      vector<double> D;
      std::any container = S.top(); S.pop();
      if (auto ptr = std::any_cast<string>(&container)) {
          dcout << "string value is " << *ptr << "\n";
          if (isIntegerType(*ptr)) { a = stoi(*ptr); I.push_back(a); }
          else if (isDoubleType(*ptr)) { c = stod(*ptr); D.push_back(c); }
      }
      container = S.top(); S.pop();
      if (auto ptr = std::any_cast<string>(&container)) {
          dcout << "string value is " << *ptr << "\n";
          if (isIntegerType(*ptr)) { b = stoi(*ptr); I.push_back(b); }
          else if (isDoubleType(*ptr)) { d = stod(*ptr); D.push_back(d); }
      }
      if (I.size()==2) { e=a+b; S.push(e); dcout << e; }
      else if (D.size()==2) { f=c+d; S.push(f); dcout << f; }
      else if (D.size()==1 && I.size()==1) { f = I[0] + D[0]; S.push(f); dcout << f; }
      else { dcout << "type mismatch"; S.push("E_TYPE"); }
      dcout << endl;
  return true;
}
bool PhosVM::sub(int ctx) { dcout << "Subtracting...\n";
      int a, b, e; double c, d, f;
      vector<int> I;
      vector<double> D;
      string TOS;
      std::any container = S.top(); S.pop();
      if (auto ptr = std::any_cast<string>(&container)) {
          dcout << "string value is " << *ptr << "\n";
          if (isIntegerType(*ptr)) { a = stoi(*ptr); I.push_back(a); TOS="int"; }
          else if (isDoubleType(*ptr)) { c = stod(*ptr); D.push_back(c); TOS="double"; }
      }
      container = S.top(); S.pop();
      if (auto ptr = std::any_cast<string>(&container)) {
          dcout << "string value is " << *ptr << "\n";
          if (isIntegerType(*ptr)) { b = stoi(*ptr); I.push_back(b); }
          else if (isDoubleType(*ptr)) { d = stod(*ptr); D.push_back(d); }
      }
      if (I.size()==2) { e=b-a; S.push(e); dcout << e; }
      else if (D.size()==2) { f=d-c; S.push(f); dcout << f; }
      else if (D.size()==1 && I.size()==1) {
        if (TOS=="int") f = D[0] - I[0];
        else f = I[0] - D[0];
        S.push(f); dcout << f; }
      else { dcout << "type mismatch"; S.push("E_TYPE"); }
      dcout << endl;
      return true;
}
#include "common.h"
#include "sampling.h"
#include "llama.h"
#include "log.h"
static common_params * g_params;
static std::string phos_chat_add_and_format(struct llama_model * model, std::vector<common_chat_msg> & chat_msgs, const std::string & role, const std::string & content) {
    common_chat_msg new_msg{role, content};
    auto formatted = common_chat_format_single(model, g_params->chat_template, chat_msgs, new_msg, role == "user");
    chat_msgs.push_back({role, content});
    LOG("formatted: '%s'\n", formatted.c_str());
    return formatted;
}
bool PhosVM::CAAF(int ctx) { dcout << "\nchat_add_and_format  ";
     execute("model g_M", 0);
     struct llama_model * model = any_cast<struct llama_model*>(S.top()); S.pop();
     execute("role g_M", 0);
     string role = any_cast<string>(S.top()); S.pop();
     execute("content g_M", 0);
     string content = any_cast<string>(S.top()); S.pop();
     execute("chat_msgs g_M",0);
     vector<common_chat_msg> chat_msgs = any_cast<vector<common_chat_msg>>(S.top()); S.pop();
     execute("g_params g_M",0);
     g_params = any_cast<common_params*>(S.top()); S.pop();
     dcout << "\nCAAF model  " <<
     phos_chat_add_and_format(model, chat_msgs, role, content) << "   end\n";
     return true;
}
