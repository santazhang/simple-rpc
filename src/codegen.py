#!/usr/bin/env python

import os

# ensure CWD is the dir of this script
os.chdir(os.path.dirname(__file__))

def expand_str(template, vals, prefix='', postfix='', sep=', '):
    if len(vals) > 0:
        return prefix + sep.join([template % v for v in vals]) + postfix
    else:
        return ""

print "codegen on rpc.h (RpcClient)"
f = open("rpc.h")
lines = f.readlines()
f.close();

f = open("rpc.h", "w")
found_autogen = False
found_autogen_end = False
for l in lines:
    if not found_autogen:
        f.write(l)
        if l.startswith("// __AUTO_GEN_CLIENT__"):
            found_autogen = True
            n_autogen = int(l[l.index("(") + 1 : l.rindex(")")])
            print "gen count = %d" % n_autogen
            for n in range(n_autogen):
                f.write("\n")
                f.write("    template<class R%s>\n" % expand_str("class T%d", range(n), prefix=', '))
                f.write("    i32 call(i32 svc_id, %sR& r) {\n" % expand_str("const T%d& t%d", zip(range(n), range(n)), postfix=', '))
                f.write("        i32 ret;\n")
                f.write("        Marshal req, *rep;\n")
                f.write("        i64 xid = xid_counter_.next();\n")
                f.write("        req << xid << svc_id%s;\n" % expand_str("t%d", range(n), sep=' << ', prefix=' << '))
                f.write("        ret = call(xid, req, rep);\n")
                f.write("        if (ret >= 0) { // No RPC error\n")
                f.write("            *rep >> r;\n")
                f.write("        }\n")
                f.write("        delete rep;\n")
                f.write("        return ret;\n");
                f.write("    }\n")
            f.write("\n")
    elif not found_autogen_end:
        if l.startswith("// __AUTO_GEN_CLIENT_END__"):
            f.write(l)
            found_autogen_end = True
    else:
        f.write(l)
f.close()
assert found_autogen and found_autogen_end

print "codegen on rpc.h (RpcServer)"
f = open("rpc.h")
lines = f.readlines()
f.close();

f = open("rpc.h", "w")
found_autogen = False
found_autogen_end = False
for l in lines:
    if not found_autogen:
        f.write(l)
        if l.startswith("// __AUTO_GEN_SERVER__"):
            found_autogen = True
            n_autogen = int(l[l.index("(") + 1 : l.rindex(")")])
            print "gen count = %d" % n_autogen
            for n in range(n_autogen):
                f.write("\n")
                f.write("    template<class S, class R%s>\n" % expand_str("class T%d", range(n), prefix=', '))
                f.write("    void reg(i32 svc_id, S* svc, i32 (S::*svc_func)(%sR& r)) {\n" % expand_str("const T%d& t%d", zip(range(n), range(n)), postfix=', '))
                f.write("        class H%d: public Handler {\n" % n)
                f.write("            S* s;\n")
                f.write("            i32 (S::*f)(%sR& r);\n" % expand_str("const T%d&", range(n), postfix=', '))
                f.write("        public:\n")
                f.write("            H%d(S* s, i32 (S::*f)(%sR& r)): s(s), f(f) {}\n" % (n, expand_str("const T%d&", range(n), postfix=', ')))
                f.write("            void handle(Marshal& req, Marshal& rep) {\n")
                f.write(expand_str("                T%d t%d;\n", zip(range(n), range(n)), sep=''))
                f.write(expand_str("t%d", range(n), sep=' >> ', prefix="                req >> ", postfix=';\n'))
                f.write("                R r;\n")
                f.write("                i32 ret = (s->*f)(%sr);\n" % expand_str("t%d", range(n), postfix=', '))
                f.write("                VERIFY(ret >= 0); // Negative error code are reserved for RPC error.\n")
                f.write("                rep << ret << r;\n")
                f.write("            }\n")
                f.write("        };\n")
                f.write("        handlers_[svc_id] = new H%d(svc, svc_func);\n" % n)
                f.write("    }\n")
            f.write("\n")
    elif not found_autogen_end:
        if l.startswith("// __AUTO_GEN_SERVER_END__"):
            f.write(l)
            found_autogen_end = True
    else:
        f.write(l)
f.close()
assert found_autogen and found_autogen_end

