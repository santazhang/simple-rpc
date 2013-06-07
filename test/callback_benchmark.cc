#include <iostream>
#include <tr1/functional>

#include "rpc/callback.h"
#include "rpc/utils.h"
#include "test/test_util.h"

namespace {

using std::tr1::bind;
using std::tr1::function;
using rpc::Callback;
using rpc::makeCallableOnce;
using rpc::makeCallable;
using rpc::Timer;
using test::Counter;

void SimpleCall() {
  const int REPEATS = 1000000;

  // comparing base, tr1, makeCallableOnce, makeCallable variations
  Timer timers[4];
  Counter counters[4];

  // use direct call as base
  Counter& c = counters[0];
  timers[0].start();
  for (int i=0; i<REPEATS; i++) {
    c.inc();
  }
  timers[0].end();

  // tr1 variation
  typedef function<void()> Tr1Callback;
  Tr1Callback* cb1 = new Tr1Callback(bind(&Counter::inc, &counters[1]));
  timers[1].start();
  for (int i=0; i<REPEATS; i++) {
    (*cb1)();
  }
  timers[1].end();
  delete cb1;

  // makeCallableOnce variation
  Callback<void>* cb2[REPEATS];
  for (int i=0; i<REPEATS; i++) {
    cb2[i] = makeCallableOnce(&Counter::inc, &counters[2]);
  }
  timers[2].start();
  for (int i=0; i<REPEATS; i++) {
    (*cb2[i])();
  }
  timers[2].end();

  // makeCallable variation
  Callback<void>* cb3 = makeCallable(&Counter::inc, &counters[3]);
  timers[3].start();
  for (int i=0; i<REPEATS; i++) {
    (*cb3)();
  }
  timers[3].end();
  delete cb3;

  std::cout << "SimpleCall (base|tr1|once|many): "
            << timers[0].elapsed() << " | "
            << timers[1].elapsed() << " | "
            << timers[2].elapsed() << " | "
            << timers[3].elapsed() << std::endl;
}

}  // unnamed namespace

int main(int argc, char* argv[]) {
  SimpleCall();
}
