/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2016 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "settings.h"
#include "preprocessor.h"       // Preprocessor
#include "utils.h"
#include "tinyxml2.h"
#include "path.h"
#include "tokenlist.h"

#include <fstream>
#include <set>

Settings::Settings()
    : _terminated(false),
      debug(false),
      debugnormal(false),
      debugwarnings(false),
      dump(false),
      exceptionHandling(false),
      inconclusive(false),
      jointSuppressionReport(false),
      experimental(false),
      quiet(false),
      inlineSuppressions(false),
      verbose(false),
      force(false),
      relativePaths(false),
      xml(false), xml_version(1),
      jobs(1),
      loadAverage(0),
      exitCode(0),
      showtime(SHOWTIME_NONE),
      preprocessOnly(false),
      maxConfigs(12),
      enforcedLang(None),
      reportProgress(false),
      checkConfiguration(false),
      checkLibrary(false)
{
    // This assumes the code you are checking is for the same architecture this is compiled on.
#if defined(_WIN64)
    platform(Win64);
#elif defined(_WIN32)
    platform(Win32A);
#else
    platform(Native);
#endif
}

namespace {
    const std::set<std::string> id = make_container< std::set<std::string> > ()
                                     << "warning"
                                     << "style"
                                     << "performance"
                                     << "portability"
                                     << "information"
                                     << "missingInclude"
                                     << "unusedFunction"
#ifdef CHECK_INTERNAL
                                     << "internal"
#endif
                                     ;
}
std::string Settings::addEnabled(const std::string &str)
{
    // Enable parameters may be comma separated...
    if (str.find(',') != std::string::npos) {
        std::string::size_type prevPos = 0;
        std::string::size_type pos = 0;
        while ((pos = str.find(',', pos)) != std::string::npos) {
            if (pos == prevPos)
                return std::string("cppcheck: --enable parameter is empty");
            const std::string errmsg(addEnabled(str.substr(prevPos, pos - prevPos)));
            if (!errmsg.empty())
                return errmsg;
            ++pos;
            prevPos = pos;
        }
        if (prevPos >= str.length())
            return std::string("cppcheck: --enable parameter is empty");
        return addEnabled(str.substr(prevPos));
    }

    if (str == "all") {
        std::set<std::string>::const_iterator it;
        for (it = id.begin(); it != id.end(); ++it) {
            if (*it == "internal")
                continue;

            _enabled.insert(*it);
        }
    } else if (id.find(str) != id.end()) {
        _enabled.insert(str);
        if (str == "information") {
            _enabled.insert("missingInclude");
        }
    } else {
        if (str.empty())
            return std::string("cppcheck: --enable parameter is empty");
        else
            return std::string("cppcheck: there is no --enable parameter with the name '" + str + "'");
    }

    return std::string("");
}


bool Settings::append(const std::string &filename)
{
    std::ifstream fin(filename.c_str());
    if (!fin.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(fin, line)) {
        _append += line + "\n";
    }
    Preprocessor::preprocessWhitespaces(_append);
    return true;
}

const std::string &Settings::append() const
{
    return _append;
}

bool Settings::platform(PlatformType type)
{
    switch (type) {
    case Unspecified:
        platformType = type;
        sizeof_bool = sizeof(bool);
        sizeof_short = sizeof(short);
        sizeof_int = sizeof(int);
        sizeof_long = sizeof(long);
        sizeof_long_long = sizeof(long long);
        sizeof_float = sizeof(float);
        sizeof_double = sizeof(double);
        sizeof_long_double = sizeof(long double);
        sizeof_wchar_t = sizeof(wchar_t);
        sizeof_size_t = sizeof(std::size_t);
        sizeof_pointer = sizeof(void *);
        defaultSign = '\0';
        char_bit = 8;
        short_bit = char_bit * sizeof_short;
        int_bit = char_bit * sizeof_int;
        long_bit = char_bit * sizeof_long;
        long_long_bit = char_bit * sizeof_long_long;
        return true;
    case Native: // same as system this code was compile on
        platformType = type;
        sizeof_bool = sizeof(bool);
        sizeof_short = sizeof(short);
        sizeof_int = sizeof(int);
        sizeof_long = sizeof(long);
        sizeof_long_long = sizeof(long long);
        sizeof_float = sizeof(float);
        sizeof_double = sizeof(double);
        sizeof_long_double = sizeof(long double);
        sizeof_wchar_t = sizeof(wchar_t);
        sizeof_size_t = sizeof(std::size_t);
        sizeof_pointer = sizeof(void *);
        {
            char x = -1;
            defaultSign = (x < 0) ? 's' : 'u';
        }
        char_bit = 8;
        short_bit = char_bit * sizeof_short;
        int_bit = char_bit * sizeof_int;
        long_bit = char_bit * sizeof_long;
        long_long_bit = char_bit * sizeof_long_long;
        return true;
    case Win32W:
    case Win32A:
        platformType = type;
        sizeof_bool = 1; // 4 in Visual C++ 4.2
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 4;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 8;
        sizeof_wchar_t = 2;
        sizeof_size_t = 4;
        sizeof_pointer = 4;
        defaultSign = '\0';
        char_bit = 8;
        short_bit = char_bit * sizeof_short;
        int_bit = char_bit * sizeof_int;
        long_bit = char_bit * sizeof_long;
        long_long_bit = char_bit * sizeof_long_long;
        return true;
    case Win64:
        platformType = type;
        sizeof_bool = 1;
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 4;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 8;
        sizeof_wchar_t = 2;
        sizeof_size_t = 8;
        sizeof_pointer = 8;
        defaultSign = '\0';
        char_bit = 8;
        short_bit = char_bit * sizeof_short;
        int_bit = char_bit * sizeof_int;
        long_bit = char_bit * sizeof_long;
        long_long_bit = char_bit * sizeof_long_long;
        return true;
    case Unix32:
        platformType = type;
        sizeof_bool = 1;
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 4;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 12;
        sizeof_wchar_t = 4;
        sizeof_size_t = 4;
        sizeof_pointer = 4;
        defaultSign = '\0';
        char_bit = 8;
        short_bit = char_bit * sizeof_short;
        int_bit = char_bit * sizeof_int;
        long_bit = char_bit * sizeof_long;
        long_long_bit = char_bit * sizeof_long_long;
        return true;
    case Unix64:
        platformType = type;
        sizeof_bool = 1;
        sizeof_short = 2;
        sizeof_int = 4;
        sizeof_long = 8;
        sizeof_long_long = 8;
        sizeof_float = 4;
        sizeof_double = 8;
        sizeof_long_double = 16;
        sizeof_wchar_t = 4;
        sizeof_size_t = 8;
        sizeof_pointer = 8;
        defaultSign = '\0';
        char_bit = 8;
        short_bit = char_bit * sizeof_short;
        int_bit = char_bit * sizeof_int;
        long_bit = char_bit * sizeof_long;
        long_long_bit = char_bit * sizeof_long_long;
        return true;
    }

    // unsupported platform
    return false;
}

bool Settings::platformFile(const std::string &filename)
{
    // open file..
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS)
        return false;

    const tinyxml2::XMLElement * const rootnode = doc.FirstChildElement();

    if (!rootnode || std::strcmp(rootnode->Name(),"platform") != 0)
        return false;

    for (const tinyxml2::XMLElement *node = rootnode->FirstChildElement(); node; node = node->NextSiblingElement()) {
        if (std::strcmp(node->Name(), "default-sign") == 0)
            defaultSign = *node->GetText();
        else if (std::strcmp(node->Name(), "char_bit") == 0)
            char_bit = std::atoi(node->GetText());
        else if (std::strcmp(node->Name(), "sizeof") == 0) {
            for (const tinyxml2::XMLElement *sz = node->FirstChildElement(); sz; sz = sz->NextSiblingElement()) {
                if (std::strcmp(node->Name(), "short") == 0)
                    sizeof_short = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "int") == 0)
                    sizeof_int = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "long") == 0)
                    sizeof_long = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "long-long") == 0)
                    sizeof_long_long = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "float") == 0)
                    sizeof_float = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "double") == 0)
                    sizeof_double = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "long-double") == 0)
                    sizeof_long_double = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "pointer") == 0)
                    sizeof_pointer = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "size_t") == 0)
                    sizeof_size_t = std::atoi(node->GetText());
                else if (std::strcmp(node->Name(), "wchar_t") == 0)
                    sizeof_wchar_t = std::atoi(node->GetText());
            }
        }
    }

    short_bit = char_bit * sizeof_short;
    int_bit = char_bit * sizeof_int;
    long_bit = char_bit * sizeof_long;
    long_long_bit = char_bit * sizeof_long_long;

    return true;
}

void Settings::importProject(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open())
        return;
    if (filename == "compile_commands.json") {
        importCompileCommands(fin);
    } else if (filename.find(".vcxproj") != std::string::npos) {
        importVcxproj(filename);
    }
}

void Settings::importCompileCommands(std::istream &istr) {
    std::map<std::string, std::string> values;

    TokenList tokenList(this);
    tokenList.createTokens(istr);
    for (const Token *tok = tokenList.front(); tok; tok = tok->next()) {
      if (Token::Match(tok, "%str% : %str% [,}]")) {
        std::string key = tok->str();
        std::string value = tok->strAt(2);
        values[key.substr(1, key.size() - 2U)] = value.substr(1, value.size() - 2U);
      }

      else if (tok->str() == "}") {
            if (!values["file"].empty() && !values["command"].empty()) {
                struct FileSettings fs;
                fs.filename = Path::fromNativeSeparators(values["file"]);
                std::string command = values["command"];
                std::string::size_type pos = 0;
                while (std::string::npos != (pos = command.find(" ",pos))) {
                    pos++;
                    if (pos >= command.size())
                        break;
                    if (command[pos] != '/' && command[pos] != '-')
                        continue;
                    pos++;
                    if (pos >= command.size())
                      break;
                    char F = command[pos++];
                    std::string fval;
                    while (pos < command.size() && command[pos] != ' ')
                        fval += command[pos++];
                    if (F=='D')
                        fs.defines += fval + ";";
                    else if (F=='U')
                        fs.undefs.insert(fval);
                    else if (F=='I')
                        fs.includePaths.push_back(fval);
                }
                fileSettings.push_back(fs);
            }
            values.clear();
        }
    }
}

namespace {
  struct ProjectConfiguration {
    ProjectConfiguration(const tinyxml2::XMLElement *cfg) {
        for (const tinyxml2::XMLElement *e = cfg->FirstChildElement(); e; e = e->NextSiblingElement()) {
            if (std::strcmp(e->Name(),"Configuration")==0)
                configuration = e->GetText();
            else if (std::strcmp(e->Name(),"Platform")==0)
                platform = e->GetText();
        }
    }
    std::string configuration;
    std::string platform;
  };

  struct ItemDefinitionGroup {
    ItemDefinitionGroup(const tinyxml2::XMLElement *idg) {
      const char *condAttr = idg->Attribute("Condition");
      if (condAttr)
          condition = condAttr;
      for (const tinyxml2::XMLElement *e1 = idg->FirstChildElement(); e1; e1 = e1->NextSiblingElement()) {
            if (std::strcmp(e1->Name(), "ClCompile") != 0)
                continue;
            for (const tinyxml2::XMLElement *e = e1->FirstChildElement(); e; e = e->NextSiblingElement()) {
                if (std::strcmp(e->Name(), "PreprocessorDefinitions") == 0)
                    preprocessorDefinitions = e->GetText();
                else if (std::strcmp(e->Name(), "AdditionalIncludeDirectories") == 0)
                    additionalIncludePaths = e->GetText();
            }
        }
    }
    bool conditionIsTrue(const ProjectConfiguration &p) const {
        std::string c = condition;
        std::string::size_type pos = 0;
        while ((pos = c.find("$(Configuration)")) != std::string::npos) {
            c.erase(pos,16);
            c.insert(pos,p.configuration);
        }
        while ((pos = c.find("$(Platform)")) != std::string::npos) {
          c.erase(pos, 11);
          c.insert(pos, p.platform);
        }
        // TODO : Better evaluation
        Settings s;
        std::istringstream istr(c);
        TokenList tokens(&s);
        tokens.createTokens(istr);
        tokens.createAst();
        for (const Token *tok = tokens.front(); tok; tok = tok->next()) {
            if (tok->str() == "==" && tok->astOperand1() && tok->astOperand2() && tok->astOperand1()->str() == tok->astOperand2()->str())
                return true;
        }
        return false;
    }
    std::string condition;
    std::string preprocessorDefinitions;
    std::string additionalIncludePaths;
  };
};

static std::list<std::string> toStringList(const std::string &s) {
    std::list<std::string> ret;
    std::string::size_type pos1 = 0;
    std::string::size_type pos2;
    while ((pos2 = s.find(";",pos1)) != std::string::npos) {
        ret.push_back(s.substr(pos1, pos2-pos1));
        pos1 = pos2 + 1;
        if (pos1 >= s.size())
            break;
    }
    if (pos1 < s.size())
        ret.push_back(s.substr(pos1));
    return ret;
}

void Settings::importVcxproj(const std::string &filename)
{
    std::list<ProjectConfiguration> projectConfigurationList;
    std::list<std::string> compileList;
    std::list<ItemDefinitionGroup> itemDefinitionGroupList;

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError error = doc.LoadFile(filename.c_str());
    if (error != tinyxml2::XML_SUCCESS)
        return;
    const tinyxml2::XMLElement * const rootnode = doc.FirstChildElement();
    if (rootnode == nullptr)
      return;
    for (const tinyxml2::XMLElement *node = rootnode->FirstChildElement(); node; node = node->NextSiblingElement()) {
        if (std::strcmp(node->Name(), "ItemGroup") == 0) {
            if (node->Attribute("Label") && std::strcmp(node->Attribute("Label"), "ProjectConfigurations") == 0) {
                for (const tinyxml2::XMLElement *cfg = node->FirstChildElement(); cfg; cfg = cfg->NextSiblingElement()) {
                    if (std::strcmp(cfg->Name(), "ProjectConfiguration") == 0)
                        projectConfigurationList.push_back(ProjectConfiguration(cfg));
                }
            } else {
              for (const tinyxml2::XMLElement *e = node->FirstChildElement(); e; e = e->NextSiblingElement()) {
                if (std::strcmp(e->Name(), "ClCompile") == 0)
                  compileList.push_back(e->Attribute("Include"));
              }
            }
        } else if (std::strcmp(node->Name(), "ItemDefinitionGroup") == 0) {
            itemDefinitionGroupList.push_back(ItemDefinitionGroup(node));
        }
    }

    for (std::list<std::string>::const_iterator c = compileList.begin(); c != compileList.end(); ++c) {
        for (std::list<ProjectConfiguration>::const_iterator p = projectConfigurationList.begin(); p != projectConfigurationList.end(); ++p) {
            for (std::list<ItemDefinitionGroup>::const_iterator i = itemDefinitionGroupList.begin(); i != itemDefinitionGroupList.end(); ++i) {
                if (!i->conditionIsTrue(*p))
                    continue;
                     FileSettings fs;
                     fs.filename = Path::simplifyPath(Path::getPathFromFilename(filename) + *c);
                     fs.defines  = i->preprocessorDefinitions;
                     fs.includePaths = toStringList(i->additionalIncludePaths);
                     if (p->platform == "Win32")
                         fs.platformType = Win32W;
                     else if (p->platform == "x64")
                         fs.platformType = Win64;
                     fileSettings.push_back(fs);
            }
        }
    }
}
