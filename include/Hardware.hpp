#define ELPP_STL_LOGGING
#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP
#include <rigtorp/SPSCQueue.h>
#include <deque>
#include <any>

#define HardwarePrintModifiersPrintValue(value) #value << ": " << value
#define HardwarePrintModifiersAlphaBool(boolean) #boolean << ": " << (boolean ? "true" : "false")
#define HardwarePrint CLOG(INFO, "hardware")
#define HardwarePrintIf(condition) CLOG_IF(condition, INFO, "hardware")

template <typename T, int CAPACITY = 1>
struct Flop {
    rigtorp::SPSCQueue<T> * input = nullptr;
    rigtorp::SPSCQueue<T> * output = nullptr;
    bool hasOutput = false;
    bool hasInput = false;
    bool debug_output = false;
    
    Flop() {
        input = new rigtorp::SPSCQueue<T>(CAPACITY);
        output = new rigtorp::SPSCQueue<T>(CAPACITY);
    }
    
    Flop(const bool & debug_output) {
        input = new rigtorp::SPSCQueue<T>(CAPACITY);
        output = new rigtorp::SPSCQueue<T>(CAPACITY);
        this->debug_output = debug_output;
    }
    
    Flop(const Flop & flop) {
        // copy constructor
        // input can only be moved
        std::swap(input, const_cast<Flop&>(flop).input);
        // output can only be moved
        std::swap(output, const_cast<Flop&>(flop).output);
        debug_output = flop.debug_output;
    }
    
    Flop(Flop && flop) {
        // move constructor
        std::swap(input, flop.input);
        std::swap(output, flop.output);
        std::swap(debug_output, flop.debug_output);
    }
    
    Flop & operator=(const Flop & flop) {
        // copy assign
        // input can only be moved
        std::swap(input, const_cast<Flop&>(flop).input);
        // output can only be moved
        std::swap(output, const_cast<Flop&>(flop).output);
        debug_output = flop.debug_output;
        return *this;
    }
    
    Flop & operator=(Flop && flop) {
        // move assign
        std::swap(input, flop.input);
        std::swap(output, flop.output);
        std::swap(debug_output, flop.debug_output);
        return *this;
    }
    
    ~Flop() {
        delete input;
        delete output;
    }
    
    bool has_input() {
        return input->front() != nullptr;
    }
    
    bool has_output() {
        return output->front() != nullptr;
    }
    
    void push_input(T && in) {
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pushing input";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(input->size());
        input->push(std::move(in));
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pushed input";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(input->size());
        hasInput = true;
    }
    
    void push_output(T && out) {
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pushing output";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(output->size());
        output->push(std::move(out));
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pushed ouput";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(output->size());
        hasOutput = true;
    }
    
    T && pull_input() {
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pulling input";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(input->size());
        T && in = std::move(*input->front());
        input->pop();
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pulled input";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(input->size());
        hasInput = false;
        return std::move(in);
    }
    
    T && pull_output() {
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pulling output";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(output->size());
        T && out = std::move(*output->front());
        output->pop();
        HardwarePrintIf(debug_output) << "[FLOP   ] " << "pulled output";
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(output->size());
        hasOutput = false;
        return std::move(out);
    }
    
    void exec() {
        HardwarePrintIf(debug_output) << "[FLOP   ] " << HardwarePrintModifiersPrintValue(input->front());
        if (input->front()) {
            push_output(std::move(pull_input()));
        }
    }
};

template <typename T>
struct Wire {
    Flop<T, 1> data;
    std::deque<std::string> outputs;
    
    Wire() {}
    
    Wire(const Wire & wire) {
        data = wire.data;
        outputs = wire.outputs;
    }
    
    Wire(Wire && wire) {
        std::swap(data, wire.data);
        std::swap(outputs, wire.outputs);
    }
    
    Wire & operator=(const Wire & wire) {
        data = wire.data;
        outputs = wire.outputs;
        return *this;
    }
    
    Wire & operator=(Wire && wire) {
        std::swap(data, wire.data);
        std::swap(outputs, wire.outputs);
        return *this;
    }
    
    void push(T & input) {
        data.push_input(std::move(input));
        data.exec();
    }
    
    void push(T && input) {
        data.push_input(std::move(input));
        data.exec();
    }
    
    T && pull() {
        return std::move(data.pull_output());
    }
};

struct ComponentTypes {
    static constexpr int Undefined = -1;
    static constexpr int Wire = 0;
};

struct Component {
    int type = ComponentTypes::Undefined;
    std::string id = "Undefined";
    std::any component;
    
    Component(int type, std::any & component, const char * component_name) {
        this->type = type;
        this->component = component;
        this->id = component_name;
    }
    
    Component(int type, std::any && component, const char * component_name) {
        this->type = type;
        this->component = std::move(component);
        this->id = component_name;
    }
};

template <typename T>
struct Hardware {
    el::Logger* logger = nullptr;
    el::Configurations config;
    
    Hardware() {
        logger = el::Loggers::getLogger("hardware");
        config.setToDefault();
        config.setGlobally(el::ConfigurationType::Format, "[%logger:%level] %msg");
        el::Loggers::reconfigureLogger("hardware", config);
    }
    
    std::deque<Component> components;
    
    void addComponent(const int component_type, std::any & component, const char * component_name) {
        components.push_back(Component(ComponentTypes::Wire, component, component_name));
    }

    void addComponent(const int component_type, std::any && component, const char * component_name) {
        components.push_back(Component(ComponentTypes::Wire, std::move(component), component_name));
    }
    
    void addWire(const char * wireName) {
        addComponent(ComponentTypes::Wire, std::move(Wire<T>()), wireName);
    }
    
    std::optional<std::reference_wrapper<Component>> find_component(const std::string & id) {
        const char * component_id = id.c_str();
        const size_t component_id_length = id.length();
        for (Component & component : components) {
            if (memcmp(component_id, component.id.c_str(), component_id_length) == 0) {
                return component;
            }
        }
        return std::nullopt;
    }
    
    std::optional<std::reference_wrapper<Wire<T>>> find_wire(const std::string & id) {
        auto component = find_component(id);
        if (component.has_value()) {
            Wire<T> & wire = std::any_cast<Wire<T>&>(component.value().get().component);
            return std::optional<std::reference_wrapper<Wire<T>>>(wire);
        }
        return std::nullopt;
    }
    
    void connectWires(const std::string & wire_1, const std::string & wire_2) {
        std::optional<std::reference_wrapper<Wire<T>>> in = find_wire(wire_1);
        CHECK_EQ(in.has_value(), true) << "wire does not exist: " << wire_1;
        CHECK_EQ(find_wire(wire_2).has_value(), true) << "wire does not exist: " << wire_2;
        in.value().get().outputs.push_back(wire_2);
    }
    
    void run(const std::string & id, T input) {
        std::optional<std::reference_wrapper<Wire<T>>> start = find_wire(id);
        start.value().get().push(std::move(input));
        T && copy = std::move(start.value().get().pull());
        if (!start.value().get().outputs.empty()) {
            for (std::string & out_id : start.value().get().outputs) {
                std::optional<std::reference_wrapper<Wire<T>>> out = find_wire(out_id);
                out.value().get().push(copy);
            }
        }
    }
};
