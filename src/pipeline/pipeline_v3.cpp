#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP
#include <rigtorp/SPSCQueue.h>
#include <iostream>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <unistd.h>
#include <queue>
#include <condition_variable>
#include <chrono>
using namespace std::chrono_literals;
#include <deque>
#include <cassert>

#define PipelinePrintModifiersPrintValue(value) #value << ": " << value
#define PipelinePrintModifiersAlphaBool(boolean) #boolean << ": " << (boolean ? "true" : "false")
#define PipelinePrintModifiersPrintStage(stage) "[Stage " << stage << "]"
#define PipelinePrintIf(condition) CLOG_IF(condition, INFO, "pipeline")
#define PipelinePrint CLOG(INFO, "pipeline")
#define PipelineFPrintIf(condition, type) CLOG_IF(condition, type, "pipeline")
#define PipelineFPrint(type) CLOG(type, "pipeline")
#define PipelinePrintStageIf(condition, stage) PipelinePrintIf(condition) << PipelinePrintModifiersPrintStage(stage) << " "
#define PipelinePrintStage(stage) PipelinePrint << PipelinePrintModifiersPrintStage(stage) << " "
#define PipelineFPrintStageIf(condition, type, stage) PipelineFPrintIf(condition, type) << PipelinePrintModifiersPrintStage(stage) << " "
#define PipelineFPrintStage(type, stage) PipelineFPrint(type) << PipelinePrintModifiersPrintStage(stage) << " "

namespace std {
    template< class Rep, class Period, class Predicate >
    bool timeout(const std::chrono::duration<Rep, Period>& rel_time, Predicate pred) {
        auto start = std::chrono::high_resolution_clock::now();
        auto end = start + rel_time;
        bool status = true;
        do {
            status = !pred();
            if (status == false) break;
        } while(std::chrono::high_resolution_clock::now() < end);
        return status;
    }
}

struct TimeSince {
    struct duration {
        typedef std::ratio<1l, 1000000000l> nano;
        typedef std::chrono::duration<unsigned long long,         std::nano> nanoseconds;
        typedef std::chrono::duration<unsigned long long,        std::micro> microseconds;
        typedef std::chrono::duration<unsigned long long,        std::milli> milliseconds;
        typedef std::chrono::duration<unsigned long long                   > seconds;
        typedef std::chrono::duration<     unsigned long, std::ratio<  60> > minutes;
        typedef std::chrono::duration<     unsigned long, std::ratio<3600> > hours;
    };
    
    std::string time;

    std::chrono::time_point<std::chrono::high_resolution_clock> start;
    
    const TimeSince & elapse() {

        duration::nanoseconds nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start);

        duration::microseconds microseconds = std::chrono::duration_cast<duration::microseconds>(nanoseconds);
        nanoseconds -= std::chrono::duration_cast<duration::nanoseconds>(microseconds);

        duration::milliseconds milliseconds = std::chrono::duration_cast<duration::milliseconds>(microseconds);
        microseconds -= std::chrono::duration_cast<duration::milliseconds>(microseconds);

        duration::seconds seconds = std::chrono::duration_cast<duration::seconds>(milliseconds);
        milliseconds -= std::chrono::duration_cast<duration::milliseconds>(seconds);

        seconds -= std::chrono::duration_cast<duration::seconds>(std::chrono::duration_cast<duration::minutes>(seconds));
    
        auto s = seconds.count();
        time = "";
        if (s < 10) time += "0";
        time += std::to_string(s);
        time += ":";
        auto mil = milliseconds.count();
        if (mil < 10) time += "00";
        else if (mil < 100) time += "0";
        time += std::to_string(mil);
        time += ":";
        auto micr = microseconds.count();
        if (micr < 10) time += "00";
        else if (micr < 100) time += "0";
        time += std::to_string(micr);
        time += ":";
        auto n = nanoseconds.count();
        if (n < 10) time += "00";
        else if (n < 100) time += "0";
        time += std::to_string(n);
        return *this;
    }
    
    void mark() {
        start = std::chrono::high_resolution_clock::now();
    }
    
    friend std::ostream &operator<<(std::ostream & output, const TimeSince & D) {
        output << const_cast<TimeSince &>(D).elapse().time;
        return output;
    }
};

template <typename T>
using PipelineQueueType =
// both deque and SPCQueue have a CAPACITY constructor
// std::deque<T>
rigtorp::SPSCQueue<T>
;

#define PipelineLambda(val, index, pipeline) [] (auto && val, int index, auto * pipeline, auto * input, auto * output)

#define PipelineCycleLambda(pipeline) [] (auto * pipeline)

TimeSince program_start;

struct Electronics {
    template <typename T, T INITIALIZER>
    struct SignalEdgeDetector {
        T signalStored = INITIALIZER;
        
        bool is_rise(T signal) {
            bool ret = false;
            if (signalStored == 0 && signal == 1) ret = true;
            signalStored = signal;
            return ret;
        }

        bool is_fall(T signal) {
            return !is_rise(signal);
        }
    };
    
    // this is a Flip-Flop, specifically a Type D ("Data" or "Delay") Flip-Flop
    
    template <typename T, int CAPACITY>
    struct Flop {
        rigtorp::SPSCQueue<T> * input = nullptr;
        rigtorp::SPSCQueue<T> * output = nullptr;
        rigtorp::SPSCQueue<T> * intermediateOutput = nullptr;
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
        
        bool has_intermediateOutput() {
            return intermediateOutput != nullptr ? intermediateOutput->front() != nullptr : false;
        }

        void push_input(T && in) {
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pushing input";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(input->size());
            input->push(std::move(in));
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pushed input";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(input->size());
        }
        
        void push_output(T && out) {
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pushing output";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(output->size());
            output->push(std::move(out));
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pushed ouput";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(output->size());
        }
        
        void push_intermediateOutput(T && out) {
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pushing intermediate output";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(intermediateOutput->size());
            intermediateOutput->push(std::move(out));
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pushed intermediate output";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(intermediateOutput->size());
        }

        T pull_input() {
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pulling input";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(input->size());
            T in = std::move(*input->front());
            input->pop();
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pulled input";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(input->size());
            return std::move(in);
        }
        
        T pull_output() {
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pulling output";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(output->size());
            T out = std::move(*output->front());
            output->pop();
            PipelinePrintIf(debug_output) << "[FLOP   ] " << "pulled output";
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(output->size());
            return std::move(out);
        }
        
        void exec() {
            PipelinePrintIf(debug_output) << "[FLOP   ] " << PipelinePrintModifiersPrintValue(input->front());
            if (input->front()) {
                if (intermediateOutput != nullptr) push_intermediateOutput(std::move(pull_input()));
                else push_output(std::move(pull_input()));
            }
        }
    };
};

struct PipelineStageTypes {
    static const int Undefined = -1;
    static const int Stage = 0;
    static const int Flop = 1;
};

template <typename T, T CAPACITY>
struct Pipeline {
    
    bool debug_output = false;
    
    el::Logger* pipelineLogger = nullptr;
    el::Configurations config;

    static std::string getTime(const el::LogMessage* message) {
        return program_start.elapse().time;
    }
    
    Pipeline () {
        pipelineLogger = el::Loggers::getLogger("pipeline");
        el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier("%TIMESINCESTART", getTime));
        config.setToDefault();
    }
    
    Pipeline(const bool & debug_output) {
        pipelineLogger = el::Loggers::getLogger("pipeline");
        el::Helpers::installCustomFormatSpecifier(el::CustomFormatSpecifier("%TIMESINCESTART", getTime));
        config.setToDefault();
        this->debug_output = debug_output;
    }
    
    // if this is false the pipeline will execute in producer-consumer mode
    // this is very similar to how an actual pipelined cpu operates
    bool sequential = false;
    bool manual_increment = false;
    
    // move the input instead of copying it
    // this allows for input mutation
    typedef std::function<void(T && val, int index, Pipeline<T, CAPACITY> * pipeline, PipelineQueueType<T> * input, PipelineQueueType<T> * output)> Task;
    
    typedef std::function<void(Pipeline * pipeline)> Cycle;
    
    Cycle cycleFuncNoop = [] (Pipeline * pipeline) {};
    Cycle cycleFunc = cycleFuncNoop;

    struct Stage {
        int type = PipelineStageTypes::Undefined;
        PipelineQueueType<T> * output = nullptr;
        Task pre = nullptr;
        Task run = nullptr;
        Task post = nullptr;
        Electronics::Flop<T, CAPACITY> flop;
        
        bool terminated = false;
        std::atomic<bool> executedInThisCycle {false};

        Stage() {
            output = new rigtorp::SPSCQueue<T>(CAPACITY);
            flop.debug_output = false;
        }
        
        Stage(const bool & debug_output) {
            output = new rigtorp::SPSCQueue<T>(CAPACITY);
            flop.debug_output = debug_output;
        }

        Stage(const Stage & stage) = delete;
        
        Stage(Stage && stage) {
            // move constructor
            std::swap(output, stage.output);
            std::swap(type, stage.type);
            std::swap(pre, stage.pre);
            std::swap(run, stage.run);
            std::swap(post, stage.post);
            std::swap(flop, stage.flop);
        }
        
        Stage & operator=(const Stage & stage) = delete;
        
        Stage & operator=(Stage && stage) {
            // move assign
            std::swap(output, stage.output);
            std::swap(type, stage.type);
            std::swap(pre, stage.pre);
            std::swap(run, stage.run);
            std::swap(post, stage.post);
            std::swap(flop, stage.flop);
            return *this;
        }
        
        ~Stage() {
            delete output;
        }
    };
    
    std::deque<Stage> stages;
    
    #define PipelineLambdaArguments Pipeline * pipeline, Stage * stage, int index, PipelineQueueType<T> * input, PipelineQueueType<T> * output, std::atomic<bool> * current_halt, std::atomic<bool> * next_halt, std::atomic<int> * halted_cycle
    
    #define PipelineLambdaTickArguments Pipeline * pipeline, std::atomic<bool> * current_halt
    
    typedef std::function<void(PipelineLambdaArguments)> TaskCallback;
    typedef std::function<void(PipelineLambdaTickArguments)> TickCallback;
    
    #define PipelineLambdaCallback [] (PipelineLambdaArguments)
    #define PipelineLambdaTickCallback [] (PipelineLambdaTickArguments)
    
    TickCallback tickcallback = PipelineLambdaTickCallback {
        while(!current_halt->load()) {
            PipelinePrintIf(pipeline->debug_output) << "[TICK   ] sleeping for 1 second";
            std::this_thread::sleep_for(1s);
            PipelinePrintIf(pipeline->debug_output) << "[TICK   ] slept for 1 second";
            PipelinePrintIf(pipeline->debug_output) << "[TICK   ] ticking";
            pipeline->cycleFunc(pipeline);
            // new cycle, reset execution status of all stages
            for (Stage & stage : pipeline->stages) {
                stage.executedInThisCycle = false;
            }
            PipelinePrintIf(pipeline->debug_output) << "[TICK   ] ticked";
        }
    };
    
    struct Functions {
        Pipeline<T, CAPACITY> * pipeline = nullptr;
        
        size_t instruction_length = 0;
        
        int index_of_prev_stage = 0;
        int index_of_this_stage = 0;
        int index_of_next_stage = 0;
        
        Stage * prev_stage = nullptr;
        Stage * this_stage = nullptr;
        Stage * next_stage = nullptr;
        
        Electronics::Flop<T, CAPACITY> * prev_flip_flop = nullptr;
        Electronics::Flop<T, CAPACITY> * this_flip_flop = nullptr;
        Electronics::Flop<T, CAPACITY> * next_flip_flop = nullptr;
        
        bool prev_stage_is_flip_flop = false;
        bool this_stage_is_flip_flop = false;
        bool next_stage_is_flip_flop = false;
        
        Functions(Pipeline<T, CAPACITY> * pipeline) {
            this->pipeline = pipeline;
        }

        Functions(Pipeline<T, CAPACITY> * pipeline, const int & index) {
            this->pipeline = pipeline;
            aquire_indexes_stages_and_flip_flops(index);
        }
        
        void aquire_indexes_stages_and_flip_flops(const int & index) {
            index_of_prev_stage = index-1;
            index_of_this_stage = index;
            index_of_next_stage = index+1;
            
            if (index_of_prev_stage != -1) {
                prev_stage = &pipeline->stages[index_of_prev_stage];
                prev_flip_flop = &prev_stage->flop;
                prev_stage_is_flip_flop = prev_stage->type == PipelineStageTypes::Flop;
            };
            
            this_stage = &pipeline->stages[index_of_this_stage];
            this_flip_flop = &this_stage->flop;
            this_stage_is_flip_flop = this_stage->type == PipelineStageTypes::Flop;
            
            if (index_of_next_stage < pipeline->stages.size()) {
                next_stage = &pipeline->stages[index_of_next_stage];
                next_flip_flop = &next_stage->flop;
                next_stage_is_flip_flop = next_stage->type == PipelineStageTypes::Flop;
            }
        }
        
        void store_instruction_length(PipelineQueueType<T> * input) {
            instruction_length = input->size();
        }
        
        bool program_counter_is_greater_than_instruction_length() {
            return pipeline->PC[0] > (instruction_length-1);
        }
        
        std::string prev_stage_as_string() {
            return std::move(std::string("Stage ") + std::to_string(index_of_prev_stage));
        }

        std::string this_stage_as_string() {
            return std::move(std::string("Stage ") + std::to_string(index_of_this_stage));
        }
        
        std::string next_stage_as_string() {
            return std::move(std::string("Stage ") + std::to_string(index_of_next_stage));
        }
        
        void lock(std::unique_lock<std::mutex> & unique_lock) {
            PipelineFPrintStageIf(
                std::timeout(1s, [&] {
                    try {
                        unique_lock.lock();
                        // succeeded
                        return true;
                    } catch (const std::system_error& e) {
                        PipelinePrintStageIf(pipeline->debug_output, index_of_this_stage)
                            << "Failed to lock: " << e.what();
                        std::this_thread::sleep_for(250ms);
                        // try again
                        return false;
                    }
                }),
                FATAL, index_of_this_stage
            ) << std::endl << std::endl
                << "Failed to aquire a lock within 1 second." << std::endl
                << "There is likely a hang somewhere." << std::endl
                << "For example, a conditional wait not being satisfied" << std::endl;
        }
    };
    
    std::deque<std::thread> pool;
    std::deque<PipelineQueueType<T> *> queues;
    std::deque<std::condition_variable*> conditions;
    std::deque<std::mutex*> mutexes;
    std::deque<std::atomic<bool>*> halts;
    std::atomic<int> halted_cycle {0};
    std::deque<T> instruction_memory;
    std::deque<T> data_memory;
    
    void * externalData = nullptr;
    int * PC = nullptr;
    int * Current_Cycle = nullptr;
    
    TaskCallback callback = PipelineLambdaCallback {
        Functions functions(pipeline);
        functions.aquire_indexes_stages_and_flip_flops(index);
        try {
            
            PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "STARTED";
            
            if (functions.index_of_this_stage == 0)
                functions.store_instruction_length(input);

            int val = 0;
            
            bool should_exit = false;
            
            bool should_process_previous_stage = false;
            bool should_process_this_stage = true;
            bool should_process_next_stage = false;
            
            int lastPC = 0;
            int newPC = 0;
            
            while(!should_exit) { // LOOP START
                if (!stage->executedInThisCycle.load()) {
                    if (input->front() == nullptr) {
                        // we do not have input
                        if (output == nullptr ? true : output->front() == nullptr) {
                            // we can halt safely if we have NO input and NO output to send
                            if (functions.index_of_this_stage == 0) {
                                // if the current stage is the very first stage
                                
                                if (functions.program_counter_is_greater_than_instruction_length()) {
                                    // and we have reached the end of our instructions
                                    // then jump to halt
                                    goto halt;
                                }
                            } else {
                                // otherwise
                                if (pipeline->Current_Cycle[0]-1 >= halted_cycle->load() && current_halt->load()) {
                                    // if we have recieved a halt
                                    // then jump to halt
                                    goto halt;
                                }
                            }
                        }
                    } else {
                        // we have input
                        stage->executedInThisCycle.store(true);
                        if (functions.this_stage_is_flip_flop) {
                            // if this stage is a flip flop
                            {
                                // we move the input into the flip-flop
                                
                                PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "pushing flip-flop input";

                                functions.this_flip_flop->push_input(std::move(*input->front()));

                                PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "pushed flip-flop input";
                            }
                            {
                                // consume input
                                PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "consuming normal input";

                                input->pop();

                                PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "consumed normal input";
                            }
                            {
    //                             //
    //                             // NOTE: this WILL cause the stages to execute out of sync
    //                             //
    //                             // emulate clock tick
    //                             // we move the flip-flop's input into its output
    //                             PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "pushing flip-flop output and consuming flip-flop input";
    //                             
    //                             functions.this_flip_flop->exec();
    //                             
    //                             PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "pushed flip-flop output and consumed flip-flop input";
                            }
                            {
                                // and then we move the flip-flop's output into our output
                                if (functions.this_flip_flop->has_output()) {
                                    // make sure we only push if we have output
                                    PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "consuming flip-flop output and pushing normal output";

                                    output->push(std::move(functions.this_flip_flop->pull_output()));

                                    PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "consumed flip-flop output and pushed normal output";
                                }
                            }
                        } else {
                            CHECK_NE(stage->run, nullptr) << "at " << "Stage: " << functions.index_of_this_stage;
                            PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "running stage";
                            
                            // take note of the program counters before and after, as it may or may not change
                            lastPC = *pipeline->PC;
                            stage->run(std::move(val), index, pipeline, input, output);
                            newPC = *pipeline->PC;
                            
                            PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "ran stage";
                            PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << PipelinePrintModifiersPrintValue(input->size());
                            PipelinePrintStageIf(pipeline->debug_output && output != nullptr, functions.index_of_this_stage) << PipelinePrintModifiersPrintValue(output->size());
                            
                            // pop our input based on our current PC
                            //
                            // TODO: divise a way to determine if a stage expects the input
                            //       to be popped based on the current PC
                            //
                            if (functions.index_of_this_stage == 0) {
                                // assume stage 0 manages the current PC
                                if (newPC != lastPC) {
                                    // if our PC has changed, then pop based on the difference
                                    // example: lastPC = 2, PC = 4, pop twice
                                    auto ims = pipeline->instruction_memory.size();
                                    auto is = input->size();
                                    auto diff = newPC - lastPC;
                                    auto diff_diff = diff;
                                    CHECK_LE(newPC, ims)
                                        << "\n\nError: The saved program counter will exceed instruction memory\n\n"
                                        << "Stage: " << functions.index_of_this_stage << "\n"
                                        << "saved program counter: " << newPC << "\n"
                                        << "previous program counter: " << lastPC << "\n"
                                        << "instruction memory size: " << ims << "\n"
                                        << "minimum required input size to complete operation: " << diff << "\n"
                                        << "input size: " << is << "\n";
                                    
                                    int PC = lastPC;
                                    bool timed_out = std::timeout(1s, [&] {
                                        bool ret = false;
                                        if (PC < newPC) {
                                            if (input->front()) {
                                                PC++;
                                                diff_diff--;
                                                PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "popping " << PipelinePrintModifiersPrintValue(*input->front());
                                                input->pop();
                                            };
                                        } else ret = true;
                                        return ret;
                                    });
                                    CHECK_NE(timed_out, true) << "timeout exceeded:\n\n"
                                        << "Stage: " << functions.index_of_this_stage << "\n"
                                        << "saved program counter: " << newPC << "\n"
                                        << "previous program counter: " << lastPC << "\n"
                                        << "instruction memory size: " << ims << "\n"
                                        << "input size needed to complete operation: " << diff_diff << "\n"
                                        << "input size: " << input->size() << "\n";
                                    ;
                                }
                            } else {
                                input->pop();
                            }
                        }
                    }
                }
            } // LOOP END
        } catch (std::exception &ex) {
            PipelineFPrint(ERROR) << std::endl << std::endl << "Caught exception" 
                << ": " << std::endl << std::endl
                << ex.what() << std::endl << std::endl
                // << "    ======= Backtrace: =========" << std::endl << el::base::debug::StackTrace()
            ;
            // should abort or should we let the thread end?
            std::abort();
//             goto end;
        }
    halt:
        // store the current halted cycle so the next stage can halt on the next cycle
        // simply send the HALT signal to the next stage
        PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage) << "HALTING, input size: "
            << (input != nullptr ? input->size() : 0) << ", "
            << "output size: " << (output != nullptr ? output->size() : 0);
        if (input != nullptr) CHECK_EQ(input->size(), 0);
        if (output != nullptr) CHECK_EQ(output->size(), 0);
        next_halt->store(true);
        halted_cycle->store(pipeline->Current_Cycle[0]);
    end:
        PipelinePrintStageIf(pipeline->debug_output, functions.index_of_this_stage)
            << "Terminated, input size: " << (input != nullptr ? input->size() : 0) << ", "
            << "output size: " << (output != nullptr ? output->size() : 0);
        if (input != nullptr) CHECK_EQ(input->size(), 0);
        if (output != nullptr) CHECK_EQ(output->size(), 0);
    };

    void add(Task task) {
        Stage anomynous_stage(debug_output);
        anomynous_stage.run = std::move(task);
        anomynous_stage.type = PipelineStageTypes::Stage;
        stages.push_back(std::move(anomynous_stage));
    }

    void add(Stage && stage) {
        stages.push_back(std::move(stage));
    }
    
    void addFlop() {
        Stage anomynous_stage(debug_output);
        anomynous_stage.type = PipelineStageTypes::Flop;
        stages.push_back(std::move(anomynous_stage));
    }

    Electronics::Flop<T, CAPACITY> & getInputFlopForStage(int stageIndex) {
        auto ss = stages.size();
        for (int i = stageIndex; i < ss && i > -1; i--) {
            if (stages[i].type == PipelineStageTypes::Flop) {
                return stages[i].flop;
            }
        }
    }
    
    std::deque<Electronics::Flop<T, CAPACITY> *> getInputFlopsForStage(int stageIndex) {
        std::deque<Electronics::Flop<T, CAPACITY> *> flops;
        auto ss = stages.size();
        for (int i = stageIndex; i < ss && i > -1; i--) {
            if (stages[i].type == PipelineStageTypes::Flop) {
                flops.push_back(&stages[i].flop);
            } else break;
        }
        return flops;
    }

    Electronics::Flop<T, CAPACITY> & getOutputFlopForStage(int stageIndex) {
        auto ss = stages.size();
        for (int i = stageIndex; i < ss; i++) {
            if (stages[i].type == PipelineStageTypes::Flop) {
                return stages[i].flop;
            }
        }
    }
    
    std::deque<Electronics::Flop<T, CAPACITY> *> getOutputFlopsForStage(int stageIndex) {
        std::deque<Electronics::Flop<T, CAPACITY> *> flops;
        auto ss = stages.size();
        for (int i = stageIndex; i < ss; i++) {
            if (stages[i].type == PipelineStageTypes::Flop) {
                flops.push_back(&stages[i].flop);
            } else break;
        }
        return flops;
    }
    
    Pipeline & run() {
        std::string fmt = "[%TIMESINCESTART] [%logger:";
        fmt += sequential ? "sequential" : "pipelined";
        fmt += ":";
        fmt += debug_output ? "with debug output" : "without debug output";
        fmt += ":%level] %msg";
        config.setGlobally(el::ConfigurationType::Format, fmt);
        el::Loggers::reconfigureLogger("pipeline", config);
        
        program_start.mark();

        auto s = instruction_memory.size();
        queues.push_back(new PipelineQueueType<T>(s));
        
        for (int i = 0; i < s; i++) queues.front()->push(std::move(instruction_memory.at(i)));
        
        auto ss = stages.size();
        
        if (!sequential) {
            conditions.push_back(new std::condition_variable);
            conditions.push_back(new std::condition_variable);
            mutexes.push_back(new std::mutex);
            halts.push_back(new std::atomic<bool>{false});
            for (int i = 0; i < ss; i++) {
                queues.push_back(new PipelineQueueType<T>(CAPACITY));
                conditions.push_back(new std::condition_variable);
                mutexes.push_back(new std::mutex);
                halts.push_back(new std::atomic<bool>{false});
                if (i != 0) {
                    if (stages[i-1].type == PipelineStageTypes::Flop) {
                        // connect the this stages input to the previous flop's output
                        stages[i-1].flop.intermediateOutput = queues[i];
                    }
                }
                pool.push_back(
                    std::thread(
                        callback, this, &stages[i], i,
                        queues[i], i+1 == ss ? nullptr : queues[i+1],
                        halts[i], halts[i+1], &halted_cycle
                    )
                );
                if (i+1 == ss) pool.push_back(
                    std::thread(tickcallback, this, halts[i])
                );
            }
        } else {
            auto qs = queues.front()->size();
            while(*PC < qs) {
                for (int i = 0; i < ss; i++) {
                    CHECK_NE(stages[i].type, PipelineStageTypes::Undefined);
                    if (stages[i].type == PipelineStageTypes::Stage) {
                        int val = 0;
                        
                        PipelineQueueType<T> _input(CAPACITY);
                        PipelineQueueType<T> * input = nullptr;
                        
                        if (i != 0) {
                            if (stages[i-1].type == PipelineStageTypes::Flop) {
                                input = &_input;
                                if (stages[i-1].flop.has_output()) {
                                    input->push(std::move(stages[i-1].flop.pull_output()));
                                }
                            } else {
                                input = stages[i-1].output;
                            }
                        }
                        
                        PipelineQueueType<T> * output = stages[i].output;
                        
                        stages[i].run(std::move(val), i, this, i == 0 ? nullptr : input, output);
                        
                    } else {
                        PipelinePrintIf(debug_output) << "Flop encountered, executing due to sequential mode";
                        PipelineQueueType<T> * input  = i == 0 ? nullptr : stages[i-1].output;
                        PipelineQueueType<T> * output = stages[i].output;
                        if (i != 0) {
                            if (stages[i-1].type == PipelineStageTypes::Flop) {
                                // there may be multiple flip-flops with no stages in between them
                                stages[i].flop.push_input(std::move(stages[i-1].flop.pull_output()));
                            } else {
                                if (stages[i-1].output->front()) {
                                    auto val = *stages[i-1].output->front();
                                    stages[i-1].output->pop();
                                    stages[i].flop.push_input(std::move(val));
                                }
                            }
                        }
                        stages[i].flop.exec();
                    }
                    
                    i+1 == ss ? cycleFunc(this) : cycleFuncNoop(this);
                }
            }
        }
        return *this;
    }

    Pipeline & run(std::deque<T> * input) {
        instruction_memory = *input;
        return run();
    }
    
    Pipeline & run(std::deque<T> input) {
        instruction_memory = input;
        return run();
    }
    
    Pipeline & clear() {
        pool.clear();
        for (auto * queue : queues) delete queue;
        queues.clear();
        for (auto * condition : conditions) delete condition;
        conditions.clear();
        for (auto * mutex : mutexes) delete mutex;
        mutexes.clear();
        for (auto * halt : halts) delete halt;
        halts.clear();
        return *this;
    }
    
    Pipeline & join() {
        if (!sequential) for (auto && thread : pool) thread.join();
        return clear();
    }
};

struct Instructions {
    // load the contents of memory location of ARG into the accumulator
    static const int load = 13;
    // add the contents of memory location ARG to what ever is in the accumulator
    static const int add = 86;
    // store what ever is in the accumulator back back into location ARG
    static const int store = 55;
    
    static const char * toString(int val) {
        #define returncase(val) case val : return #val
        switch(val) {
            returncase(load);
            returncase(add);
            returncase(store);
            default : return "unknown";
        };
        #undef returncase
    }
};

template <typename T>
struct Synchronized_Variable {
    rigtorp::SPSCQueue<T> value = rigtorp::SPSCQueue<T>(1);
    std::condition_variable condition_queue_is_empty;
    std::mutex queue_is_empty_mutex;

    Synchronized_Variable() {} // default initialization
    
    Synchronized_Variable(const Synchronized_Variable & synchronized_Variable) = delete;
    
    Synchronized_Variable(const T & val) {
        store(val);
    };
    
    Synchronized_Variable(Synchronized_Variable && synchronized_Variable) {
        // move constructor
        std::swap(value, synchronized_Variable.value);
    }
    
    Synchronized_Variable & operator=(const Synchronized_Variable & synchronized_Variable) = delete;
    
    Synchronized_Variable & operator=(Synchronized_Variable && synchronized_Variable) {
        // move assign
        std::swap(value, synchronized_Variable.value);
        return *this;
    }
    
    void store(const T & val) {
        value.push(val);
        condition_queue_is_empty.notify_one();
    }
    
    T & peek() {
        std::unique_lock<std::mutex> l (queue_is_empty_mutex);
        condition_queue_is_empty.wait(l, [&] {
            // return false to keep waiting
            // if value->front() returns nullptr
            // then the expression evaluates to false
            // because it is equal to nullptr
            return value.front() != nullptr;
        });
        T * ptr = value.front();
        return *ptr;
    }
    
    T & load() {
        T & val = peek();
        value.pop();
        return val;
    }
};

void twoStagedPipeline(bool sequential) {
    Pipeline<int, 1> pipeline(false);
    
    // two-staged pipeline
    
    // during the cycle "stage" the flop is triggered, stores its input into its output, and then the cycle ends, at which point a new fetch begins
    
    struct registers {
        int clocktmp = 0;
        int clock = 0;
        int clock_last = 0;
        int PC = 0;
        int * CIR = 0;
        int * CIRPlusOne = 0;
        Synchronized_Variable<int> MAR;
        Synchronized_Variable<int *> MDR;
        Synchronized_Variable<int *> MDRPlusOne;
        int ACC = 0;
    } a;
    
    pipeline.externalData = &a;
    pipeline.PC = &a.PC;
    pipeline.Current_Cycle = &a.clock;
    
    // in a 5 stage pipeline, PC is stored in a latch before being sent to fetch
    
    pipeline.cycleFunc = PipelineCycleLambda(p) {
        struct registers * reg = static_cast<struct registers*>(p->externalData);
        PipelinePrint << "--- clock: " << reg->clock << ": Cycle END             ---";
        PipelinePrint << "--- clock: " << reg->clock << ": Cycle Sub-Stage BEGIN ---";
        for (auto & stage : p->stages) if (stage.type == PipelineStageTypes::Flop) {
            stage.flop.exec();
        }
        PipelinePrint << "--- clock: " << reg->clock << ": Cycle Sub-Stage END   ---";
        reg->clock_last = reg->clock;
        reg->clock++;
        PipelinePrint << "--- clock: " << reg->clock << ": Cycle BEGIN           ---";
    };
    
//     pipeline.add(PipelineLambda(val, i, p) {
//         struct registers * reg = static_cast<struct registers*>(p->externalData);
//         output->push(reg->PC + 2);
//     });
    // TODO: pipeline.addFlop(number_of_flop_inputs, number_of_flop_outputs), PipelineFlopLambda(p, flop) {});
    // TODO: ability to name stages, and locate them by name
    // TODO: ability to manually specity inputs and outputs for next stage inside lambda
//     pipeline.addFlop();
    pipeline.add(PipelineLambda(val, i, p) {
        struct registers * reg = static_cast<struct registers*>(p->externalData);
        
        PipelinePrintStage(i) << "clock: " << reg->clock << ": fetch BEGIN, PC: " << reg->PC
        << ", " << PipelinePrintModifiersPrintValue(p->instruction_memory);
        
        // for efficiency, load the value immediately after storing it
        
        reg->MAR.store(reg->PC);
        int memoryAddressRegisterValue = reg->MAR.load();
        
        // MDR is written here
        PipelinePrintStage(i) << "clock: " << reg->clock << ": storing MDR with address of instruction memory location " << memoryAddressRegisterValue;
        int * addr1 = &p->instruction_memory.at(memoryAddressRegisterValue);
        reg->MDR.store(addr1);
        PipelinePrintStage(i) << "clock: " << reg->clock << ": stored MDR with address " << addr1;
        PipelinePrintStage(i) << "clock: " << reg->clock << ": loading MDR";
        int * memoryDataRegisterValue = reg->MDR.load();
        PipelinePrintStage(i) << "clock: " << reg->clock << ": loaded MDR with address " << memoryDataRegisterValue;
        
        reg->MDRPlusOne.store(&p->instruction_memory.at(memoryAddressRegisterValue+1));
        int * memoryDataRegisterValuePlusOne = reg->MDRPlusOne.load();
        
        // MDR is loaded here
        reg->CIR = memoryDataRegisterValue;
        reg->CIRPlusOne = memoryDataRegisterValuePlusOne;
        reg->PC += 2;
        output->push(*memoryDataRegisterValuePlusOne);
        PipelinePrintStage(i) << "clock: " << reg->clock << ": fetch END";
    });
    pipeline.addFlop();
    pipeline.add(PipelineLambda(val, i, p) {
        struct registers * reg = static_cast<struct registers*>(p->externalData);
        PipelinePrintStage(i) << "clock: " << reg->clock << ": decode BEGIN";
        
        reg->MAR.store(*input->front());
        int memoryAddressRegisterValue = reg->MAR.load();
        
        // MDR is written here
        PipelinePrintStage(i) << "clock: " << reg->clock << ": storing MDR with address of data memory location " << memoryAddressRegisterValue;
        int * addr2 = &p->data_memory.at(memoryAddressRegisterValue);
        reg->MDR.store(addr2);
        PipelinePrintStage(i) << "clock: " << reg->clock << ": stored MDR with address " << addr2;
        
        PipelinePrintStage(i) << "clock: " << reg->clock << ": decode END: "
            << Instructions::toString(*reg->CIR);
        
        output->push(*reg->CIR);
        
        PipelinePrintStage(i) << "clock: " << reg->clock << ": output: " << *reg->CIR;
    });
    pipeline.addFlop();
    pipeline.add(PipelineLambda(val, i, p) {
        struct registers * reg = static_cast<struct registers*>(p->externalData);
        int o = *input->front();
        PipelinePrintStage(i) << "clock: " << reg->clock << ": execute BEGIN, input: " << o << ", "
            << "pipeline memory: " << p->data_memory << ", ACC: " << reg->ACC;
            
        // MDR is loaded here
        PipelinePrintStage(i) << "clock: " << reg->clock << ": loading MDR";
        int * memoryDataRegisterValue = reg->MDR.load();
        PipelinePrintStage(i) << "clock: " << reg->clock << ": loaded MDR with address " << memoryDataRegisterValue;
        
        switch(o) {
            case Instructions::load: {
                reg->ACC = *memoryDataRegisterValue;
                PipelinePrintStage(i) << PipelinePrintModifiersPrintValue(reg->ACC);
                break;
            }
            case Instructions::add: {
                // add instruction is passed to ALU
                // contents of accumulator are moved to another place ready to be worked with
                int tmp = reg->ACC;
                reg->ACC = *memoryDataRegisterValue;
                
                // ALU add ACC and tmp together and store it in ACC
                reg->ACC += tmp;
                break;
            }
            case Instructions::store: {
                *memoryDataRegisterValue = reg->ACC;
                break;
            }
            default: break;
        }
        PipelinePrintStage(i) << "clock: " << reg->clock << ": execute END,   "
            << "pipeline memory: " << p->data_memory << ", ACC: " << reg->ACC;
    });
    
    pipeline.sequential = sequential;
    pipeline.manual_increment = true;
    
    pipeline.instruction_memory = {
        // load the contents of memory location of 0 into the accumulator
        Instructions::load, 0,
        // add the contents of memory location 1 to what ever is in the accumulator
        Instructions::add, 1,
        // store what ever is in the accumulator back back into location 2
        Instructions::store, 2
    };
    
    pipeline.data_memory = {
        1,
        2,
        0
    };
    
    pipeline.run().join();
}

int main() {
    twoStagedPipeline(false);
    return 0;
}
