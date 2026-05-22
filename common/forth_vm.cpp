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

void PhosVM::execute(std::string cmd, int ctx) {
        // Syntax: ReturnType (ClassName::*)(Args...)
        
        /*
        using Handler = bool (PhosVM::*)(int);
        std::unordered_map<std::string, Handler> handlers = {
            {"ADD", &PhosVM::add},
            {"SUB", &PhosVM::sub}
        };
        */

    // auto tokens = tokenize(command);
    auto tokens = tokenize(cmd);
    
    int i; std::string tok;
    if (tokens[1]=="DBG") std::cout << "  size() " << tokens.size() << " / ";        
    for(i=0;i<tokens.size();i++)
    {
        // printf("%s",argv[i]);
        if (tokens[1]=="DBG") std::cout << tokens[i] << " / "; // debug mode !PHOS DBG
 
        tok=tokens[i];
        if (handlers.count(tok)) {
            Handler func = handlers[tok];
            // Must use (instance.*pointer)(args)
            (this->*func)(ctx); 
        }
        else S.push(tok);
    }
    
    if (tokens[0]=="PHOS") {
      std::cout << "  is PHOS !!  msg_start: ";

        if (handlers.count(cmd)) {
            Handler func = handlers[cmd];
            // Must use (instance.*pointer)(args)
            (this->*func)(ctx); 
        }
    }
}

bool PhosVM::dup(int ctx) { std::cout << "dup ...\n"; 
    if (S.empty()) return false; // Handle empty stack
    std::any topElement = S.top(); // Get the top element
    S.push(topElement);            // Push it back
    // S.push(S.end()); 
    return true; 
}
bool PhosVM::depth(int ctx) { std::cout << "depth " << S.size() << endl; 
    // if (S.empty()) return false; // Handle empty stack
    S.push(S.size());
    return true; 
}
#include <typeindex>
bool PhosVM::gettype(int ctx) { // comment here
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
bool PhosVM::peek(int ctx){ // FORTH has no peek
try {
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
        std::cout << "  P = &S ... OK  " << std::endl; // line by line msg until catch
        std::cout << S.top().type().name() << std::endl;

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

} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;

}    
bool PhosVM::pick(int ctx){ // FORTH has no peek
try {        
        std::deque<std::any>* P;
        P = (std::deque<std::any>*) &S; // stack pointer
        std::cout << "  P = &S ... OK  " << std::endl; // line by line msg until catch
        std::cout << S.top().type().name() << std::endl;

    int n = std::stoi(std::any_cast<std::string>(S.top()),nullptr,10); // all items on stack are string because of tokenize()
    S.pop();
    n = S.size() - n - 1; // depth - n - 1 // get depth after pop n
    std::cout << "  depth " << S.size() << std::endl; // line by line msg until catch
    std::cout << "  n is " << n << std::endl;
    std::cout << (*P)[n].type().name() << std::endl;
    // if (typeid((*P)[n].type().name())==typeid(std::string)) 
    if (typeid((*P)[n].type())==typeid(std::string)) 
        // std::cout << (*P)[n].type().name() << std::endl;
        std::cout << "  showstack ... is string  " << std::endl;
    else std::cout << "  is not string  " << std::endl;
    std::cout << std::any_cast<std::string>((*P)[n]) << std::endl;
    
    S.push((*P)[n]);

} catch (std::exception& e){ std::cerr << "exception: " << e.what() << std::endl; }

    return true;
}    
bool PhosVM::add(int ctx) { std::cout << "Adding...\n"; return true; }
bool PhosVM::sub(int ctx) { std::cout << "Subtracting...\n"; return true; }
