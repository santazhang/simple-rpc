// # simple-rpc
//
// Simple RPC in C++, with Python binding
//
// https://github.com/santazhang/simple-rpc
// http://www.yzhang.net/blog/2013-05-31-simple-rpc-1.html

// ## Define your RPC

// First, write your `.rpc` file.
/* demo.rpc */

// Optional namespace for your RPC.
namespace demo;

// Create an RPC service called `Demo`.
service Demo {
    // Add an RPC method called `sayhi`, with one `string` type input parameter called `hi`
    sayhi(string hi);

    // Add an RPC method called `sum`, which adds up `a`, `b` and `c`, and stores the result in `result`.
    // Note that integer size is explicity specified (`i32` for 32 bit integer).
    // The `|` sign separates input and output parameters.
    sum(i32 a, i32 b, i32 c | i32 result);

    // There is also `i8`, `i16` and `i64`.
    is_prime(i64 num | i8 yes_or_no);

    // If your method does not have input parameter.
    calendar_date( | string month, i32 day, i32 year);

    // Or maybe neither input nor output?
    nop();

    // STL types are also supported:
    // `pair`, `vector`, `list`, `set`, `map`, `unordered_set`, `unordered_map`
    sort(set<string> input | vector<string> sorted_output);
};

// Want to use custom types? No problem :)
struct point3 {
    double x;
    double y;
    double z;
};

service Math {
    euclidean_distance(point3 a, point3 b | double result);
};

// ## Generate RPC code

// Run `bin/rpcgen` against the `.rpc` file you wrote.
//
//     bin/rpcgen demo.rpc
//
// A `.h` file will be generated for you, which contains RPC code stub.
// It contains all the `struct`, and both client side and
// server side code stub for the `service` you've defined.

// For the above `demo.rpc` example, a `demo.h` will be generated.
// In this file, you will see something like this:
class DemoService: public rpc::Service {
    /* ... */
    virtual void sayhi(const std::string& hi);
    virtual void sum(const rpc::i32& a, const rpc::i32& b, const rpc::i32& c, rpc::i32* result);
    virtual void is_prime(const rpc::i64& num, rpc::i8* yes_or_no);
    virtual void calendar_date(std::string* month, rpc::i32* day, rpc::i32* year);
    virtual void nop();
    virtual void sort(const std::set<std::string>& input, std::vector<std::string>* sorted_output);
    /* ... */
};

// Those functions are to be implemented by you. Write a `demo.cc` file, `#include` the generated
// `demo.h`, and fill out the functions.

/* demo.cc */
#include "demo.h"

namespace demo {

/* ... */

void DemoService::sum(const rpc::i32& a, const rpc::i32& b, const rpc::i32& c, rpc::i32* result) {
    *result = a + b + c;
}

/* ... */

}

// ## Start your RPC server

// `#include` the simple-rpc library code and your RPC code.
// Create the `rpc::Server` object, register the service you defined, and
// start the RPC server!
#include "rpc/server.h"
#include "demo.h"

int main() {
    rpc::Server server;
    demo::DemoService demo_svc;
    server.reg(&demo_svc);
    server.start("127.0.0.1:8848");
    for (;;) {
        sleep(1);
    }
    return 0;
}

// ## Send RPC requests

// In the automatically generated `demo.h` file, you will find `DemoProxy` class.
// It makes RPC call very easy to use.

// `#include` the simple-rpc library code and your RPC code.
// Use `rpc::ClientPool` class to create connection to RPC server,
// and use it to create a `DemoProxy` instance.
#include "rpc/client.h"
#include "demo.h"

int main() {
    rpc::ClientPool clnt_pool;
    demo::DemoProxy demo(clnt_pool.get_client("127.0.0.1:8848"));

    // Now you can easily call the RPC functions.
    demo.sayhi("hello RPC");

    i32 a = 1, b = 2, c = 3;
    i32 result;
    demo.sum(a, b, c, &result);
    printf("%d + %d + %d = %d\n", a, b, c, result);
    return 0;
}

// ## More features to be covered

// simple-rpc also supports the following features, which will be documented soon.
// You can also checkout the code in `test` source code folder for real world usages.
// * Set number of worker threads used in `rpc::Server`.
// * Supporting custom `struct`/`class` (defined outside `.rpc` file).
// * Asynchronous RPC, which could be used to send a batch of RPC requests without blocking each other.
//   You can wait for their completion, or set a callback function to be notified.
// * If an RPC handler is very short, process it directly, without delegating to a worker thread.
// * Writing server/client in Python!
