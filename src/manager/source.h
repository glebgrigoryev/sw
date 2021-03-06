// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "cppan_version.h"
#include "property_tree.h"

#include <nlohmann/json_fwd.hpp>
#include <primitives/stdcompat/variant.h>

namespace YAML { class Node; }
using yaml = YAML::Node;

namespace sw
{

struct EmptySource
{
    EmptySource() = default;
    EmptySource(const yaml &root, const String &name = EmptySource::getString()) {}

    bool empty() const { return true; }
    bool isValid(const String &name, String *error = nullptr) const { return true; }
    bool isValidUrl() const { return true; }

    bool load(const ptree &p) { return true; }
    bool save(ptree &p) const { return true; }
    bool load(const nlohmann::json &p) { return true; }
    bool save(nlohmann::json &p) const { return true; }
    void save(yaml &root, const String &name = EmptySource::getString()) const {}

    String print() const { return ""; }
    void applyVersion(const Version &v) {}
    void download() const {}

    static String getString() { return "empty"; }

    bool operator==(const EmptySource &rhs) const
    {
        return true;
    }
};

struct SW_MANAGER_API SourceUrl
{
    String url;

    SourceUrl() = default;
    SourceUrl(const String &url);
    SourceUrl(const yaml &root, const String &name);

    bool empty() const { return url.empty(); }
    bool isValid(const String &name, String *error = nullptr) const;
    bool isValidUrl() const;
    bool load(const ptree &p);
    bool save(ptree &p) const;
    bool load(const nlohmann::json &p);
    bool save(nlohmann::json &p) const;
    void save(yaml &root, const String &name) const;
    String print() const;
    void applyVersion(const Version &v);

protected:
    template <typename ... Args>
    bool checkValid(const String &name, String *error, Args && ... args) const;
};

struct SW_MANAGER_API Git : SourceUrl
{
    String tag;
    String branch;
    String commit;

    Git() = default;
    Git(const String &url, const String &tag = "", const String &branch = "", const String &commit = "");
    Git(const yaml &root, const String &name = Git::getString());

    void download() const;
    bool isValid(String *error = nullptr) const;
    bool load(const ptree &p);
    bool save(ptree &p) const;
    bool load(const nlohmann::json &p);
    bool save(nlohmann::json &p) const;
    void save(yaml &root, const String &name = Git::getString()) const;
    String print() const;
    void applyVersion(const Version &v);

    bool operator==(const Git &rhs) const
    {
        return std::tie(url, tag, branch, commit) == std::tie(rhs.url, rhs.tag, rhs.branch, rhs.commit);
    }

    static String getString() { return "git"; }
};

struct SW_MANAGER_API Hg : Git
{
    int64_t revision = -1;

    Hg() = default;
    Hg(const yaml &root, const String &name = Hg::getString());

    void download() const;
    bool isValid(String *error = nullptr) const;
    bool load(const ptree &p);
    bool save(ptree &p) const;
    bool load(const nlohmann::json &p);
    bool save(nlohmann::json &p) const;
    void save(yaml &root, const String &name = Hg::getString()) const;
    String print() const;

    bool operator==(const Hg &rhs) const
    {
        return std::tie(url, tag, branch, commit, revision) == std::tie(rhs.url, rhs.tag, rhs.branch, rhs.commit, rhs.revision);
    }

    static String getString() { return "hg"; }
};

struct SW_MANAGER_API Bzr : SourceUrl
{
    String tag;
    int64_t revision = -1;

    Bzr() = default;
    Bzr(const yaml &root, const String &name = Bzr::getString());

    void download() const;
    bool isValid(String *error = nullptr) const;
    bool load(const ptree &p);
    bool save(ptree &p) const;
    bool load(const nlohmann::json &p);
    bool save(nlohmann::json &p) const;
    void save(yaml &root, const String &name = Bzr::getString()) const;
    String print() const;

    bool operator==(const Bzr &rhs) const
    {
        return std::tie(url, tag, revision) == std::tie(rhs.url, rhs.tag, rhs.revision);
    }

    static String getString() { return "bzr"; }
};

struct SW_MANAGER_API Fossil : Git
{
    Fossil() = default;
    Fossil(const yaml &root, const String &name = Fossil::getString());

    void download() const;
    using Git::save;
    void save(yaml &root, const String &name = Fossil::getString()) const;

    bool operator==(const Fossil &rhs) const
    {
        return std::tie(url, tag, branch, commit) == std::tie(rhs.url, rhs.tag, rhs.branch, rhs.commit);
    }

    static String getString() { return "fossil"; }
};

struct SW_MANAGER_API Cvs : SourceUrl
{
    String tag;
    String branch;
    String revision;
    String module;

    Cvs() = default;
    Cvs(const yaml &root, const String &name = Cvs::getString());

    void download() const;
    bool isValid(String *error = nullptr) const;
    bool isValidUrl() const;
    bool load(const ptree &p);
    bool save(ptree &p) const;
    bool load(const nlohmann::json &p);
    bool save(nlohmann::json &p) const;
    void save(yaml &root, const String &name = Cvs::getString()) const;
    String print() const;
    String printCpp() const;

    bool operator==(const Cvs &rhs) const
    {
        return std::tie(url, tag, branch, revision, module) == std::tie(rhs.url, rhs.tag, rhs.branch, rhs.revision, rhs.module);
    }

    static String getString() { return "cvs"; }
};

struct Svn : SourceUrl
{
    String tag;
    String branch;
    int64_t revision = -1;

    Svn() = default;
    Svn(const yaml &root, const String &name = Svn::getString());

    void download() const;
    bool isValid(String *error = nullptr) const;
    bool load(const ptree &p);
    bool save(ptree &p) const;
    bool load(const nlohmann::json &p);
    bool save(nlohmann::json &p) const;
    void save(yaml &root, const String &name = Svn::getString()) const;
    String print() const;
    String printCpp() const;

    bool operator==(const Svn &rhs) const
    {
        return std::tie(url, tag, branch, revision) == std::tie(rhs.url, rhs.tag, rhs.branch, rhs.revision);
    }

    static String getString() { return "svn"; }
};

struct SW_MANAGER_API RemoteFile : SourceUrl
{
    RemoteFile() = default;
    RemoteFile(const String &url);
    RemoteFile(const yaml &root, const String &name = RemoteFile::getString());

    void download() const;
    using SourceUrl::save;
    void save(yaml &root, const String &name = RemoteFile::getString()) const;
    void applyVersion(const Version &v);

    bool operator==(const RemoteFile &rhs) const
    {
        return url == rhs.url;
    }

    static String getString() { return "remote"; }
};

struct SW_MANAGER_API RemoteFiles
{
    StringSet urls;

    RemoteFiles() = default;
    RemoteFiles(const yaml &root, const String &name = RemoteFiles::getString());

    void download() const;
    bool empty() const { return urls.empty(); }
    bool isValidUrl() const;
    bool load(const ptree &p);
    bool save(ptree &p) const;
    bool load(const nlohmann::json &p);
    bool save(nlohmann::json &p) const;
    void save(yaml &root, const String &name = RemoteFiles::getString()) const;
    String print() const;
    void applyVersion(const Version &v);

    bool operator==(const RemoteFiles &rhs) const
    {
        return urls == rhs.urls;
    }

    static String getString() { return "files"; }
};

// TODO: add: svn, cvs, darcs, p4
// do not add local files
#define SOURCE_TYPES(f,d) \
    f(EmptySource) d \
    f(Git) d \
    f(Hg) d \
    f(Bzr) d \
    f(Fossil) d \
    f(Cvs) d \
    f(Svn) d \
    f(RemoteFile) d \
    f(RemoteFiles)

#define DELIM_COMMA ,
#define DELIM_SEMICOLON ;
#define SOURCE_TYPES_EMPTY(x) x
using Source = variant<SOURCE_TYPES(SOURCE_TYPES_EMPTY, DELIM_COMMA)>;
#undef SOURCE_TYPES_EMPTY

SW_MANAGER_API
void download(const Source &source, int64_t max_file_size = 0);

/// load from global object with 'source' subobject
SW_MANAGER_API
Source load_source(const ptree &p);

/// save to global object with 'source' subobject
SW_MANAGER_API
void save_source(ptree &p, const Source &source);

/// load from current (passed) object
SW_MANAGER_API
Source load_source(const nlohmann::json &j);

/// save to current (passed) object
SW_MANAGER_API
void save_source(nlohmann::json &j, const Source &source);

/// load from global object with 'source' subobject
SW_MANAGER_API
bool load_source(const yaml &root, Source &source);

/// save to global object with 'source' subobject
SW_MANAGER_API
void save_source(yaml &root, const Source &source);

SW_MANAGER_API
String print_source(const Source &source);

SW_MANAGER_API
String get_source_hash(const Source &source);

SW_MANAGER_API
bool isValidSourceUrl(const Source &source);

SW_MANAGER_API
void applyVersionToUrl(Source &source, const Version &v);

}

namespace std
{

template<> struct hash<sw::Source>
{
    size_t operator()(const sw::Source& p) const
    {
        return hash<String>()(sw::print_source(p));
    }
};

}
