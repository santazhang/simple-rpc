#!/usr/bin/env python

import sys
import os
sys.path += os.path.abspath(os.path.join(os.path.split(__file__)[0], "..")),

%%

parser RpcDef:
    ignore: '\\s+'
    ignore: '//[^\\n]+'

    token EOF: '$'
    token IDENTIFIER: '[$a-zA-Z0-9_][$a-zA-Z0-9_]*'

    rule rpc_def: {{ namespace = None }} [ns_decl {{ namespace = ns_decl }}] def_list EOF
        {{ return namespace, def_list }}

    rule ns_decl: "namespace" IDENTIFIER
        {{ return IDENTIFIER }}

    rule def_list: {{ struct_def_list = []; service_def_list = [] }}
        (struct_def {{ struct_def_list += struct_def, }} | service_def {{ service_def_list += service_def, }} )*
        {{ return  struct_def_list, service_def_list }}

    rule struct_def: "struct" IDENTIFIER "{" field_list "}"
        {{ return IDENTIFIER, field_list }}

    rule field_list: {{ field_list = [] }}
        (field_def {{ field_list += field_def,}} )+
        {{ return field_list }}

    rule field_def: type_info IDENTIFIER
        {{ return type_info, IDENTIFIER }}

    rule type_info:
        string_type {{ return string_type }}
        | vector_type {{ return vector_type }}
        | list_type {{ return list_type }}
        | set_type {{ return set_type }}
        | deque_type {{ return deque_type }}
        | map_type {{ return map_type }}
        | "i32" {{ return "rpc::i32" }}
        | "i64" {{ return "rpc::i64" }}
        | IDENTIFIER {{ return IDENTIFIER }}

    rule string_type: "string"
        {{ return "std::string" }}

    rule vector_type: "vector" "<" type_info ">"
        {{ return "std::vector<%s >" % type_info }}

    rule list_type: "list" "<" type_info ">"
        {{ return "std::list<%s >" % type_info }}

    rule set_type: "set" "<" type_info ">"
        {{ return "std::set<%s >" % type_info }}

    rule deque_type: "deque" "<" type_info ">"
        {{ return "std::deque<%s >" % type_info }}

    rule map_type: {{ key_type = None; value_type = None }}
        "map" "<" (type_info {{ key_type = type_info }} ) ',' (type_info {{ value_type = type_info }} ) ">"
        {{ return "std::map<%s, %s >" % (key_type, value_type) }}

    rule service_def: "service" IDENTIFIER "{" func_list "}"
        {{ return IDENTIFIER, func_list }}

    rule func_list: {{ func_list = [] }}
        (func_def {{ func_list += func_def, }} )+
        {{ return func_list }}

    rule func_def: {{ in_args = None; out_args = None; f_attr = None}}
        [func_attr {{ f_attr = func_attr }} ] IDENTIFIER "\("
            (arg_list {{ in_args = arg_list }} ) [";" (arg_list {{ out_args = arg_list }} )] "\)"
        {{ return IDENTIFIER, f_attr, in_args, out_args }}

    rule func_attr: "fast" {{ return "fast" }}
        |"raw" {{ return "raw" }}

    rule arg_list:
        {{ a_list = [] }}
        | func_arg {{ a_list = [func_arg, ] }} (',' func_arg {{ a_list += func_arg, }} )*
        {{ return a_list }}

    rule func_arg: {{ ident = None }}
        type_info [IDENTIFIER {{ ident = IDENTIFIER }} ]
        {{ return type_info, ident }}

%%

g_rpc_counter = 0x1001;

def space(n):
    return " " * n

def write_ln(f, line="\n", indent=0):
    if line == "":
        return
    f.write(space(indent))
    f.write(line)
    if not line.endswith("\n"):
        f.write('\n')

def gen_struct_def(struct_def, h):
    name, fields = struct_def
    write_ln(h, "struct %s {" % name)
    for field in fields:
        write_ln(h, "%s %s;" % (field[0], field[1]), 4)
    write_ln(h, "};")
    write_ln(h)
    write_ln(h, "inline rpc::Marshal& operator <<(rpc::Marshal& m, const %s& o) {" % name)
    for field in fields:
        write_ln(h, "m << o.%s;" % field[1], 4)
    write_ln(h, "return m;", 4);
    write_ln(h, "}")
    write_ln(h)
    write_ln(h, "inline rpc::Marshal& operator >>(rpc::Marshal& m, %s& o) {" % name)
    for field in fields:
        write_ln(h, "m >> o.%s;" % field[1], 4)
    write_ln(h, "return m;", 4);
    write_ln(h, "}")
    write_ln(h)

def gen_struct_def_list(struct_def_list, h):
    for struct_def in struct_def_list:
        gen_struct_def(struct_def, h)

def gen_service_def(service_def, h):
    global g_rpc_counter

    svc_name, members = service_def
    write_ln(h, "class %sService: public rpc::Service {" % svc_name)
    write_ln(h, "public:")
    write_ln(h, "enum {", 4)
    for member in members:
        func_name = member[0]
        write_ln(h, func_name.upper() + ' = ' + hex(g_rpc_counter) + ',', 8)
        g_rpc_counter += 1
    write_ln(h, "};", 4)
    write_ln(h)
    write_ln(h, "void reg_to(rpc::Server* svr) {", 4)
    for member in members:
        func_name, func_attr = member[0], member[1]
        if func_attr == "raw":
            write_ln(h, "svr->reg(%s, this, &%sService::%s);" % (func_name.upper(), svc_name, func_name) , 8)
        else:
            write_ln(h, "svr->reg(%s, this, &%sService::__%s__wrapped__);" % (func_name.upper(), svc_name, func_name) , 8)
    write_ln(h, "}", 4)

    write_ln(h)
    write_ln(h, "private:")

    for member in members:
        func_name, func_attr, in_args, out_args = member[0], member[1], member[2], member[3]
        func_args = []
        if func_attr == "raw":
            continue
        write_ln(h, "void __%s__wrapped__(rpc::Request* req, rpc::ServerConnection* sconn) {" % func_name, 4)
        in_count = 0
        out_count = 0
        if func_attr == "fast":
            call_args = []
            for in_arg in in_args:
                write_ln(h, "%s in_%d;" % (in_arg[0], in_count), 8)
                write_ln(h, "req->m >> in_%d;" % in_count, 8)
                call_args += "in_%d" % in_count,
                in_count += 1
            for out_arg in out_args:
                write_ln(h, "%s out_%d;" % (out_arg[0], out_count), 8)
                call_args += "&out_%d" % out_count,
                out_count += 1

            write_ln(h, "this->%s(%s);" % (func_name, ", ".join(call_args)), 8)
            write_ln(h, "sconn->begin_reply(req);", 8)
            for i in range(out_count):
                write_ln(h, "*sconn << out_%d;" % i, 8)
            write_ln(h, "sconn->end_reply();", 8)
            write_ln(h, "delete req;", 8)
            write_ln(h, "sconn->release();", 8)
        else:
            write_ln(h, "class R: public rpc::Runnable {", 8)
            write_ln(h, "%sService* __thiz__;" % svc_name, 12)
            write_ln(h, "rpc::Request* __req__;", 12)
            write_ln(h, "rpc::ServerConnection* __sconn__;", 12)
            write_ln(h, "public:", 8)
            write_ln(h, "R(%sService* thiz, rpc::Request* r, rpc::ServerConnection* sc): __thiz__(thiz), __req__(r), __sconn__(sc) {}" % svc_name, 12)
            write_ln(h, "void run() {", 12)
            call_args = []
            for in_arg in in_args:
                write_ln(h, "%s in_%d;" % (in_arg[0], in_count), 16)
                write_ln(h, "__req__->m >> in_%d;" % in_count, 16)
                call_args += "in_%d" % in_count,
                in_count += 1
            for out_arg in out_args:
                write_ln(h, "%s out_%d;" % (out_arg[0], out_count), 16)
                call_args += "&out_%d" % out_count,
                out_count += 1

            write_ln(h, "__thiz__->%s(%s);" % (func_name, ", ".join(call_args)), 16)
            write_ln(h, "__sconn__->begin_reply(__req__);", 16)
            for i in range(out_count):
                write_ln(h, "*__sconn__ << out_%d;" % i, 16)
            write_ln(h, "__sconn__->end_reply();", 16)
            write_ln(h, "delete __req__;", 16)
            write_ln(h, "__sconn__->release();", 16)
            write_ln(h, "}", 12)
            write_ln(h, "};", 8)
            write_ln(h, "sconn->run_async(new R(this, req, sconn));", 8)
        write_ln(h, "", 8)
        write_ln(h, "}", 4)
        write_ln(h)

    write_ln(h, "public:")
    write_ln(h, "// these member functions need to be implemented by user", 4)
    for member in members:
        func_name, func_attr, in_args, out_args = member[0], member[1], member[2], member[3]
        func_args = []
        if func_attr == "raw":
            write_ln(h, "// NOTE: remember to reply req, delete req, and sconn->release(); use sconn->run_async for heavy job", 4)
            write_ln(h, "virtual void %s(rpc::Request* req, rpc::ServerConnection* sconn);" % func_name, 4)
            continue

        if in_args != None:
            for in_arg in in_args:
                func_arg = "const %s&" % in_arg[0]
                if in_arg[1] != None:
                    func_arg += " %s" % in_arg[1]
                func_args += func_arg,
        if out_args != None:
            for out_arg in out_args:
                func_arg = "%s*" % out_arg[0]
                if out_arg[1] != None:
                    func_arg += " %s" % out_arg[1]
                func_args += func_arg,
        write_ln(h, "virtual void %s(%s);" % (func_name, ", ".join(func_args)), 4)


    write_ln(h)
    write_ln(h, "}; // class %sService" % svc_name)
    write_ln(h)

    write_ln(h, "class %sProxy {" % svc_name)
    write_ln(h, "rpc::Client* __cl__;", 4)
    write_ln(h, "public:")
    write_ln(h, "%sProxy(rpc::Client* cl): __cl__(cl) {}" % svc_name, 4)

    for member in members:

        func_name, func_attr, in_args, out_args = member[0], member[1], member[2], member[3]
        if func_attr == "raw":
            continue

        write_ln(h)
        func_args = []
        async_args = []
        in_count = 0
        out_count = 0
        if in_args != None:
            for in_arg in in_args:
                func_arg = "const %s&" % in_arg[0]
                if in_arg[1] != None:
                    func_arg += " %s" % in_arg[1]
                else:
                    func_arg += " in_%d" % in_count
                func_args += func_arg,
                async_args += func_arg,
                in_count += 1
        if out_args != None:
            for out_arg in out_args:
                func_arg = "%s*" % out_arg[0]
                if out_arg[1] != None:
                    func_arg += " %s" % out_arg[1]
                else:
                    func_arg += " out_%d" % out_count
                func_args += func_arg,
                out_count += 1
        write_ln(h, "rpc::i32 %s(%s) {" % (func_name, ", ".join(func_args)), 4)
        call_args = []
        in_count = 0
        if in_args != None:
            for in_arg in in_args:
                if in_arg[1] != None:
                    call_args += "%s" % in_arg[1],
                else:
                    call_args += "in_%d" % in_count,
                in_count += 1
        write_ln(h, "rpc::Future* __fu__ = async_%s(%s);" % (func_name, ", ".join(call_args)), 8)
        write_ln(h, "if (__fu__ == NULL) {", 8)
        write_ln(h, "return ENOTCONN;", 12)
        write_ln(h, "}", 8)
        write_ln(h, "rpc::i32 __ret__ = __fu__->get_error_code();", 8)
        write_ln(h, "if (__ret__ == 0) {", 8)
        out_count = 0
        if out_args != None:
            for out_arg in out_args:
                if out_arg[1] != None:
                    write_ln(h, "__fu__->get_reply() >> *%s;" % out_arg[1], 12)
                else:
                    write_ln(h, "__fu__->get_reply() >> *out_%d;" % out_count, 12)
                out_count += 1
        write_ln(h, "}", 8)
        write_ln(h, "__fu__->release();", 8)
        write_ln(h, "return __ret__;", 8)
        write_ln(h, "}", 4)
        write_ln(h)

        write_ln(h, "rpc::Future* async_%s(%s) {" % (func_name, ", ".join(async_args)), 4)
        write_ln(h, "rpc::Future* __fu__ = __cl__->begin_request();", 8)
        write_ln(h, "if (__fu__ != NULL) {", 8)
        write_ln(h, "rpc::i32 __rpc_id__ = %sService::%s;" % (svc_name, func_name.upper()), 12)
        write_ln(h, "*__cl__ << __rpc_id__;", 12)
        in_count = 0
        if in_args != None:
            for in_arg in in_args:
                if in_arg[1] != None:
                    write_ln(h, "*__cl__ << %s;" % in_arg[1], 12)
                else:
                    write_ln(h, "*__cl__ << in_%d;" % in_count, 12)
                in_count += 1
        write_ln(h, "}", 8)
        write_ln(h, "__cl__->end_request();", 8)
        write_ln(h, "return __fu__;", 8)
        write_ln(h, "}", 4)

    write_ln(h)

    for member in members:
        func_name, func_attr = member[0], member[1]
        if func_attr == "raw":
            write_ln(h, "// raw rpc '%s' not included" % func_name, 4)

    write_ln(h)
    write_ln(h, "}; // class %sProxy" % svc_name)
    write_ln(h)



def gen_service_def_list(service_def_list, h):
    for service_def in service_def_list:
        gen_service_def(service_def, h)

def gen_rpc_def(rpc_def, h):
    ns, rpc_def_body = rpc_def
    struct_def_list, service_def_list = rpc_def_body
    if ns != None:
        write_ln(h, "namespace %s {" % ns)
        write_ln(h)
        gen_struct_def_list(struct_def_list, h)
        gen_service_def_list(service_def_list, h)
        write_ln(h, "} // namespace %s" % ns)
    else:
        gen_struct_def_list(struct_def_list, h)
        gen_service_def_list(service_def_list, h)


def expand_str(template, vals, prefix='', postfix='', sep=', '):
    if len(vals) > 0:
        return prefix + sep.join([template % v for v in vals]) + postfix
    else:
        return ""

def range2(v):
    return zip(range(v), range(v))


def rpc_gen(rpc_fpath):
    f = open(rpc_fpath)
    rpc_def = parse("rpc_def", f.read())
    f.close()
    h_fpath = os.path.splitext(rpc_fpath)[0] + '.h'
    h = open(h_fpath, 'w')

    write_ln(h, "// this file is generated from '%s'" % os.path.split(rpc_fpath)[1])
    write_ln(h, "// make sure you have included server.h and client.h before including this file")
    write_ln(h)
    write_ln(h, "#pragma once")
    write_ln(h)
    write_ln(h, "#include <errno.h>")
    write_ln(h)

    gen_rpc_def(rpc_def, h)
    h.close()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "usage: %s <rpc-def-file>" % sys.argv[0]
        exit(1)
    rpc_gen(sys.argv[1])
