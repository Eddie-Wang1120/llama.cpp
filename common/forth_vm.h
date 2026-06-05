#pragma once
#include <stack>
#include <any>
#include <string>
#include <unordered_map>
#include <functional>
#include "llama.h"

// PhosVM
#include <iostream>
#include <string>
#include <unordered_map>

// .h included many times
// std::stack<std::any> S;

// Forward declare llama context wrapper
struct llama_context;

class ForthVM {
public:
    // Stack types
    using StringStack = std::stack<std::string>;
    using ValueStack = std::stack<double>;  // MVP: numeric only; extend to variant later
    using AnyStack = std::stack<std::any>;
    
    // Command handler signature
    using Handler = std::function<bool(ForthVM&, llama_context*)>;
    
    // std::unordered_map<std::string, void(ForthVM::*)()> handlers_;
    // std::unordered_map<std::string, bool(ForthVM::*)()> handlers_;
    // std::unordered_map<std::string, bool(ForthVM::*)(ForthVM &, llama_context *)()> handlers_;
    // std::unordered_map<std::string, std::function<bool(ForthVM&, llama_context*)>> handlers_;
    // std::unordered_map<std::string, void(*)()> handlers_;
    // std::unordered_map<std::string, bool(*)()> handlers_;
    // std::unordered_map<std::string, bool(ForthVM::*)(ForthVM&, llama_context*)> handlers_;
    // std::unordered_map<std::string, bool(ForthVM&, llama_context*)> handlers_;
    std::unordered_map<std::string, Handler> handlers_;
    std::unordered_map<std::string, void(*)()> funcMap;

    // PhosVM convention ??
    // using HandlerX = bool (ForthVM::*)(int);
    // using Handler = std::function<bool(ForthVM&, llama_context*)>;
    using HandlerX = bool(ForthVM::*)(ForthVM&, llama_context*);
    std::unordered_map<std::string, HandlerX> handlers = {
            {"ADD", &ForthVM::cmd_add},
            {"SUB", &ForthVM::cmd_sub}
        };


    ForthVM();

    void execute(std::string cmd, int ctx) {
      // handlers_["+"] = &ForthVM::cmd_add;
      // handlers_["+"] = ForthVM::cmd_add;
    }
    
    // Parse and execute a command string
    // Returns: true if execution succeeded, false on error
    bool execute(const std::string& command, llama_context* ctx = nullptr);
    
    // Get top of string stack (for debugging/output)
    std::string top_string() const;
    bool has_string() const;
    int size() const;
        
    // Get top of value stack
    double top_value() const;
    bool has_value() const;

    // Register custom command handler
    void register_handler(const std::string& name, Handler fn);
    
    // cxxforth(): weird naming but then .... let forkers/forthers decide ....
    int forth(int argc, char** argv) const;
    int evaluate(int argc, char** argv, const std::string& command) const;
    
private:
    StringStack string_stack_;
    ValueStack value_stack_;
    // std::unordered_map<std::string, Handler> handlers_;
    
    // Built-in handlers
    bool cmd_add(ForthVM& vm, llama_context* ctx);      // +
    bool cmd_sub(ForthVM& vm, llama_context* ctx);      // -
    bool cmd_mul(ForthVM& vm, llama_context* ctx);      // *
    bool cmd_div(ForthVM& vm, llama_context* ctx);      // /
    bool cmd_sym(ForthVM& vm, llama_context* ctx);      // sym: (push symbol name)
    bool cmd_temp(ForthVM& vm, llama_context* ctx);     // temp: (set temperature)
    bool cmd_top_p(ForthVM& vm, llama_context* ctx);    // top-p: (set nucleus sampling)
    bool cmd_print(ForthVM& vm, llama_context* ctx);    // print: (output TOS)
    
    // Helper: tokenize space-delimited input
    static std::vector<std::string> tokenize(const std::string& input);
};

class PhosVM {
public:
    bool dup(int ctx); // ze FIRST of all words ...
    bool depth(int ctx); // S.size() 
    bool gettype(int ctx); // type() is a C++ function to get the type of variable!!    // need gettype tostr
    bool gettype_any(int ctx); // not working, left for debug.
    bool tostr(int ctx); // to string
    bool type(int ctx); // must convert everything to string before output ??
    bool cr(int ctx);     
    bool showstack(int ctx);     
    bool peek(int ctx); 
    bool p_M(int ctx); // debug M
    bool g_M(int ctx); // key g_M -- get element index key in M
    bool k_M(int ctx); // k_M -- list all keys in M
    bool s_M(int ctx); // value key s_M -- set element index key to value in M
    bool pvs(int ctx); // vector pvs -- print vector<string>
    bool inV(int ctx); // vector string inV -- is string in vector ?
    bool g_M_debug(int ctx); 
    bool g_M_gettype(int ctx);
    bool peekr(int ctx); // m n peekr // peek range with type check     
    bool peek_any(int ctx); // not working
    bool pick(int ctx); 
    bool find(int ctx);             
    // bool push(int ctx, std::any obj);             
    bool push(int ctx);                 
    bool CAAF(int ctx);      
    bool add(int ctx); // { std::cout << "Adding...\n"; return true; }
    bool sub(int ctx); // { std::cout << "Subtracting...\n"; return true; }
    static std::vector<std::string> tokenize(const std::string& input);

    using Handler = bool (PhosVM::*)(int);
    std::unordered_map<std::string, Handler> handlers = {
            {"dup", &PhosVM::dup},
            {"depth", &PhosVM::depth},
            {"gettype", &PhosVM::gettype},          
            {"tostr", &PhosVM::tostr},                      
            {"type", &PhosVM::type}, 
            {"cr", &PhosVM::cr},                                  
            {".s", &PhosVM::showstack},
            {"peek", &PhosVM::peek},
            {"peekr", &PhosVM::peekr},
            {"pick", &PhosVM::pick},
            {"p_M", &PhosVM::p_M},
            {"g_M", &PhosVM::g_M},
            {"s_M", &PhosVM::s_M},
            {"k_M", &PhosVM::k_M},
            {"pvs", &PhosVM::pvs},            
            {"inV", &PhosVM::inV},
            {"vcb", &PhosVM::pick}, // variable curly bracket (define)
            {"veq", &PhosVM::pick}, // variable eq (define)
            {"vdc", &PhosVM::pick}, // variable declare (no define)
            {"find", &PhosVM::find}, // get index of stack item matching string partially
            {"ADD", &PhosVM::add},
            {"push", &PhosVM::push},
            {"CAAF", &PhosVM::CAAF},
            {"add", &PhosVM::add},
            {"sub", &PhosVM::sub},
            {"SUB", &PhosVM::sub}
        };
        
    void execute(std::string cmd, int ctx);
    /* {
        // Syntax: ReturnType (ClassName::*)(Args...)
        
        using Handler = bool (PhosVM::*)(int);
        std::unordered_map<std::string, Handler> handlers = {
            {"ADD", &PhosVM::add},
            {"SUB", &PhosVM::sub}
        };

        if (handlers.count(cmd)) {
            Handler func = handlers[cmd];
            // Must use (instance.*pointer)(args)
            (this->*func)(ctx); 
        }
    }
    */
};

