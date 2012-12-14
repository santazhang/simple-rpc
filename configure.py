#!/usr/bin/env python

# Makefile generator for c/c++ projects
#
# Author: Santa Zhang <santa1987@gmail.com>

import os
import sys
from time import *

CONF_NAME = "configure.py.conf"
if os.path.exists(CONF_NAME) is False:
    CONF_NAME = os.path.join(os.path.split(__file__)[0], "configure.py.conf")
if os.path.exists(CONF_NAME) is False:
    print "*** error: 'configure.py.conf' not found! place it in current working directory, or in the same directory with 'configure.py'!"
    exit(1)

print "using configure file: '%s'" % CONF_NAME


_gen_mkf = None
def _gen_open_file(fn):
    global _gen_mkf
    _gen_mkf = open(fn, "w")

def _gen_close_file():
    global _gen_mkf
    _gen_mkf.close()
    _gen_mkf = None


_gen_last_line = ""
def _gen_line(line):
    global _gen_last_line
    global _gen_mkf
    if not (_gen_last_line == "" and line == ""):
        #print line
        _gen_mkf.write(line)
        _gen_mkf.write("\n")
    _gen_last_line = line

def gen(txt = ""):
    splt = txt.split("\n")
    for line in splt:
        _gen_line(line)

class define_str:
    def __init__(self, key, value):
        self.key = key
        self.value = value

class config_h:
    def __init__(self, fpath):
        self.fpath = fpath
    def do(self, *defs):
        f = open(self.fpath, "w")
        f.write("#pragma once\n")
        f.write("\n")
        for def_item in defs:
            if def_item.__class__ == define_str:
                f.write("#define %s \"%s\"\n\n" % (def_item.key, def_item.value))
            else:
                print "*** error: unsupported class '%s' in config_h() call!" % def_item.__class__.__name__
                exit(1)
        print "generated config file '%s'" % self.fpath
        f.close()

class cflags:
    def __init__(self, cflags_val):
        self.val = cflags_val
    def __str__(self):
        return self.val

class cc:
    def __init__(self, cc_val):
        self.val = cc_val
    def __str__(self):
        return self.val

class ld:
    def __init__(self, ld_val):
        self.val = ld_val
    def __str__(self):
        return self.val

class ldflags:
    def __init__(self, ldflags_val):
        self.val = ldflags_val
    def __str__(self):
        return self.val


all_src = []

class src:
    def __init__(self, *paths):
        self.val = paths
        global all_src
        all_src.extend(paths)
    def get_src(self):
        return self.val





all_target = [] # target_name only, ordered
all_target_dict = {}  # target_name -> target

class target:

    def __init__(self, name):
        self.name = name
        self.dep_targets = []
        self.settings = {}
        self.cmds = []
        self.description = ""
        global all_target
        all_target += name,
        all_target_dict[name] = self

    def do(self, *settings):
        for s in settings:
            if isinstance(s, cc):
                self.settings["cc"] = s
            elif isinstance(s, cflags):
                self.settings["cflags"] = s
            elif isinstance(s, ld):
                self.settings["ld"] = s
            elif isinstance(s, ldflags):
                self.settings["ldflags"] = s
            elif isinstance(s, src):
                if self.settings.has_key("src") == False:
                    self.settings["src"] = []
                self.settings["src"].extend(s.get_src())
            else:
                raise Exception("Setting not supported: %s", str(type(s)))
            if self.settings.has_key("src"):
                if self.settings.has_key("cflags"):
                    for s in self.settings["src"]:
                        self.settings["cflags"] = cflags(("-I%s " % s) + str(self.settings["cflags"]))
        return self

    def describe(self, description):
        self.description = description
        return self

    def depend(self, *dep_targets):
        self.dep_targets.extend(dep_targets)
        return self

    def _gen_var_def(self):
        if self.settings.has_key("src") == False:
            return
        if self.settings.has_key("cc"):
            gen("CC_%s=%s" % (self.name, self.settings["cc"]))
        else:
            gen("CC_%s=gcc" % (self.name))

        if self.settings.has_key("cflags"):
            gen("CFLAGS_%s=%s" % (self.name, self.settings["cflags"]))
        else:
            gen("CFLAGS_%s=" % (self.name))

        if self.settings.has_key("ld"):
            gen("LD_%s=%s" % (self.name, self.settings["ld"]))
        else:
            gen("LD_%s=gcc" % (self.name))

        if self.settings.has_key("ldflags"):
            gen("LDFLAGS_%s=%s" % (self.name, self.settings["ldflags"]))
        else:
            gen("LDFLAGS_%s=" % (self.name))
        gen()

    def _gen_dep_cmd(self):
        if self.settings.has_key("src") == False:
            if len(self.dep_targets) > 0:
                gen("%s:%s" % (self.name, " ".join(self.dep_targets)))
                gen()
            return

        # forward declaration for possible default tasks
        gen("_%s: %s" % (self.name, self.name))
        gen()

        hpaths = []
        cpaths = []
        hfn_dep = {} # h fn -> dep fn
        cfn_dep = {} # c fn -> dep fn (h file)
        hfn_path = {} # h filename -> full path
        cfn_path = {} # c filename -> full path
        for s in self.settings["src"]:
            if os.path.isdir(s):
                for root, dirnames, fnames in os.walk(s):
                    for fn in fnames:
                        fpath = os.path.join(root, fn)
                        if fpath.endswith(".c") or fpath.endswith(".cpp") or fpath.endswith(".cxx") or fpath.endswith(".cc"):
                            cpaths += fpath,
                            if cfn_path.has_key(fn):
                                raise Exception("Duplicate file: '%s' in '%s' and '%s'" % (fn, fpath, cfn_path[fn]))
                            cfn_path[fn] = fpath
                        elif fpath.endswith(".h"):
                            hpaths += fpath,
                            if hfn_path.has_key(fn):
                                raise Exception("Duplicate file: '%s' in '%s' and '%s'" % (fn, fpath, hfn_path[fn]))
                            hfn_path[fn] = fpath
            else:
                # s is file
                if s.endswith(".c") or s.endswith(".cpp") or s.endswith(".cxx") or s.endswith(".cc"):
                    cpaths += s,
                    if cfn_path.has_key(fn):
                        raise Exception("Duplicate file: '%s' in '%s' and '%s'" % (fn, fpath, cfn_path[fn]))
                    cfn_path[fn] = fpath
                elif s.endswith(".h"):
                    hpaths += s,
                    if hfn_path.has_key(fn):
                        raise Exception("Duplicate file: '%s' in '%s' and '%s'" % (fn, fpath, hfn_path[fn]))
                    hfn_path[fn] = fpath

        # determine hfn_dep
        for hfp in hpaths:
            hfn = os.path.split(hfp)[1]
            hfn_dep[hfn] = set()
            hf = open(hfp, "r")
            for l in hf.readlines():
                l = l.strip()
                if l.startswith("#include ") and l.endswith('"'):
                    idx = l.index('"') + 1
                    idx2 = l.rindex('"')
                    dep_hfn = l[idx:idx2].strip()
                    dep_hfn = os.path.split(dep_hfn)[1]
                    hfn_dep[hfn].add(dep_hfn)
            hf.close()

        # determine propagated dependency
        for hfp in hpaths:
            hfn = os.path.split(hfp)[1]
            visited = set()
            while True:
                if len(visited) == len(hfn_dep[hfn]):
                    break
                new_item = None
                for dep_hfn in hfn_dep[hfn]:
                    dep_hfn = os.path.split(dep_hfn)[1]
                    if dep_hfn not in visited:
                        new_item = dep_hfn
                        break
                if new_item != None:
                    visited.add(new_item)
                    if hfn_dep.has_key(new_item):
                        for dep_dep_hfn in hfn_dep[new_item]:
                            hfn_dep[hfn].add(dep_dep_hfn)
                    else:
                        print "Header file '%s' not found while analyzing file '%s'" % (new_item, hfn)
                        exit(1)

        # generate final result
        for hfp in hpaths:
            hfn = os.path.split(hfp)[1]
            lst = list(hfn_dep[hfn])
            lst.sort()
            #print hfn, lst

        for cfp in cpaths:
            cdep = set()
            cf = open(cfp, "r")
            for l in cf.readlines():
                l = l.strip()
                if l.startswith("#include ") and l.endswith('"'):
                    idx = l.index('"') + 1
                    idx2 = l.rindex('"')
                    dep_hfp = l[idx:idx2].strip()
                    dep_hfn = os.path.split(dep_hfp)[1]
                    cdep.add(dep_hfn)
                    for hdep in hfn_dep[dep_hfn]:
                        cdep.add(hdep)

            #print cfp, cdep
            cf.close()

            # generate build command foreach .o file
            cfn = os.path.split(cfp)[1]
            lst = list(cdep)
            lst.sort()
            lst2 = []
            for e in lst:
                lst2 += hfn_path[e],
            gen("obj-%s/%s.o: obj-%s/.place_holder %s " % (self.name, cfn, self.name, cfp) + " ".join(lst2))
            gen("\t@echo \"  [cc] %s\" ; $(CC_%s) $(CFLAGS_%s) -c %s -o obj-%s/%s.o" % (cfp, self.name, self.name, cfp, self.name, cfn))
            gen()

            cfn_dep[cfn] = cdep

        main_cpaths = set()


        for cfp in cpaths:
            cf = open(cfp, "r")
            has_main = False
            for l in cf.readlines():
                l = l.strip()
                if l.startswith("main(") or l.startswith("int main(") or l.startswith("void main("):
                    has_main = True
                    main_cpaths.add(cfp)
                    break
            cf.close()

            # determine indirect dependency
            if has_main:
                cfn = os.path.split(cfp)[1]
                #print "has main:", cfp, cfn_dep[cfn]
                bin_fn = os.path.splitext(cfn)[0]
                #print "bin_fn =", bin_fn
                visited = set()

                ext_hdep = cfn_dep[cfn].copy()
                #print cfp, ext_hdep

                while True:
                    new_item = None
                    for e in ext_hdep:
                        if e not in visited:
                            visited.add(e)
                            new_item = e
                            break
                    if new_item != None:
                        hfp = hfn_path[new_item]
                        cfn = os.path.splitext(os.path.split(hfp)[1])[0] + ".c"
                        if cfn_dep.has_key(cfn):
                            #print cfn, cfn_dep[cfn]
                            for e in cfn_dep[cfn]:
                                #print e
                                ext_hdep.add(e)
                    else:
                        break

                obj_files = set()
                cfn = os.path.split(cfp)[1]
                obj_files.add("obj-%s/%s.o" % (self.name, cfn))

                for hfp in ext_hdep:
                    cfn = os.path.splitext(os.path.split(hfp)[1])[0] + ".c"
                    if cfn_path.has_key(cfn):
                        obj_files.add("obj-%s/%s.o" % (self.name, cfn))
                    ccfn = os.path.splitext(os.path.split(hfp)[1])[0] + ".cc"
                    if cfn_path.has_key(ccfn):
                        obj_files.add("obj-%s/%s.o" % (self.name, ccfn))


                obj_files = list(obj_files)
                obj_files.sort()

                #print cfp, cdep
                gen("bin-%s/%s: obj-%s/.place_holder %s " % (self.name, bin_fn, self.name, cfp) + " ".join(obj_files))

                gen("\t@echo \"  [ld] %s\" ; $(LD_%s) $(CFLAGS_%s) %s $(LDFLAGS_%s) -o bin-%s/%s" % (bin_fn, self.name, self.name, " ".join(obj_files), self.name, self.name, bin_fn))
                gen()

        #print main_cpaths
        bin_targets = []
        lst = list(main_cpaths)
        lst.sort()
        for e in lst:
            e = os.path.splitext(os.path.split(e)[1])[0]
            bin_targets += "bin-%s/%s" % (self.name, e),


        gen("%s: bin-%s/.place_holder %s" % (self.name, self.name, " ".join(bin_targets)))
        gen()

        gen("bin-%s/.place_holder:" % self.name)
        gen("\t@mkdir -p bin-%s" % self.name)
        gen("\t@touch bin-%s/.place_holder" % self.name)
        gen()

        gen("obj-%s/.place_holder:" % self.name)
        gen("\t@mkdir -p obj-%s" % self.name)
        gen("\t@touch obj-%s/.place_holder" % self.name)
        gen()


def _gen_mkfile_head():
    gen("# Makefile automatically generated by configure.py at %s" % asctime())
    gen()


_default_target_dep = []


def _gen_mkfile_tail():
    global _default_target_dep
    gen("clean:")
    for t in all_target:
        if all_target_dict[t].settings.has_key("src"):
            gen("\trm -rf bin-%s obj-%s" % (t, t))
    gen()
    gen("dist:")
    gen("\t@./configure.py dist")
    gen()
    gen("help:")
    gen("\t@echo \"Available targets:\"")
    gen("\t@echo")
    max_t_name_len = 5
    for t in all_target:
        if len(t) > max_t_name_len:
            max_t_name_len = len(t)
    t_description_spcing = 4
    for t in all_target:
        #if all_target_dict[t].settings.has_key("src"):
        spcing = " " * (t_description_spcing + max_t_name_len - len(t))
        default_target_flag = " "
        if t in _default_target_dep:
            default_target_flag = "*"
        gen("\t@echo \" %s %s%s%s\"" % (default_target_flag, t, spcing, all_target_dict[t].description))

    spcing = " " * (t_description_spcing + max_t_name_len - len("clean"))
    default_target_flag = " "
    if "clean" in _default_target_dep:
        default_target_flag = "*"
    gen("\t@echo \" %s clean%sclean all build files and directory\"" % (default_target_flag, spcing))

    spcing = " " * (t_description_spcing + max_t_name_len - len("dist"))
    default_target_flag = " "
    if "dist" in _default_target_dep:
        default_target_flag = "*"
    gen("\t@echo \" %s dist%smake distribution packages\"" % (default_target_flag, spcing))

    spcing = " " * (t_description_spcing + max_t_name_len - len("help"))
    default_target_flag = " "
    if "help" in _default_target_dep:
        default_target_flag = "*"
    gen("\t@echo \" %s help%sshow this help message\"" % (default_target_flag, spcing))

    if len(default_target_flag) > 0:
        gen("\t@echo")
        gen("\t@echo \"Targets marked with '*' is default build target\"")

def _gen_default_target():
    global _default_target_dep
    if len(_default_target_dep) > 0:
        gen("_default: " + " ".join(_default_target_dep))
        gen()


def default_target(*defa_targets):
    global _default_target_dep
    _default_target_dep.extend(defa_targets)



def _public_gen_mkfile():
    _gen_mkfile_head()

    for t in all_target:
        all_target_dict[t]._gen_var_def()

    _gen_default_target()

    for t in all_target:
        print "generating targets for '%s'" % t
        all_target_dict[t]._gen_dep_cmd()

    _gen_mkfile_tail()


def _public_print_help():
    print "script to automatically generate Makefile for C/C++ projects"
    print
    print "  ./configure.py         generate Makefile, with configures loaded from 'configure.py.conf'"
    print "  ./configure.py help    display this info"
    print "  ./configure.py dist    make a distribution package (git repo only)"
    print
    print "author: Santa Zhang (santa1987@gmail.com)"

def _git_is_dirty():
    pipe = os.popen("git status")
    lines = pipe.readlines()
    pipe.close()
    if lines[1].startswith("#"):
        return True
    else:
        return False

def _public_make_dist():
    if _git_is_dirty():
        print "*** warning: making dist packages on a dirty tree, only latest committed code tree will be packed"

    tag_name = None
    commit_id = None

    def my_exec_read_one_line(cmd):
        pipe = os.popen(cmd)
        line = pipe.readline()
        pipe.close()
        return line

    def get_tags():
        pipe = os.popen("git tag")
        tags = []
        for line in pipe.readlines():
            tags += line.strip(),
        pipe.close()
        return tags

    # try to get the tag name from git show
    current_commit_hash = my_exec_read_one_line("git show")
    tags = get_tags()
    for tag in tags:
        tagged_commit_hash = my_exec_read_one_line("git show %s" % tag)
        if tagged_commit_hash == current_commit_hash:
            tag_name = tag
            break

    if tag_name == None:
        # no tag name found, we use the 'commit-id' instead (first 7 chars in hash)
        line = my_exec_read_one_line("git log")
        commit_id = line.split()[1][:7]

    # get the project name
    proj_name = None
    check_dir = os.path.split(os.path.realpath("configure.py"))[0]
    while check_dir != "/":
        if ".git" in os.listdir(check_dir):
            proj_name = os.path.split(check_dir)[1]
            source_root = check_dir
            break
        check_dir = os.path.split(check_dir)[0]

    if proj_name == None:
        print "*** error: cannot find .git directory in any parent directory! not a valid git repository!"
        exit(1)

    if tag_name != None:
        tar_fn = "%s/%s-%s.tar" % (source_root, proj_name, tag_name)
    else:
        tar_fn = "%s/%s-snapshot-%s.tar" % (source_root, proj_name, commit_id)


    if os.path.exists(tar_fn):
        os.remove(tar_fn)
    if os.path.exists(tar_fn + ".gz"):
        os.remove(tar_fn + ".gz")
    cmd = "git archive --format tar --prefix %s/ -o %s HEAD" % (os.path.splitext(os.path.split(tar_fn)[1])[0], tar_fn)
    print "[cmd] %s" % cmd
    os.system(cmd)

    cmd = "gzip %s" % tar_fn
    print "[cmd] %s" % cmd
    os.system(cmd)

#
# Entry section
#

# add project targets
execfile(CONF_NAME)

# after reading the config file, change dir!
os.chdir(os.path.split(__file__)[0])

if len(sys.argv) > 1:
    if sys.argv[1] == "dist":
        _public_make_dist()
    elif sys.argv[1] == "help" or sys.argv[1] == "-h" or sys.argv[1] == "--help":
        _public_print_help()
    else:
        print "*** error: option '%s' not understood!" % sys.argv[1]
else:
    _gen_open_file("Makefile")
    _public_gen_mkfile()
    _gen_close_file()

