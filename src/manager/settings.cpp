// Copyright (C) 2016-2018 Egor Pugin <egor.pugin@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "settings.h"

#include "database.h"
#include "directories.h"
#include "exceptions.h"
#include "hash.h"
#include "stamp.h"

#include <boost/algorithm/string.hpp>

#include <primitives/hasher.h>
#include <primitives/templates.h>

#include <fstream>

#include <primitives/log.h>
DECLARE_STATIC_LOGGER(logger, "settings");

#define CPPAN_LOCAL_BUILD_PREFIX "sw-build-"
#define CONFIG_ROOT "/etc/sw/"

namespace sw
{

Directories &getDirectoriesUnsafe();

Settings::Settings()
{
    build_dir = temp_directory_path() / "build";
    storage_dir = get_root_directory() / STORAGE_DIR;
}

void Settings::load(const path &p, const SettingsType type)
{
    auto root = load_yaml_config(p);
    load(root, type);
}

void Settings::load(const yaml &root, const SettingsType type)
{
    load_main(root, type);

    auto get_storage_dir = [this](SettingsType type)
    {
        switch (type)
        {
        /*case SettingsType::Local:
            return cppan_dir / STORAGE_DIR;*/
        case SettingsType::User:
            return Settings::get_user_settings().storage_dir;
        case SettingsType::System:
            return Settings::get_system_settings().storage_dir;
        default:
        {
            auto d = fs::absolute(storage_dir);
            fs::create_directories(d);
            return fs::canonical(d);
        }
        }
    };

    auto get_build_dir = [](const path &p, SettingsType type, const auto &dirs)
    {
        switch (type)
        {
        case SettingsType::Local:
            return current_thread_path();
        case SettingsType::User:
        case SettingsType::System:
            return dirs.storage_dir_tmp / "build";
        default:
            return p;
        }
    };

    Directories dirs;
    dirs.storage_dir_type = storage_dir_type;
    auto sd = get_storage_dir(storage_dir_type);
    dirs.set_storage_dir(sd);
    dirs.build_dir_type = build_dir_type;
    dirs.set_build_dir(get_build_dir(build_dir, build_dir_type, dirs));

    getDirectoriesUnsafe().update(dirs, type);
}

void Settings::load_main(const yaml &root, const SettingsType type)
{
    auto packages_dir_type_from_string = [](const String &s, const String &key) {
        if (s == "local")
            return SettingsType::Local;
        if (s == "user")
            return SettingsType::User;
        if (s == "system")
            return SettingsType::System;
        throw std::runtime_error("Unknown '" + key + "'. Should be one of [local, user, system]");
    };

    get_map_and_iterate(root, "remotes", [this](auto &kv) {
        auto n = kv.first.template as<String>();
        bool o = n == DEFAULT_REMOTE_NAME; // origin
        Remote rm;
        Remote *prm = o ? &remotes[0] : &rm;
        prm->name = n;
        String provider;
        YAML_EXTRACT_VAR(kv.second, prm->url, "url", String);
        //YAML_EXTRACT_VAR(kv.second, prm->data_dir, "data_dir", String);
        YAML_EXTRACT_VAR(kv.second, provider, "provider", String);
        if (!provider.empty())
        {
            /*if (provider == "sw")
                prm->default_source = &Remote::sw_source_provider;*/
        }
        YAML_EXTRACT_VAR(kv.second, prm->user, "user", String);
        YAML_EXTRACT_VAR(kv.second, prm->token, "token", String);
        if (!o)
            remotes.push_back(*prm);
    });

    YAML_EXTRACT_AUTO(disable_update_checks);
    YAML_EXTRACT(storage_dir, String);
    YAML_EXTRACT(build_dir, String);
    YAML_EXTRACT(output_dir, String);

    auto &p = root["proxy"];
    if (p.IsDefined())
    {
        if (!p.IsMap())
            throw std::runtime_error("'proxy' should be a map");
        YAML_EXTRACT_VAR(p, proxy.host, "host", String);
        YAML_EXTRACT_VAR(p, proxy.user, "user", String);
    }

    storage_dir_type = packages_dir_type_from_string(get_scalar<String>(root, "storage_dir_type", "user"), "storage_dir_type");
    /*if (root["storage_dir"].IsDefined())
        storage_dir_type = SettingsType::None;*/
    build_dir_type = packages_dir_type_from_string(get_scalar<String>(root, "build_dir_type", "system"), "build_dir_type");
    /*if (root["build_dir"].IsDefined())
        build_dir_type = SettingsType::None;*/
}

bool Settings::is_custom_build_dir() const
{
    return build_dir_type == SettingsType::Local/* || build_dir_type == SettingsType::None*/;
}

bool Settings::checkForUpdates() const
{
    if (disable_update_checks)
        return false;

#ifdef _WIN32
    String stamp_file = "/client/.service/win32.stamp";
#elif __APPLE__
    String stamp_file = "/client/.service/macos.stamp";
#else
    String stamp_file = "/client/.service/linux.stamp";
#endif

    auto fn = fs::temp_directory_path() / unique_path();
    download_file(remotes[0].url + stamp_file, fn);
    auto stamp_remote = boost::trim_copy(read_file(fn));
    fs::remove(fn);
    boost::replace_all(stamp_remote, "\"", "");
    uint64_t s1 = cppan_stamp.empty() ? 0 : std::stoull(cppan_stamp);
    uint64_t s2 = std::stoull(stamp_remote);
    if (!(s1 != 0 && s2 != 0 && s2 > s1))
        return false;

    LOG_INFO(logger, "New version of the CPPAN client is available!");
    LOG_INFO(logger, "Feel free to upgrade it from website (https://cppan.org/) or simply run:");
    LOG_INFO(logger, "cppan --self-upgrade");
#ifdef _WIN32
    LOG_INFO(logger, "(or the same command but from administrator)");
#else
    LOG_INFO(logger, "or");
    LOG_INFO(logger, "sudo cppan --self-upgrade");
#endif
    LOG_INFO(logger, "");
    return true;
}

Settings &Settings::get(SettingsType type)
{
    static Settings settings[toIndex(SettingsType::Max) + 1];

    auto i = toIndex(type);
    auto &s = settings[i];
    switch (type)
    {
    case SettingsType::Local:
    {
        RUN_ONCE
        {
            s = get(SettingsType::User);
        };
    }
    break;
    case SettingsType::User:
    {
        RUN_ONCE
        {
            s = get(SettingsType::System);

            auto fn = get_config_filename();
            if (!fs::exists(fn))
            {
                error_code ec;
                fs::create_directories(fn.parent_path(), ec);
                if (ec)
                    throw std::runtime_error(ec.message());
                auto ss = get(SettingsType::System);
                ss.save(fn);
            }
            s.load(fn, SettingsType::User);
        };
    }
    break;
    case SettingsType::System:
    {
        RUN_ONCE
        {
            auto fn = CONFIG_ROOT "default";
            if (!fs::exists(fn))
                break;
            s.load(fn, SettingsType::System);
        };
    }
    break;
    }
    return s;
}

Settings &Settings::get_system_settings()
{
    return get(SettingsType::System);
}

Settings &Settings::get_user_settings()
{
    return get(SettingsType::User);
}

Settings &Settings::get_local_settings()
{
    return get(SettingsType::Local);
}

void Settings::clear_local_settings()
{
    get_local_settings() = get_user_settings();
}

void Settings::save(const path &p) const
{
    std::ofstream o(p);
    if (!o)
        throw std::runtime_error("Cannot open file: " + p.string());
    yaml root;
    root["remotes"][DEFAULT_REMOTE_NAME]["url"] = remotes[0].url;
    root["storage_dir"] = storage_dir.string();
    o << dump_yaml_config(root);
}

void cleanConfig(const String &c)
{
    if (c.empty())
        return;

    auto h = hash_config(c);

    auto remove_pair = [&](const auto &dir) {
        fs::remove_all(dir / c);
        fs::remove_all(dir / h);
    };

    remove_pair(getDirectories().storage_dir_bin);
    remove_pair(getDirectories().storage_dir_lib);
    //remove_pair(getDirectories().storage_dir_exp);
#ifdef _WIN32
    //remove_pair(getDirectories().storage_dir_lnk);
#endif

    // for cfg we also remove xxx.cmake files (found with xxx.c.cmake files)
    remove_pair(getDirectories().storage_dir_cfg);
    for (auto &f : boost::make_iterator_range(fs::directory_iterator(getDirectories().storage_dir_cfg), {}))
    {
        if (!fs::is_regular_file(f) || f.path().extension() != ".cmake")
            continue;
        auto parts = split_string(f.path().string(), ".");
        if (parts.size() == 2)
        {
            if (parts[0] == c || parts[0] == h)
                fs::remove(f);
            continue;
        }
        if (parts.size() == 3)
        {
            if (parts[1] == c || parts[1] == h)
            {
                fs::remove(parts[0] + ".cmake");
                fs::remove(parts[1] + ".cmake");
                fs::remove(f);
            }
            continue;
        }
    }

    // obj
    auto &sdb = getServiceDatabase();
    for (auto &p : sdb.getInstalledPackages())
    {
        auto d = p.getDirObj() / "build";
        if (!fs::exists(d))
            continue;
        for (auto &f : boost::make_iterator_range(fs::directory_iterator(d), {}))
        {
            if (!fs::is_directory(f))
                continue;
            if (f.path().filename() == c || f.path().filename() == h)
                fs::remove_all(f);
        }
    }

    // config hashes (in sdb)
    sdb.removeConfigHashes(c);
    sdb.removeConfigHashes(h);
}

void cleanConfigs(const Strings &configs)
{
    for (auto &c : configs)
        cleanConfig(c);
}

}
