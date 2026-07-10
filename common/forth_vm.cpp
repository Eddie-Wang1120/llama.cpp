#include "forth_vm.h"
#include "common.h"  // llama.cpp common utilities
#include <sstream>
#include <iostream>
#include <cmath>
#include "cxxforth.h"
#include <filesystem>
#include <string>

// ForthVM fvm;

ForthVM::ForthVM() {

    // handlers_["+"] = &cmd_add;
    // handlers_["+"] = &ForthVM::cmd_add;
    // handlers_["+"] = cmd_add;

    // Register built-in arithmetic handlers
    // handlers_["+"] = &(ForthVM::cmd_add);
    // handlers_["+"] = ForthVM::cmd_add;
    // handlers_["+"] = forth_vm::cmd_add;
    // handlers_["+"] = *this::cmd_add;
/*    handlers_["-"] = &ForthVM::cmd_sub;
    handlers_["*"] = &ForthVM::cmd_mul;
    handlers_["/"] = &ForthVM::cmd_div;
    
    // Register LLM control handlers
    handlers_["sym:"] = &ForthVM::cmd_sym;
    handlers_["temp:"] = &ForthVM::cmd_temp;
    handlers_["top-p:"] = &ForthVM::cmd_top_p;
    handlers_["print:"] = &ForthVM::cmd_print;
*/
}

std::vector<std::string> ForthVM::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {  // space-delimited, skips whitespace
        tokens.push_back(token);
    }
    return tokens;
}

// call cxxforth evaluate()
int ForthVM::evaluate(int argc, char** argv, const std::string& command) const {
    // cxxforth_main(argc,argv);
    std::cout << "  in ForthVM::evaluate()  ";
    
    // MUST explicit cast c_str() to (char*) else parameter match error !!
    forthvm_main(argc,argv,(char*) command.c_str());
    return string_stack_.empty() ? 0 : string_stack_.size();
}

bool ForthVM::execute(const std::string& command, llama_context* ctx) {
    auto tokens = tokenize(command);
    
    int i;
    std::cout << "  size() " << tokens.size() << " / ";        
    for(i=0;i<tokens.size();i++)
    {
        // printf("%s",argv[i]);
        std::cout << tokens[i] << " / ";
    }
    
    if (tokens[0]=="FORTH") {
      std::cout << "  is FORTH !!  msg_start: ";
      
      // OXW-202604 fake command line as "cxxforth ..." must have argv[0]=cxxforth ??
      // tokens[0]="cxxforth";
      tokens[1]="_/hello.fs";
      // tokens[0]="_/hello.fs"; // leave only 1 token on stack ??
      
      std::cout << "Current working directory: " 
          << std::filesystem::current_path() << std::endl;

/*
# Source - https://stackoverflow.com/a/19082779
# Posted by Dietmar Kühl, modified by community. See post 'Timeline' for change history
# Retrieved 2026-04-20, License - CC BY-SA 3.0
*/

      // std::string array[] = { "s1", "s2" };
      std::vector<char*> vec;
      std::transform(std::begin(tokens), std::end(tokens),
                     std::back_inserter(vec),
                     [](std::string& s){ s.push_back(0); return &s[0]; });
      vec.push_back(nullptr);
      char** carray = vec.data();
           
      // forth(tokens.size(), command.c_str());
      // forth(tokens.size(), carray);
      evaluate(tokens.size(), carray, command);
    }
    
    for (const auto& token : tokens) {
        // Check if token is a registered command
        auto it = handlers_.find(token);
        if (it != handlers_.end()) {
            // Execute handler; abort on failure
            if (!it->second(*this, ctx)) {
            // if (!*(it->second)(*this, ctx)) {
            // if (!(*(it->second))(*this, ctx)) {
            // if (!it->second()) { // bool(*)() can use conditions in return
                std::cerr << "[ForthVM] Error executing command: " << token << std::endl;
                return false;
            }
        } else {
            // Push unknown token as string literal
            string_stack_.push(token);
            // Also try to push as numeric value if possible
            try {
                value_stack_.push(std::stod(token));
            } catch (...) {
                // Not a number; string stack only
            }
        }
    }
    return true;
}

// ============ Built-in Handler Implementations ============

/*
bool ForthVM::cmd_add() {
    if (vm.value_stack_.size() < 2) return false;
    double b = vm.value_stack_.top(); vm.value_stack_.pop();
    double a = vm.value_stack_.top(); vm.value_stack_.pop();
    vm.value_stack_.push(a + b);
    return true;
}
*/ 


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
    if (std::abs(b) < 1e-10) return false;  // avoid division by zero
    vm.value_stack_.push(a / b);
    return true;
}

bool ForthVM::cmd_sym(ForthVM& vm, llama_context*) {
    // sym: expects a symbol name on string stack TOS
    if (vm.string_stack_.empty()) return false;
    std::string sym_name = vm.string_stack_.top(); vm.string_stack_.pop();
    // MVP: just echo; future: integrate with grammar/sampling
    std::cout << "[ForthVM] Registered symbol: " << sym_name << std::endl;
    return true;
}

bool ForthVM::cmd_temp(ForthVM& vm, llama_context* ctx) {
    if (vm.value_stack_.empty()) return false;
    double temp = vm.value_stack_.top(); vm.value_stack_.pop();
    if (ctx && temp >= 0.0 && temp <= 2.0) {
        // Note: llama.cpp doesn't expose per-call temp setter in public API
        // This is a placeholder; real implementation would use sampler chain
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

// ============ Public Getters ============
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

// int cxxforth_main(int argc, const char** argv);
int ForthVM::forth(int argc, char** argv) const {
    cxxforth_main(argc,argv);
    // forthvm_main(argc,argv);
    return string_stack_.empty() ? 0 : string_stack_.size();
}

#include <typeindex>
using namespace std;

std::vector<std::string> PhosVM::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {  // space-delimited, skips whitespace
        tokens.push_back(token);
    }
    return tokens;
}

extern std::stack<std::any> S;

std::string anyToString(const std::any& value) {
    if (!value.has_value()) {
        return "[empty]";
    }

    // Check for exact std::string type
    if (value.type() == typeid(std::string)) {
        return std::any_cast<std::string>(value);
    }
    
    // Check for C-style string literals (const char*)
    if (value.type() == typeid(const char*)) {
        return std::any_cast<const char*>(value);
    }

    // Check for common numeric types
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


// Example:
// > !PHOS common_chat_msg new_msg{role, content}; common_chat_msg 3 find

bool PhosVM::find(int ctx){ // pattern N find
// need .map or iterate function, use pick ?
// stop at n, 0 = whole stack
    try {
          // std::cout << ((typeid(S.top().type()) == typeid(S.top().type())) ? "0" : "1") << " " << ((typeid(S.top().type()) == typeid(std::any)) ? "0" : "1") << "  in find  " << std::any_cast<std::string>(S.top()) << std::endl;           
          
      gettype(ctx); // N
      // string v_type=any_cast<string>(S.top()); S.pop();
      string v_type = anyToString(S.top()); S.pop();
      int n;
      if (v_type=="string")
      {
        // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
        string s0=anyToString(S.top());
        std::cout << "  s0 " << s0 << std::endl;
        n = stoi(s0,nullptr,10); 
        S.pop();
      }
      else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
        n = any_cast<int>(S.top());
        S.pop();
      }
      
      string ptn = anyToString(S.top()); S.pop();

          std::cout.flush();

    // get n then pop
    // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
    // S.pop();

    int D=S.size(); int m,i;
    // for(i=0;i<S.size();i++)
    for(i=0;i<n;i++) // debug
    {
        // printf("%s",argv[i]);
        // if (tokens[1]=="DBG") std::cout << tokens[i] << " / "; // debug mode !PHOS DBG
      
        m = D-i-1;
        S.push(i); 
        pick(ctx);
        string tok = anyToString(S.top());
        cout << tok << " " << i << " find" << endl;
        S.pop(); // pick result
        S.pop(); //
        
        // Search for the substring
        size_t pos = tok.find(ptn);

        // Check if the substring was found
        if (pos != std::string::npos) {
            std::cout << "Found at position: " << pos << "  depth " << S.size() << std::endl;
            //  S.push(i); S.push(pos);
            S.push((int)i); S.push((int)pos);
            return true;
        } else {
            std::cout << "Not found!" << std::endl;
        }

        
        // implement pickn, n=integer ?? or just convert m to string ?
        // if ( std::any_cast<std::type_index>(S.top()) == std::type_index(typeid(int)) ) {
        {
          // std::cout << ((typeid(S.top().type()) == typeid(S.top().type())) ? "0" : "1") << " " << ((typeid(S.top().type()) == typeid(std::any)) ? "0" : "1") << "  in find  " << std::any_cast<std::string>(S.top()) << std::endl;           
          std::cout.flush();
        }
        S.push("FIND");
        // S.push_back(m);
        //tok=tokens[];
        
        /*
        if (handlers.count(tok)) {
            Handler func = handlers[tok];
            // Must use (instance.*pointer)(args)
            (this->*func)(ctx); 
        }
        else S.push(tok);
        */
    }
    }
    catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}    


// unordered_map<string,vector<string>> C = any_cast<unordered_map<string,vector<string>>> m["C"];

// std::unordered_map<std::string, std::any> M = S;
    // vector<vector<string>> C; // colon definition words
// unordered_map<string,vector<string>>& c = any_cast<unordered_map<string,vector<string>>&> m["C"];

// Explicitly cast to a reference of the exact stored type
// auto& c = std::any_cast<std::unordered_map<std::string, std::vector<std::string>>&>(m["C"]);

        // Syntax: ReturnType (ClassName::*)(Args...)
        
        /*
        using Handler = bool (PhosVM::*)(int);
        std::unordered_map<std::string, Handler> handlers = {
            {"ADD", &PhosVM::add},
            {"SUB", &PhosVM::sub}
        };
        */

// unordered_map<string,vector<string>>& c = any_cast<unordered_map<string, any>&>((*P)[0]["C"]);

// unordered_map<string,vector<string>>& c = any_cast<unordered_map<string,vector<string>>&>(m["C"]);

// unordered_map<string,vector<string>>& c = unordered_map<string,vector<string>>(m["C"]);

// unordered_map<string, vector<string>> c = std::any_cast<unordered_map<string, vector<string>>>(m["C"]);

      // vector<string> c;

// bool DBG = true;
extern bool DBG;

// std::streambuf* old_buffer = std::cout.rdbuf(nullptr);
// ostream dcout = cout; // debug cout

// custom debug cout
#define dcout if (DBG) cout
// #define d_cout if (DBG) cout
// #define d_cout if (c_dbg(0)) cout
// #define d_cout if (this->C_DBG) cout
// #define DBG this->C_DBG

#define d_cout if (this->L_DBG > this->W_DBG) cout 
// -- L_DBG output debug level; W_DBG local word debug level;
// -- set W_DBG in execute() before calling word ?

// Source - https://stackoverflow.com/a/48321945
// Posted by user3545770
// Retrieved 2026-06-04, License - CC BY-SA 3.0

// std::string String::join(const std::vector<std::string> &lst, const std::string &delim)
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

// --- colon-definition state machine (file-scope to match existing g_params / S style) ---
static bool phos_compiling = false;
static std::string phos_new_name;
static std::vector<std::string> phos_body;

extern bool X_DBG; // save and restore DBG while setting DBG to local temp value

// -------- colon-definition introspection / maintenance --------

auto PhosVM::words_ptr(int ctx) { // return ptr
    X_DBG=DBG; DBG=true;
    d_cout << "  in words ...\n"; 
    try {
      // pointer to stack, integer indexed -- <stack> cannot be indexed !!
      std::deque<std::any>* P;
      P = (std::deque<std::any>*) &S; 
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);

      // Colon Definition Word CDW
      unordered_map<string,vector<string>> c;    

      // DBG = false;
      if (m.count("C")) d_cout << "m has CDW" << endl;
      else d_cout << "in m CDW not found" << endl; // #define d_cout if (DBG) cout
      
      std::any container = m["C"];
      if (auto ptr = std::any_cast<int>(&container)) { // test for <any> type
          d_cout << "int value is " << *ptr << "\n";
      } 
      // Colon Definition Word CDW
      else if (auto ptr = std::any_cast<unordered_map<string,vector<string>>>(&container)) {
          d_cout << "vector of string: " << (*ptr).count("fruits") << "\n";
          c = *ptr;
          
          // for (const auto& str : any_cast<vector<string>>(m["C"]["fruits"])) {
          if (false) for (const auto& str : c["fruits"]) {
              dcout << str << " ";
          }
  
          for (const auto& [name, body] : *ptr) {
            std::cout << "    " << name << "  ->  ";
            for (const auto& t : body) std::cout << t << " ";
            std::cout << std::endl;
          }
          
          return ptr;
          
      }
      else d_cout << "CDW not found" << endl;

      if (false) {    
        std::deque<std::any>* P = (std::deque<std::any>*) &S;
        auto& m = any_cast<unordered_map<string,any>&>((*P)[0]);
        auto& c = any_cast<unordered_map<string,vector<string>>&>(m["C"]);
        std::cout << "  dictionary (" << c.size() << " words):" << std::endl;
      }
      
      // }
    } catch (std::exception& e) {
        std::cerr << "  WORDS exception: " << e.what() << std::endl;
        // return false;
    }
    DBG=X_DBG;
    // return true;
}

bool PhosVM::words(int ctx) {
    X_DBG=DBG; DBG=true;
    d_cout << "  in words ...\n"; 
    try {
          
      auto ptr = std::any_cast<unordered_map<string,vector<string>>>(words_ptr(ctx));
      for (const auto& [name, body] : ptr) {
        std::cout << "    " << name << "  ->  ";
        for (const auto& t : body) std::cout << t << " ";
        std::cout << std::endl;
      }
      
      if (false) {    
        std::deque<std::any>* P = (std::deque<std::any>*) &S;
        auto& m = any_cast<unordered_map<string,any>&>((*P)[0]);
        auto& c = any_cast<unordered_map<string,vector<string>>&>(m["C"]);
        std::cout << "  dictionary (" << c.size() << " words):" << std::endl;
      }
      
      // }
    } catch (std::exception& e) {
        std::cerr << "  WORDS exception: " << e.what() << std::endl;
        return false;
    }
    DBG=X_DBG;
    return true;
}

bool PhosVM::see(int ctx) {
    if (S.empty()) { std::cerr << "  SEE: empty stack" << std::endl; return false; }
    try {
        std::string name = any_cast<string>(S.top()); S.pop();
        std::deque<std::any>* P = (std::deque<std::any>*) &S;
        auto& m = any_cast<unordered_map<string,any>&>((*P)[0]);
        auto& c = any_cast<unordered_map<string,vector<string>>&>(m["C"]);
        auto it = c.find(name);
        if (it == c.end()) {
            std::cerr << "  SEE: '" << name << "' not in dictionary" << std::endl;
            return false;
        }
        std::cout << ": " << name << "  ";
        for (const auto& t : it->second) std::cout << t << " ";
        std::cout << ";" << std::endl;
    } catch (std::exception& e) {
        std::cerr << "  SEE exception: " << e.what() << std::endl;
        return false;
    }
    return true;
}

bool PhosVM::forget(int ctx) {
    if (S.empty()) { std::cerr << "  FORGET: empty stack" << std::endl; return false; }
    try {
        std::string name = any_cast<string>(S.top()); S.pop();
        std::deque<std::any>* P = (std::deque<std::any>*) &S;
        auto& m = any_cast<unordered_map<string,any>&>((*P)[0]);
        auto& c = any_cast<unordered_map<string,vector<string>>&>(m["C"]);
        auto erased = c.erase(name);
        std::cout << "  FORGET " << name << " "
                  << (erased ? "ok" : "(not found)") << std::endl;
    } catch (std::exception& e) {
        std::cerr << "  FORGET exception: " << e.what() << std::endl;
        return false;
    }
    return true;
}

/*            
    // 1. Create your unordered_map
    std::unordered_map<std::string, int> age_map = {
        {"Alice", 25},
        {"Bob", 30},
        {"Charlie", 22}
    };
    // 2. Direct assignment converts it to a JSON object
    json json_output = age_map;
    // 3. Print or serialize to string
    std::cout << json_output.dump(4) << std::endl; // '4' 

    std::unordered_map<std::string, std::vector<std::string>> my_map = {
        {"fruits", {"apple", "banana", "cherry"}},
        {"vegetables", {"carrot", "broccoli"}}
    };
    // 2. Convert map directly to a json object
    json json_object = my_map;
    // 3. Serialize to a string (argument 4 enables pretty-printing with spaces)
    std::string json_string = json_object.dump(4);
    // 4. Output the result
    std::cout << json_string << std::endl;

            // nlohmann::json json_obj = std::any_cast<nlohmann::json>(S.top()); S.pop();
            // nlohmann::json json_obj = std::any_cast<nlohmann::json>(c); S.pop();
            // nlohmann::json json_obj = std::any_cast<nlohmann::json>(c["fruits"]); S.pop();
            // nlohmann::json json_obj = (c["fruits"]); S.pop();
*/

#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool PhosVM::drop(int ctx) { std::cout << "drop ...\n";
    if (S.empty()) {
        std::cerr << "  drop: stack underflow" << std::endl;
        return false;
    }
    std::any topElement = S.top();
    S.pop();
    return true;
}

bool PhosVM::swap(int ctx) {
    if (S.size() < 2) return false;
    std::any a = S.top(); S.pop();
    std::any b = S.top(); S.pop();
    S.push(a);
    S.push(b);
    return true;
}

bool PhosVM::over(int ctx) {
    if (S.size() < 2) {
        return false;
    }
    std::any top = S.top();
    S.pop();
    std::any second = S.top();
    S.push(top);
    S.push(second);
    return true;
}

bool PhosVM::equal(int ctx) { std::cout << "equal (=) ...\n";
    if (S.size() < 2) return false;
    
    std::any val2 = S.top(); S.pop();
    std::any val1 = S.top(); S.pop();
    
    // Prevent execution failure by verifying types match before direct comparison
    if (val1.type() != val2.type()) {
        S.push(false); // Structural mismatch defaults to false
        return true;
    }    
    try {
        if (val1.type() == typeid(int)) {
            S.push(std::any_cast<int>(val1) == std::any_cast<int>(val2));
        } else if (val1.type() == typeid(std::string)) {
            S.push(std::any_cast<std::string>(val1) == std::any_cast<std::string>(val2));
        } else if (val1.type() == typeid(bool)) {
            S.push(std::any_cast<bool>(val1) == std::any_cast<bool>(val2));
        } else {
            return false; // Unsupported polymorphic comparison
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool PhosVM::add_auto(int ctx) {
    if (S.size() < 2) return false;
    auto a = std::any_cast<int>(S.top()); S.pop();
    auto b = std::any_cast<int>(S.top()); S.pop();
    S.push(a + b);
    return true;
}

bool PhosVM::multiply(int ctx) {
    if (S.size() < 2) return false;
    auto a = std::any_cast<float>(S.top()); S.pop();
    auto b = std::any_cast<float>(S.top()); S.pop();
    S.push(a * b);
    return true;
}

bool PhosVM::json_get(int ctx) {
    std::cout << "json_get (M[C]) ...\n";
    if (S.size() < 2) return false;
    try {
        std::string key = std::any_cast<std::string>(S.top()); S.pop();
        nlohmann::json json_obj = std::any_cast<nlohmann::json>(S.top()); S.pop();
        
        if (json_obj.contains(key)) {
            nlohmann::json value = json_obj[key];
            
            // Push the value back as the appropriate C++ type
            if (value.is_string()) S.push(value.get<std::string>());
            else if (value.is_number_integer()) S.push(value.get<int>());
            else if (value.is_number_float()) S.push(value.get<float>());
            else if (value.is_boolean()) S.push(value.get<bool>());
            else S.push(value); // Fallback to nested json object/array
        } else {
            std::cerr << "[LLASMA VERIFY FAILED] json_get: Key '" << key << "' not found in JSON.\n";
            return false; // Halts VM if LLM hallucinates a non-existent key
        }
    } catch (const std::exception& e) {
        std::cerr << "[LLASMA VERIFY FAILED] json_get: " << e.what() << "\n";
        return false;
    }
    return true;
}

// just add and compile, no test to speed up.
bool PhosVM::string_to_json(int ctx) {
    std::cout << "string_to_json ...\n";
    if (S.empty()) return false;
    try {
        std::string json_str = std::any_cast<std::string>(S.top()); S.pop();
        nlohmann::json json_obj = nlohmann::json::parse(json_str);
        S.push(json_obj);
    } catch (const std::bad_any_cast& e) {
        std::cerr << "[LLASMA VERIFY FAILED] string_to_json: Expected string on stack. " << e.what() << "\n";
        return false;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[LLASMA VERIFY FAILED] string_to_json: LLM generated invalid JSON. " << e.what() << "\n";
        return false; // Crucial: Catches LLM JSON hallucinations
    }
    return true;
}

bool PhosVM::json_to_string(int ctx) {
    std::cout << "\njson_to_string ...\n";
    X_DBG=DBG; DBG=true;
    if (S.empty()) return false;
    try {
        std::any container = S.top(); // any c;
        if (auto ptr = std::any_cast<int>(&container)) {
            dcout << "int value is " << *ptr << "\n";
        }
        else if (auto ptr = std::any_cast<unordered_map<string,vector<string>>>(&container)) {
            d_cout << "vector of string: " << (*ptr).count("fruits") << "\n";
            // c = any_cast<unordered_map<string,vector<string>>>(*ptr);
            auto c = *ptr;
            if (false) for (const auto& str : c["fruits"]) {
                dcout << str << " ";
            }
            nlohmann::json json_obj = c; S.pop();
            S.push(json_obj.dump()); // .dump() converts nlohmann::json to std::string
            d_cout << json_obj.dump() << endl;            
        }
        else d_cout << "CDW not found" << endl;    
    } catch (const std::bad_any_cast& e) {
        std::cerr << "[LLASMA VERIFY FAILED] json_to_string: Expected json object on stack. " << e.what() << "\n";
        return false;
    }
    DBG=X_DBG;
    return true;
}

#include <fstream>
bool PhosVM::write_file(int ctx) {
    std::cout << "write_file ...\n";
    if (S.size() < 2) return false;
    try {
        // Note FORTH stack order: filepath pushed first, contents pushed second.
        // So we pop contents first, then filepath.
        std::string filepath = std::any_cast<std::string>(S.top()); S.pop();
        std::string contents = std::any_cast<std::string>(S.top()); S.pop();
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[LLASMA VERIFY FAILED] write_file: Cannot open file for writing " << filepath << "\n";
            return false;
        }
        file << contents;
    } catch (const std::exception& e) {
        std::cerr << "[LLASMA VERIFY FAILED] write_file: " << e.what() << "\n";
        return false;
    }
    return true;
}

bool PhosVM::read_file(int ctx) {
    std::cout << "read_file ...\n";
    if (S.empty()) return false;
    try {
        std::string filepath = std::any_cast<std::string>(S.top()); S.pop();
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[LLASMA VERIFY FAILED] read_file: Cannot open file " << filepath << "\n";
            return false; // Prevents crashes from hallucinated file paths
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        S.push(content);
    } catch (const std::exception& e) {
        std::cerr << "[LLASMA VERIFY FAILED] read_file: " << e.what() << "\n";
        return false;
    }
    return true;
}

bool PhosVM::C_DBG=false;
int  PhosVM::D_CTR=0;
int  PhosVM::W_DBG=0; // word debug level
int  PhosVM::L_DBG=0; // run time debug level, if (L_DBG>W_DBG) cout ...

bool PhosVM::c_dbg(int ctx) { 
    // std::cout << "c_dbg -- debug counter ...\n";
    if ((this->D_CTR)>0) {
      this->D_CTR--;
      return true;
    }
    else return false;
}

bool PhosVM::s_ctr(int ctx) { 
    std::cout << "s_ctr -- set D_CTR ...\n";
    if (S.empty()) return false;
    try {
        std::string str = std::any_cast<std::string>(S.top()); S.pop();
        this->D_CTR = stoi(str);
    } catch (const std::exception& e) {
        std::cerr << "[LLASMA VERIFY FAILED] dbg " << e.what() << "\n";
        return false;
    }
    return true;
}

bool PhosVM::l_dbg(int ctx) { 
    std::cout << "l_dbg -- set L_DBG debug level ...\n";
    if (S.empty()) return false;
    try {
        std::string str = std::any_cast<std::string>(S.top()); S.pop();
        this->L_DBG = stoi(str);
    } catch (const std::exception& e) {
        std::cerr << "[LLASMA VERIFY FAILED] dbg " << e.what() << "\n";
        return false;
    }
    return true;
}

bool PhosVM::s_dbg(int ctx) { 
    std::cout << "s_dbg -- set DBG flag ...\n";
    if (S.empty()) return false;
    try {
        std::string str = std::any_cast<std::string>(S.top()); S.pop();
        
        // if (str=="1" || str=="true") DBG=true;
        // else DBG=false;
        if (str=="1" || str=="true") this->C_DBG=true;
        else this->C_DBG=false;        
    } catch (const std::exception& e) {
        std::cerr << "[LLASMA VERIFY FAILED] dbg " << e.what() << "\n";
        return false;
    }
    return true;
}

bool PhosVM::g_dbg(int ctx) { 
    // std::cout << "g_dbg -- get DBG flag ...\n";
    d_cout << "g_dbg -- get DBG flag ...\n";
    if (S.empty()) return false;
    try {
        // cout << "DBG is " << DBG << endl;
        cout << "DBG is " << this->C_DBG << endl;
    } catch (const std::exception& e) {
        std::cerr << "[LLASMA VERIFY FAILED] dbg " << e.what() << "\n";
        return false;
    }
    return true;
}

void PhosVM::execute_minimax(std::string cmd, int ctx) {

      this->W_DBG=1; // word debug level for exectute() itself !!
      X_DBG=DBG; // DBG=true;
      d_cout << "\n\x1b[32m  !PHMM start \x1b[0m" << endl; // new convention x_* 
      d_cout << "  in execute ...\n"; // first line of output use newline or no indent, subsequent line use indent, then easy to spot first line in output

      std::deque<std::any>* P;
      P = (std::deque<std::any>*) &S;
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
      unordered_map<string,vector<string>> c;
      
      // DBG = false;
      if (m.count("C")) dcout << "m has CDW" << endl;
      else d_cout << "in m CDW not found" << endl;
      
      std::any container = m["C"];
      if (auto ptr = std::any_cast<int>(&container)) {
          dcout << "int value is " << *ptr << "\n";
      }
      else if (auto ptr = std::any_cast<unordered_map<string,vector<string>>>(&container)) {
          d_cout << "vector of string: " << (*ptr).count("fruits") << "\n";
          c = *ptr;
          for (const auto& str : c["fruits"]) {
              dcout << str << " ";
          }
      }
      else d_cout << "CDW not found" << endl;
      
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
          d_cout << "  m[\"DBG\"] value is " << *ptr << "\n";
      }
    auto tokens = tokenize(cmd);
    int i; std::string tok;
    if (tokens.size() >= 2 && tokens[1]=="DBG") std::cout << "  size() " << tokens.size() << " / ";
    
          d_cout << "\n\nc[fruits] ";  
          for (const auto& str : c["fruits"]) {
              d_cout << str << " ";
          }
    
    auto& w = any_cast<unordered_map<string,vector<string>>&>(m["C"]);
    
    for(i=0;i<(int)tokens.size();i++)
    {
        if (tokens.size() >= 2 && tokens[1]=="DBG") std::cout << tokens[i] << " / ";
        tok=tokens[i];
        // ----- compile-mode: buffer tokens verbatim until ';' -----
        if (phos_compiling) {
            if (tok == ";") {
                if (phos_new_name.empty() || phos_body.empty()) {
                    std::cerr << "  [PhosVM] ': " << phos_new_name
                              << " ... ;' rejected: empty name or empty body" << std::endl;
                    phos_compiling = false;
                    phos_new_name.clear();
                    phos_body.clear();
                    continue;
                }
                c[phos_new_name] = phos_body;
                dcout << "  defined: " << phos_new_name << "  ->  ";
                for (const auto& t : phos_body) dcout << t << " ";
                dcout << std::endl;
          

          d_cout << "\n\n625 c["<< phos_new_name << "] ";  
          for (const auto& str : c[phos_new_name]) {
              d_cout << str << " ";
          }

          if (true) {
            w[phos_new_name] = phos_body;                
            d_cout << "\n\n630 w["<< phos_new_name << "] ";  
            for (const auto& str : w[phos_new_name]) {
                d_cout << str << " ";
            }
          }
                phos_compiling = false;
                phos_new_name.clear();
                phos_body.clear();
                continue;
            }
            if (tok == ":") {
                std::cerr << "  [PhosVM] nested ':' forbidden while compiling "
                          << phos_new_name << std::endl;
                phos_compiling = false;
                phos_new_name.clear();
                phos_body.clear();
                continue;
            }
            phos_body.push_back(tok);
            continue;
        }
        // ----- interactive 'NAME NAME body ;' -----
        if (tok == ":") {
            if (i + 1 >= (int)tokens.size()) {
                std::cerr << "  [PhosVM] ':' without a name at end of input" << std::endl;
                continue;
            }
            phos_new_name = tokens[++i];
            phos_body.clear();
            phos_compiling = true;
            dcout << "  compiling: " << phos_new_name << " ..." << std::endl;
            continue;
        }
        // ----- normal dispatch -----
        if (c.count(tok)) {
            for (const auto& str : c[tok]) {
                std::cout << str << " ";
            }
            execute(join(c[tok], " "),ctx);
            dcout << "AFTER colon def word" << endl;
        } else
        if (handlers.count(tok)) {
            if (tok=="s_M") this->W_DBG=1; // if (L_DBG>W_DBG) cout ...
            else this->W_DBG=0;
            Handler func = handlers[tok];
            (this->*func)(ctx);
        }
        else S.push(tok);
        std::cout.flush();
    }
    DBG = true;
    if (tokens[0]=="PHOS") {
        d_cout << "\n  end PHOS !!  msg_start: ";
        if (handlers.count(cmd)) {
            Handler func = handlers[cmd];
            (this->*func)(ctx);
        }
    }
    DBG=X_DBG; // should not restore DBG in execute()
}


// void PhosVM::execute_20260708(std::string cmd, int ctx) {
void PhosVM::execute(std::string cmd, int ctx) {

      this->W_DBG=1; // word debug level for exectute() itself !!
      d_cout << "  execute X_DBG " << X_DBG << "  DBG " << DBG << endl;
      X_DBG=DBG; // DBG=true; // this causes DBG ON for subsequent calls
      d_cout << "\n\x1b[32m  !PHOS start \x1b[0m" << endl; // new convention x_* 
      d_cout << "  in execute ...\n"; 

      // pointer to stack, integer indexed -- <stack> cannot be indexed !!
      std::deque<std::any>* P;
      P = (std::deque<std::any>*) &S; 
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);

      // Colon Definition Word CDW
      unordered_map<string,vector<string>> c;
            
      DBG = false;
      if (m.count("C")) d_cout << "m has CDW" << endl;
      else d_cout << "in m CDW not found" << endl; // #define d_cout if (DBG) cout
      
      std::any container = m["C"];
      if (auto ptr = std::any_cast<int>(&container)) { // test for <any> type
          d_cout << "int value is " << *ptr << "\n";
      } 
      // Colon Definition Word CDW
      else if (auto ptr = std::any_cast<unordered_map<string,vector<string>>>(&container)) {
          d_cout << "vector of string: " << (*ptr).count("fruits") << "\n";
          c = *ptr;
          
          // for (const auto& str : any_cast<vector<string>>(m["C"]["fruits"])) {
          for (const auto& str : c["fruits"]) {
              dcout << str << " ";
          }
      }
      else d_cout << "CDW not found" << endl;

      container = m["LR"]; // recursion level
      // auto pct = &m["RL"];
      if (auto ptr = std::any_cast<int>(&container)) {
          d_cout << "  LR value is " << *ptr << "\n";
      } 

      container = m["LS"]; // recursion level
      // auto pct = &m["RL"];
      if (auto ptr = std::any_cast<int>(&container)) {
          d_cout << "  LS value is " << *ptr << "\n";
      }   // if (MORE_FLAGS) dcout ??
      // DBG=true; dcout ....
      // word specific DBG ; turn on at start, turn off at end ; or at execute ?
      // : DBGY true  t_bool DBG s_M ; 
      // : DBGN false t_bool DBG s_M ; // use DBGY before words and DBGN after words !!
      // turn color on and off using escape code ??
      container = m["DBG"]; // recursion level
      // auto pct = &m["RL"];
      if (auto ptr = std::any_cast<bool>(&container)) {
          d_cout << "  m[\"DBG\"] value is " << *ptr << "\n";
      } 

    // DBG = true; // execute tok show DBG messages
    // auto tokens = tokenize(command);
    auto tokens = tokenize(cmd);
    
    int i; std::string tok;
    if (tokens[1]=="DBG") std::cout << "  size() " << tokens.size() << " / ";        
    
    for(i=0;i<tokens.size();i++)
    {
        // !PHOS DBG -- debug mode
        // printf("%s",argv[i]);
        if (tokens[1]=="DBG") std::cout << tokens[i] << " / "; // debug mode !PHOS DBG
        tok=tokens[i];
        dcout << "tok is " << tok << endl;

        // execute Colon Definition Word
        if (c.count(tok)) {
            for (const auto& str : c[tok]) {
                std::cout << str << " ";
            }
            execute(join(c[tok], " "),ctx);
            dcout << "AFTER colon def word" << endl;
        } else 
        if (handlers.count(tok)) {
            if (tok=="s_M") this->W_DBG=1; // if (L_DBG>W_DBG) cout ...
            else this->W_DBG=0;        
            dcout << "  in handlers tok is " << tok << endl;
            Handler func = handlers[tok];
            // Must use (instance.*pointer)(args)
            (this->*func)(ctx); // execute PhosVM::FUNCNAME()
        }
        else S.push(tok);

        // test S.top().type()        
        // std::cout << ((typeid(S.top().type()) == typeid(S.top().type())) ? "0" : "1") << " " << ((typeid(S.top().type()) == typeid(std::any)) ? "0" : "1") << " " << std::any_cast<std::string>(S.top()) << std::endl;           
        std::cout.flush();

    }
    
    DBG = true; // flag for dcout
    if (tokens[0]=="PHOS") {
        dcout << "\n\x1b[32m  end PHOS !!  msg_start 2x newline:\x1b[0m\n\n";
        // printf("\x1b[32m[Forth] Result: %.4g\x1b[0m\n", forth_vm.top_v
                                
        if (handlers.count(cmd)) {
            Handler func = handlers[cmd];
            // Must use (instance.*pointer)(args)
            (this->*func)(ctx); 
        }
    }
    DBG = X_DBG; // false;
}

// if (DBG) dcout = cout;
      // else old_buffer = std::dcout.rdbuf(nullptr); // std::dcout.rdbuf(old_buffer);      
      // #define dcout if (DBG) cout

bool PhosVM::dup(int ctx) { std::cout << "dup ...\n"; 
    if (S.empty()) return false; // Handle empty stack
    std::any topElement = S.top(); // Get the top element
    S.push(topElement);            // Push it back
    // S.push(S.end()); 
    return true; 
}

bool PhosVM::peekr(int ctx) { std::cout << "m n peekr ...\n"; // type check peek range 
    std::cout.flush();

    // A: any --> int
    // B: any --> string
    // cannot be independent function, because do not know type to return !!
    // use ternary: "int" ? do_int : do_string ; // ??
    // nested ternary: "int" ? do_int : "string" ? do_string : others ; 
    
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
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
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
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
      m = any_cast<int>(S.top());
      S.pop();
    }
    
    int i;
    for(i=m; i<n; i++) {
      
      S.push(i); pick(ctx);
      // S.push(i); peek(ctx);
        
      // cannot use peek, because it does not copy and push item on stack  
      // the gettype input is i !! on n-th item on stack
      gettype(ctx); 
      v_type=anyToString(S.top()); S.pop();
      int m;
      if (v_type=="string")
      {
        string s0=anyToString(S.top());
        std::cout << "  s0 " << s0 << std::endl;
        // m = stoi(s0,nullptr,10); 
        S.pop();
      }
      else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
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

    // std::cout << "\nstart depth " << S.size() << endl; 
    if (S.empty()) return false; // Handle empty stack
    // S.push(S.size());
    
    int b=S.size();
    S.push(b); // S.push(S.size()); is DIFFERENT !! first stored as int var !! type_index works !!

    std::type_index my_type = std::type_index(S.top().type());
    // (*P)[n].type()
    if (compareTypeIndexWithString(my_type, "std::string")) {
      d_cout << "type_index is std::string ";
    }
    else if (compareTypeIndexWithString(my_type, "int")) {
        d_cout << "type_index is int ";
    }
    else d_cout << "type_index " << type_index(S.top().type()).name() << " ";

    // S.push(std::to_string(b));
    std::cout << "depth " << S.size() << endl; 
    
    // if ( std::any_cast<std::type_index>(S.top()) == std::type_index(typeid(int)) ) 
    {
    // if (a==typeid(int)) {
    //    S.pop();
    //    int b = std::any_cast<int>(S.top()); S.pop();
    //    S.push(std::to_string(b));
    }

    return true; 
}

    // LSM-2026050 depth() REMOVED
    // S.push(std::to_string(b));    
    // typeid does not work, use type_index to check type first before cast!!
    // dummy check after push with type_index !!    
    /*
    std::type_index my_type = std::type_index(typeid(int));
    if (compareTypeIndexWithString(my_type, "int")) {
        std::cout << "The type_index is an int!\n";
    }
    my_type = std::type_index(typeid(std::string));
    if (compareTypeIndexWithString(my_type, "std::string")) {
      std::cout << "The type_index is an std::string!\n";
    }
    */
    
// gettype SHOULD NOT POP !!    
bool PhosVM::gettype(int ctx) { // comment here
    // S.push(S.pop().type()); S.pop() returns void ??
    // std::any topElement = S.top(); // Get the top element
    // auto top_type = S.top().type(); 
    // const std::type_info& top_type = S.top().type();
    // std::any a = std::type_index(typeid(int));
    // std::any a = std::type_index(S.top().type());
    
    std::type_index my_type = std::type_index(S.top().type());
    // (*P)[n].type()
    if (compareTypeIndexWithString(my_type, "std::string")) {
      d_cout << "type_index is std::string! ";
      // S.pop(); 
      S.push("string");
    }
    else if (compareTypeIndexWithString(my_type, "int")) {
      d_cout << "type_index is int! ";
      // S.pop(); 
      S.push("int");
    }
    else {
      std::cout << "type_index " << type_index(S.top().type()).name() << " ";      
      // S.pop(); 
      S.push(my_type);
    }
    
    // S.push(std::to_string(b));
    // std::cout << "gettype depth " << S.size() << " " << any_cast<string>(S.top()) << endl; 
    // std::cout << "gettype depth " << S.size() << " " << std::any_cast<std::string>(S.top()) << endl; 

    // S.pop();
    // S.push(topElement.type());
    // S.push(top_type);
    // S.push(a);
    
    return true; 
}


// not working. leave as notes.
bool PhosVM::gettype_any(int ctx) { // comment here
    // S.push(S.pop().type()); S.pop() returns void ??
    // std::any topElement = S.top(); // Get the top element
    // auto top_type = S.top().type(); 
    // const std::type_info& top_type = S.top().type();
    // std::any a = std::type_index(typeid(int));
    std::any a = std::type_index(S.top().type());
    S.pop();
    // S.push(topElement.type());
    // S.push(top_type);
    S.push(a);
    return true; 
}
bool PhosVM::tostr(int ctx) { 
    std::cout << "  tostr ...\n"; 
    // std::any a = S.top(); S.pop();
    // std::type_index a = std::any_cast<std::type_index> S.top();
    // if (S.top()==std::type_index(typeid(int))) {
    
    if ( std::any_cast<std::type_index>(S.top()) == std::type_index(typeid(int)) ) {
    // if (a==typeid(int)) {
        S.pop();
        int b = std::any_cast<int>(S.top()); S.pop();
        S.push(std::to_string(b));
    }
    return true; 
}
bool PhosVM::type(int ctx){ // output string
    std::cout << std::any_cast<std::string>(S.top()) ; S.pop();
    return true; 
}
bool PhosVM::cr(int ctx){ // carriage return = new line
    std::cout << std::endl ; 
    return true; 
}
bool PhosVM::showstack(int ctx){

// Source - https://stackoverflow.com/a/8480675
// Posted by nsanders, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-04, License - CC BY-SA 4.0

try {
//    compare( -1, 3 );
        std::deque<std::any>* P;
    P = (std::deque<std::any>*) &S; // stack pointer
            std::cout << "  P = &S ... OK  " << std::endl; // line by line msg until catch
            std::cout << S.top().type().name() << std::endl;
    // int n = std::any_cast<int>(S.top()); 

    // get n then pop
    int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
    S.pop();
    
    std::cout << "  n is " << n << std::endl;
    std::cout << (*P)[n].type().name() << std::endl;
    // if (typeid((*P)[n].type().name())==typeid(std::string)) 
    if (typeid((*P)[n].type())==typeid(std::string)) 
        // std::cout << (*P)[n].type().name() << std::endl;
        std::cout << "  showstack ... is string  " << std::endl;
    else std::cout << "  is not string  " << std::endl;
    std::cout << std::any_cast<std::string>((*P)[n]) << std::endl;
}
// catch( const std::invalid_argument& e ) {
/* catch( ... ) {
    // do stuff with exception... 
} */
// Source - https://stackoverflow.com/a/42004384
// Posted by serup, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-04, License - CC BY-SA 3.0
catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;
}

// not working any_cast version
bool PhosVM::peek_any(int ctx){ // FORTH has no peek
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
        std::cout << "  P = &S ... OK  " << std::endl; // line by line msg until catch
        std::cout << S.top().type().name() << std::endl;

    int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
    S.pop();
    std::cout << "  depth " << S.size() << std::endl; // line by line msg until catch
    std::cout << "  n is " << n << std::endl;
    std::cout << (*P)[n].type().name() << std::endl;
    // if (typeid((*P)[n].type().name())==typeid(std::string)) 
    if (typeid((*P)[n].type())==typeid(std::string)) 
        // std::cout << (*P)[n].type().name() << std::endl;
        std::cout << "  showstack ... is string  " << std::endl;
    else std::cout << "  is not string  " << std::endl;
    std::cout << std::any_cast<std::string>((*P)[n]) << std::endl;

} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;

}

#include <concepts>
// C++20 ??
/*
void printStringVector(const std::same_as<std::vector<std::string>> auto& vec) {
    for (const auto& str : vec) {
        std::cout << str << " ";
    }
    std::cout << "\n";
}
*/

#include <type_traits>
template <typename T>
void processData(const T& data) {
    if constexpr (std::is_same_v<T, std::vector<std::string>>) {
        std::cout << "Processing a vector of strings.\n";
    } else {
        std::cout << "Processing a different type.\n";
    }
}

bool PhosVM::peek(int ctx){ // FORTH has no peek
  try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
        // std::cout << "  P = &S ... OK  " << std::endl; // line by line msg until catch

    gettype(ctx);
    // string v_type=any_cast<string>(S.top()); S.pop();
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10); 
      S.pop();
    }
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
      n = any_cast<int>(S.top());
      S.pop();
    }

      std::cout << "  depth " << S.size(); // line by line msg until catch
      std::cout << "  n " << n;
      // std::cout << (*P)[n].type().name() << std::endl;
      // std::cout << "type_index " << type_index((*P)[n].type()).name() << std::endl;

      n = S.size() - n - 1; // depth - n - 1 // get depth after pop n      
      std::any container = (*P)[n]; // S.top();
      if (auto ptr = std::any_cast<int>(&container)) {
          std::cout << "Success! Value is " << *ptr << "\n";
      }
      else if (auto ptr = std::any_cast<string>(&container)) {
          std::cout << "Success! ptr  ";
          std::cout << *ptr << " ";
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
      std::cout.flush();
    
      // peek output
      // std::cout << "  peek " << std::any_cast<std::string>((*P)[n]) << std::endl;
  } catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
  return true;
}

      /*
      my_type = std::type_index((*P)[n].type());
      // (*P)[n].type()
      if (compareTypeIndexWithString(my_type, "std::string")) {
        std::cout << "The type_index is an std::string!\n";
      }
      */

// need to word to read M at S[0]
//     std::unordered_map<std::string, std::any> M;
//     unordered_map<string, any> M;
// can use pick ??


// LSM-20260602 sample input
// > !PHOS 123 abc s_M 456 pqr s_M k_M

// LSM-20260602 k_M should output array? then new WORD to search array if exist, then g_M ?
// M can store type of var on stack
bool PhosVM::k_M(int ctx){ // k_M list all keys in M
// S[0] = M, super buffer variable (PHP super global ?)
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
    
    // get stack items ... keep for future
    // string key=anyToString(S.top()); S.pop();

    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    std::cout << "  k_M m DEFINED   ";
    
    vector<string> keys;
    
    // Print using structured bindings
    for (const auto& [key, value] : m) {
        // std::cout << key << ": " << value << "\n";
        std::cout << key << " ";
        keys.push_back(key);
    }
 
    S.push(keys);

    std::cout << "\ndepth " << S.size() << "  read test  ";    
    vector<string> V0=any_cast<vector<string>>(S.top());
    for (const auto& key : V0) {
        // std::cout << key << ": " << value << "\n";
        std::cout << key << " ";
        // keys.push_back(key);
    }
    
    processData(V0);
    
    // std::any container = 42;
    std::any container = S.top();

    // Returns a pointer to the value, or nullptr on mismatch
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

    // S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}

bool PhosVM::pvs(int ctx){ // k_M list all keys in M
// S[0] = M, super buffer variable (PHP super global ?)
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer

        std::any container = S.top();

    // Returns a pointer to the value, or nullptr on mismatch
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

    // S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}

bool PhosVM::inV(int ctx){ // k_M list all keys in M
// S[0] = M, super buffer variable (PHP super global ?)
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
     
    // std::any container = 42;
    // std::any container = S.top();
    std::any container;
    std::string target = "banana";
    
    // out of order input, load on type match
    // auto ptr;
    // Returns a pointer to the value, or nullptr on mismatch
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
          // input is string
          target = *ptr;      
      }
      else {
          std::cout << "Type mismatch or container is empty.\n";
      }
    }
    
    // for (const auto& key : any_cast<vector<string>>(&container)) {        
    for (const auto& key : any_cast<vector<string>>(container)) {        
          std::cout << key << " ";
    }
    
    std::vector<std::string> fruits = {"apple", "banana", "cherry"};
    // std::string target = "banana";
    
    // Search for the exact string match
    // auto it = std::find(fruits.begin(), fruits.end(), target);

    vector<string> vec = any_cast<vector<string>>(container);
    auto it = std::find(vec.begin(), vec.end(), target);

    if (it != vec.end()) {
        // Calculate the index position by subtracting the base iterator
        int index = std::distance(vec.begin(), it);
        std::cout << target << " found at index: " << index << std::endl;
    } else {
        std::cout << target << " not found." << std::endl;
    }

    // S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}

bool PhosVM::g_M(int ctx){ // g_M get M modern convention
// S[0] = M, super buffer variable (PHP super global ?)
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
    // LSM-20260530 check typeid *P[n] below ....
    // if (typeid(S.top().type())==typeid(std::string)) 
    
    string key=anyToString(S.top()); S.pop();

    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    d_cout << "  g_M m DEFINED " << endl;

    // M["test"]=(int) 999;
    // (*pM)["test"]=(int) n;
    S.push(m[key]);
  
    std::any container = m[key];
    if (auto ptr = std::any_cast<int>(&container)) {
        std::cout << "int value is " << *ptr << "\n";
    } 
    else if (auto ptr = std::any_cast<bool>(&container)) {
        std::cout << "bool value is " << *ptr << "\n";
    } 
    
/*    gettype(ctx);
    // string v_type=any_cast<string>(S.top()); S.pop();
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      // n = stoi(s0,nullptr,10); 
      m[key]=s0;
      
      std::cout << "  g_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<string>(m[key]) << std::endl;

      S.pop();
    }
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
      n = any_cast<int>(S.top());
      m[key]=(int) n;
      S.pop();
      
      std::cout << "  g_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m[key]) << std::endl;

    }
    
    std::cout << "  g_M m ASSIGNED " << endl;
*/    
    // S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;
}

// LSM-20260603 deprecated
bool PhosVM::g_M_gettype(int ctx){ // g_M get M modern convention
// S[0] = M, super buffer variable (PHP super global ?)
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
    // LSM-20260530 check typeid *P[n] below ....
    // if (typeid(S.top().type())==typeid(std::string)) 
    
    string key=anyToString(S.top()); S.pop();

    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    std::cout << "  g_M m DEFINED " << endl;

    // M["test"]=(int) 999;
    // (*pM)["test"]=(int) n;
    S.push(m[key]);
    
    gettype(ctx);
    // string v_type=any_cast<string>(S.top()); S.pop();
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      // n = stoi(s0,nullptr,10); 
      m[key]=s0;
      
      std::cout << "  g_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<string>(m[key]) << std::endl;

      S.pop();
    }
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
      n = any_cast<int>(S.top());
      m[key]=(int) n;
      S.pop();
      
      std::cout << "  g_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m[key]) << std::endl;

    }
    
    std::cout << "  g_M m ASSIGNED " << endl;
    
    // S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;
}

// bool PhosVM::push(int ctx, std::any obj) {
bool PhosVM::push(int ctx) {
    // S.push(obj);
    return true;
}    

// S[0] = M, super buffer variable (PHP super global ?)
bool PhosVM::s_M(int ctx){ // s_M set M modern convention
try {        
    d_cout << "in s_M  L_DBG " << this->L_DBG << "  W_DBG " << W_DBG << endl;

    std::deque<std::any>* P;
    P = (std::deque<std::any>*) &S; 

    // first argument on stack is key
    string key=anyToString(S.top()); S.pop();

    unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
    
    X_DBG=DBG; // DBG=true;
    d_cout << "\n\x1b[32ms_M m DEFINED \x1b[0m" << endl; // new convention x_* 

    // conditional block to test type of next argument on stack
    std::any container = S.top(); 
    if (auto ptr = std::any_cast<struct llama_model*>(&container)) {
        d_cout << "llama_model key is " << key <<"\n";
        m[key]=*ptr;
        S.pop();
    } 
    else if (auto ptr = std::any_cast<std::vector<common_chat_msg>>(&container)) {
        d_cout << "vector common_chat_msg key is ..." << key << "\n";
        m[key]=*ptr;
        S.pop();
    }
    else if (auto ptr = std::any_cast<common_params*>(&container)) {
        d_cout << "common_params* key is " << key << "\n";
        m[key]=*ptr;
        S.pop();
    }
    else {                
      gettype(ctx); // print type_index is ....
      string v_type=anyToString(S.top()); S.pop();
      int n;
      if (v_type=="string")
      {        
        // special string prefix, trigger special type M entry
        string s0=anyToString(S.top());
        d_cout << "string key is " << key << "\n";
        d_cout << "  s0 " << s0 << std::endl;      // n = stoi(s0,nullptr,10); 

        if (s0=="t_bool") {
          S.pop();
          s0=anyToString(S.top());
          if (s0=="true") m[key]=true;
          else if (s0=="false") m[key]=false;
          else m[key]=true;
          
          if (key=="DBG") DBG=any_cast<bool>(m[key]); // special global flag
        }
        else m[key]=s0;
        
        S.pop();
      }
      else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
        n = any_cast<int>(S.top());
        m[key]=(int) n;
        S.pop();
        
        std::cout << "  s_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m[key]) << std::endl;
      }      
      d_cout << "  s_M m ASSIGNED " << endl;
    }
    DBG=X_DBG; // restore DBG
    } catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }
    return true;
}

    // stack pointer
    // LSM-20260530 check typeid *P[n] below ....
    // if (typeid(S.top().type())==typeid(std::string)) 
    // d_cout << "\ns_M m DEFINED " << endl; // new convention x_* 
    // green text
    // printf("\x1b[32m[Forth] Result: %.4g\x1b[0m\n", forth_vm.top_v
    // M["test"]=(int) 999;
    // (*pM)["test"]=(int) n;        
    // struct llama_model *     
        // dcout << "llama_model value is " << *ptr << "\n";
        // d_cout << "llama_model value is " << "\n";
        // std::vector<common_chat_msg>    
      // d_cout << "vector common_chat_msg value is ..." << "\n";
      // string v_type=any_cast<string>(S.top()); S.pop();
      // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
      // std::cout << "  s_M " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<string>(m[key]) << std::endl;
    // S.push((*P)[n]);
    // }

bool PhosVM::g_M_debug(int ctx){ // g_M get M, s_M set M modern convention
// S[0] = M, super buffer variable (PHP super global ?)
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
    // LSM-20260530 check typeid *P[n] below ....
    // if (typeid(S.top().type())==typeid(std::string)) 
    
    
    gettype(ctx);
    // string v_type=any_cast<string>(S.top()); S.pop();
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10); 
      S.pop();
    }
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
      n = any_cast<int>(S.top());
      S.pop();
    }
    
    /*
      n = S.size() - n - 1; // depth - n - 1 // get depth after pop n
      
      std::cout << "  depth " << S.size() << std::endl; // line by line msg until catch
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
      // (*P)[n].type()
      if (compareTypeIndexWithString(my_type, "std::string")) {
        std::cout << "The type_index is an std::string!\n";
      }
      
      std::cout.flush();
    
      // pick output
      // std::cout << "  pick " << std::any_cast<std::string>((*P)[n]) << std::endl;
      */
      
      // int n=0;
      // unordered_map<string, any> M;
      // C++ string index must use double quotes, not single quotes "test" !!
      
      // unordered_map<string, any>* pM = any_cast<unordered_map<string, any>>((*P)[0]);
      // unordered_map<string, any>* pM = any_cast<unordered_map<string, any>>(&((*P)[0]));
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
      std::cout << "  pick pM DEFINED " << endl;

      // M["test"]=(int) 999;
      // (*pM)["test"]=(int) n;
      // m["test"]=(int) n;
      std::cout << "  pick pM ASSIGNED " << endl;
      
      // any_cast<unordered_map<string, any>>((*P)[0])["test"]=(int) n;
      // any_cast<unordered_map<string, int>>((*P)[0])["test"]=(int) n;

      std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m["test"]) << std::endl;

      // std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>((*pM)["test"]) << std::endl;
      // std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(((*P)[0])["test"]) << std::endl;
//      std::cout << "  pick bucket_count " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  n " << n << " " << any_cast<int>((any_cast<unordered_map<string, any>>((*P)[0]))["test"]) << std::endl;
      // std::cout << "  pick bucket_count " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  n " << n << " " << (any_cast<unordered_map<string, any>>((*P)[0])["test"]) << std::endl;
      
      // S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;
}

bool PhosVM::p_M(int ctx){ // S[0] = M, super buffer variable (PHP super global ?)
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
    // LSM-20260530 check typeid *P[n] below ....
    // if (typeid(S.top().type())==typeid(std::string)) 
    
    
    gettype(ctx);
    // string v_type=any_cast<string>(S.top()); S.pop();
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10); 
      S.pop();
    }
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
      n = any_cast<int>(S.top());
      S.pop();
    }
    
    /*
      n = S.size() - n - 1; // depth - n - 1 // get depth after pop n
      
      std::cout << "  depth " << S.size() << std::endl; // line by line msg until catch
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
      // (*P)[n].type()
      if (compareTypeIndexWithString(my_type, "std::string")) {
        std::cout << "The type_index is an std::string!\n";
      }
      
      std::cout.flush();
    
      // pick output
      // std::cout << "  pick " << std::any_cast<std::string>((*P)[n]) << std::endl;
      */
      
      // int n=0;
      // unordered_map<string, any> M;
      // C++ string index must use double quotes, not single quotes "test" !!
      
      // unordered_map<string, any>* pM = any_cast<unordered_map<string, any>>((*P)[0]);
      // unordered_map<string, any>* pM = any_cast<unordered_map<string, any>>(&((*P)[0]));
      unordered_map<string, any>& m = any_cast<unordered_map<string, any>&>((*P)[0]);
      std::cout << "  pick pM DEFINED " << endl;

      // M["test"]=(int) 999;
      // (*pM)["test"]=(int) n;
      m["test"]=(int) n;
      std::cout << "  pick pM ASSIGNED " << endl;
      
      // any_cast<unordered_map<string, any>>((*P)[0])["test"]=(int) n;
      // any_cast<unordered_map<string, int>>((*P)[0])["test"]=(int) n;

      std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(m["test"]) << std::endl;

      // std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>((*pM)["test"]) << std::endl;
      // std::cout << "  pick " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  " << any_cast<int>(((*P)[0])["test"]) << std::endl;
//      std::cout << "  pick bucket_count " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  n " << n << " " << any_cast<int>((any_cast<unordered_map<string, any>>((*P)[0]))["test"]) << std::endl;
      // std::cout << "  pick bucket_count " << any_cast<unordered_map<string, any>>((*P)[0]).bucket_count() << "  n " << n << " " << (any_cast<unordered_map<string, any>>((*P)[0])["test"]) << std::endl;
      
      // S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;
}
      
bool PhosVM::pick(int ctx){ // FORTH has no peek, because .s show all stack, only few items ??
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
    // LSM-20260530 check typeid *P[n] below ....
    // if (typeid(S.top().type())==typeid(std::string)) 
    
    gettype(ctx);
    // string v_type=any_cast<string>(S.top()); S.pop();
    string v_type=anyToString(S.top()); S.pop();
    int n;
    if (v_type=="string")
    {
      // int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
      string s0=anyToString(S.top());
      std::cout << "  s0 " << s0 << std::endl;
      n = stoi(s0,nullptr,10); 
      S.pop();
    }
    else if (v_type=="int") { // int cast and string cast are not the same because string is pointer?
      n = any_cast<int>(S.top());
      S.pop();
    }
      n = S.size() - n - 1; // depth - n - 1 // get depth after pop n
      
      std::cout << "  depth " << S.size() << std::endl; // line by line msg until catch
      std::cout << "  n is " << n << std::endl;
      std::cout << (*P)[n].type().name() << std::endl;
      std::cout << "type_index " << type_index((*P)[n].type()).name() << std::endl;
      
      std::type_index my_type = std::type_index(typeid(int));
      if (compareTypeIndexWithString(my_type, "int")) {
        d_cout << "The type_index is an int!\n";
      }
      
      my_type = std::type_index(typeid(std::string));
      if (compareTypeIndexWithString(my_type, "std::string")) {
        d_cout << "The type_index is an std::string!\n";
      }

      my_type = std::type_index((*P)[n].type());
      // (*P)[n].type()
      if (compareTypeIndexWithString(my_type, "std::string")) {
        d_cout << "The type_index is an std::string!\n";
      }
      
      std::cout.flush();
    
      // pick output
      std::cout << "  pick " << std::any_cast<std::string>((*P)[n]) << std::endl;
      
      S.push((*P)[n]);
    // }
} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;
}

       // std::cout << "  P = &S ... OK  " << std::endl; // line by line msg until catch
        // std::cout << "S.top().type().name() " << S.top().type().name() << std::endl;

        // if (typeid(S.top().type())==typeid(std::string)) {
        // std::cout << typeid(S.top().type()).name() << " " << typeid(std::string).name() << std::endl;
        // std::cout << ((typeid(S.top().type()) == typeid(S.top().type())) ? "0" : "1") << " " << ((typeid(S.top().type()) == typeid(std::string)) ? "0" : "1") << "  in pick  " << std::endl;           
        // std::cout.flush();

    // from tostr !!
    // if ( std::any_cast<std::type_index>(S.top()) == std::type_index(typeid(int)) ) {

    // if (S.top().type().name())
    //}
    // if (typeid((*P)[n].type().name())==typeid(std::string)) 
    
      // LSM-20260530 this works? No, it does not !!
      /*
      if (typeid((*P)[n].type())==typeid(std::string)) 
          // std::cout << (*P)[n].type().name() << std::endl;
          std::cout << "  showstack ... is string  " << std::endl;
      else std::cout << "  is not string  " << std::endl;
      */

// Source - https://stackoverflow.com/a/42053423
// Posted by aniliitb10
// Retrieved 2026-06-04, License - CC BY-SA 3.0

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
  // if both int, output int
  // if both double, output double
  // if int double, output double
  // else output string error

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
      string TOS; //  is TOS int or double, affects I[0]-D[0] or D[0]-I[0]
      std::any container = S.top(); S.pop();
      if (auto ptr = std::any_cast<string>(&container)) {
          d_cout << "string value is " << *ptr << "\n";
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

// extern static common_params            * g_params;
// extern common_params            * g_params;
static common_params            * g_params;

/* LSM: chat_add_and_format FVM Clone&Migrate C/M 20260525_1425 */
static std::string phos_chat_add_and_format(struct llama_model * model, std::vector<common_chat_msg> & chat_msgs, const std::string & role, const std::string & content) {
    
    common_chat_msg new_msg{role, content};
    // push variables on to stack
    // push whole line as string to FVM, let FVM parse var type, var name, var values.
    // then pop var into C++ to check correctness.
    
    auto formatted = common_chat_format_single(model, g_params->chat_template, chat_msgs, new_msg, role == "user");
    chat_msgs.push_back({role, content});
    // LOG_DBG("formatted: '%s'\n", formatted.c_str());
    LOG("formatted: '%s'\n", formatted.c_str());
    return formatted;
}

bool PhosVM::CAAF(int ctx) { d_cout << "\nchat_add_and_format  ";
     // chat_add_and_format(model, chat_msgs, "user", std::move(buffer))
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
     
     d_cout << "\nCAAF model  " <<
     phos_chat_add_and_format(model, chat_msgs, role, content) << "   end\n";
     
     return true;
} 

#include <cstdlib>
bool PhosVM::sys_google(int ctx) { dcout << "\nsystem call ....  ";
    system("curl https://www.google.com");
    return true;
}

string pstr() { // pop string

    std::any container = S.top(); S.pop();
    string s0;
    if (auto ptr = std::any_cast<string>(&container)) {
        cout << "string value is " << *ptr << "\n";
        s0 = *ptr;
        return s0;
    } 
    else return "E_XSTR"; // error not string

}

bool PhosVM::sys(int ctx) { dcout << "\nsystem call ....  ";

    string s_of, s_url;
    s_of  = pstr(); if (s_of=="E_XSTR") return false;
    s_url = pstr(); if (s_of=="E_XSTR") return false;

    // system("curl https://www.google.com");
    string cmd = "curl " + s_url + " > _/outfile/" + s_of;
    system(cmd.c_str());    
    return true;
}

bool PhosVM::curl(int ctx) { dcout << "\ncurl ....  ";

    string s_of, s_url;
    s_of  = pstr(); if (s_of=="E_XSTR") return false;
    s_url = pstr(); if (s_of=="E_XSTR") return false;

    // system("curl https://www.google.com");
    string cmd = "curl " + s_url + " > _/outfile/" + s_of;
    system(cmd.c_str());    
    return true;
}

bool PhosVM::md5(int ctx) { dcout << "\nmd5sum ....  ";

    string s_of, s_url;
    s_of  = pstr(); if (s_of=="E_XSTR") return false;
    // s_url = pstr(); if (s_of=="E_XSTR") return false;

    string cmd = "md5sum _/outfile/" + s_of + " >> _/outfile/l_md5";
    system(cmd.c_str());    
    return true;
}

// https://github.com/HowardHinnant/date
#include "tz.h"

std::string current_time()
{
    const auto now_ms = date::floor<std::chrono::milliseconds>(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << date::make_zoned(date::current_zone(), now_ms);
    return ss.str();
}

bool PhosVM::mstime(int ctx) { // dcout << "\ntime ....  " << endl;

    string text = current_time();
    std::replace(text.begin(), text.end(), ' ', '_');

    dcout << "\ntime ....  " <<  text << endl;
    // system("curl https://www.google.com");
    return true;
}

#include <cstdint>

uint64_t cyrb53(const std::string& str, uint32_t seed = 0) {
    uint32_t h1 = 0xdeadbeef ^ seed;
    uint32_t h2 = 0x41c6ce57 ^ seed;

    for (char ch : str) {
        // cast to uint8_t to handle potential negative char values correctly
        uint32_t c = static_cast<uint8_t>(ch); 
        h1 = (h1 ^ c) * 2654435761U;
        h2 = (h2 ^ c) * 1597334677U;
    }

    h1 = ((h1 ^ (h1 >> 16)) * 2246822507U) ^ ((h2 ^ (h2 >> 13)) * 3266489909U);
    h2 = ((h2 ^ (h2 >> 16)) * 2246822507U) ^ ((h1 ^ (h1 >> 13)) * 3266489909U);

    // Combine h1 and h2 into a single 64-bit unsigned integer
    return (static_cast<uint64_t>(2097151 & h2) << 32) | h1;
}

/*
#include <string>
#include <vector>
#include <sstream>
#include <stdexcept> */
#include <iomanip>

// A standard helper function to decode Base64 (simulating atob)
std::string atob(const std::string& input) {
    static const std::string b64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[b64_chars[i]] = i;

    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (T[c] == -1) {
            if (c == '=') break; // Padding reached
            continue;            // Skip non-base64 characters like whitespace
        }
        val = (val << 6) | T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// Helper to parse a hex string into an unsigned __int128
unsigned __int128 hexToBigInt(const std::string& hex) {
    unsigned __int128 result = 0;
    for (char c : hex) {
        result <<= 4;
        if (c >= '0' && c <= '9')      result += (c - '0');
        else if (c >= 'a' && c <= 'f') result += (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') result += (c - 'A' + 10);
    }
    return result;
}

// The translated function
unsigned __int128 b64ToBn(const std::string& b64) {
    // 1. Decode base64 to binary string
    std::string bin = atob(b64);
    
    // 2. Convert each character to its 2-digit hex representation
    std::stringstream ss;
    for (unsigned char ch : bin) {
        // setting width to 2 and filling with '0' handles the JS `h.length % 2` check automatically
        ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(ch);
    }
    
    std::string hexStr = ss.str();

    // 3. Convert the hex string back into a BigInt type
    return hexToBigInt(hexStr);
}

// A standard helper function to encode a string to Base64 (simulating btoa)
std::string btoa(const std::string& data) {
    static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    int val = 0, valb = -6;
    for (unsigned char c : data) {
        val = (val << 8) | c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(lookup[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4) out.push_back('=');
    return out;
}

// The translated function
// Note: If 'bn' exceeds 64-bit/128-bit limits, replace unsigned __int128 
// with your BigInt library's integer type (e.g., boost::multiprecision::cpp_int)
// std::string bnToB64(unsigned __int128 bn) {
std::string bnToB64(uint64_t bn) {
    // 1. Convert BigInt to hex string
    // bn 64 bit
    std::stringstream ss;
    ss << std::hex << bn;

// bn 128 bit
/*    ss << std::hex 
       << static_cast<unsigned long long>(bn >> 64) 
       << std::setfill('0') << std::setw(16) 
       << static_cast<unsigned long long>(bn & 0xFFFFFFFFFFFFFFFFULL);
*/
    std::string hex = ss.str();

    // 2. If hex string length is odd, pad with leading '0'
    if (hex.length() % 2 != 0) {
        hex = '0' + hex;
    }

    // 3. Convert hex pairs to raw binary characters
    std::string bin = "";
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string part = hex.substr(i, 2);
        char b = static_cast<char>(std::stoul(part, nullptr, 16));
        bin.push_back(b);
    }

    // 4. Return Base64 encoded string
    return btoa(bin);
}

bool PhosVM::h53s(int ctx) { dcout << "\nh53 .... H0 "; // << endl;

    std::any container = S.top(); S.pop();
    string s0;
    if (auto ptr = std::any_cast<string>(&container)) {
        dcout << "string value is " << *ptr << "\n";
        s0 = *ptr;
    } 
    
    uint64_t H0 = cyrb53(s0,0);
    dcout << H0 << " " << bnToB64(H0) << endl;
    s0 = bnToB64(H0);
    S.push(s0);
    return true;
}

bool PhosVM::h53(int ctx) { dcout << "\nh53 .... H0 "; // << endl;

    std::any container = S.top(); S.pop();
    string s0;
    if (auto ptr = std::any_cast<string>(&container)) {
        dcout << "string value is " << *ptr << "\n";
        s0 = *ptr;
    } 
    
    uint64_t H0 = cyrb53(s0,0);
    dcout << H0 << endl;
    S.push(H0);
    return true;
}

/*
"function bnToB64(bn){\n  
var hex = BigInt(bn).toString(16);\n  

if (hex.length % 2) { hex = '0' + hex; }\n  var bin = [];\n  var i = 0;\n  var d;\n  var b;\n  while (i < hex.length) {\n    d = parseInt(hex.slice(i, i + 2), 16);\n    b = String.fromCharCode(d);\n    bin.push(b);\n    i += 2;\n  }\n  return btoa(bin.join(''));\n}"

*/
