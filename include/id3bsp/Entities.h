/*
Parses the entity sting embedded in BSP files, e.g.:

{
"model" "*41"
"target" "t31"
"targetname" "t336"
"classname" "trigger_aidoor"
}

Each Entity struct contains key-value pairs.
*/
#pragma once
#include <unordered_map>
#include <string>
#include <vector>

namespace id3bsp
{
    struct Entity
    {
        using Dictionary = std::unordered_map<std::string, std::string>;

        Dictionary KeyValuePairs;

        static bool Parse(const std::string& entityStr, const char* filename,
            std::vector<Entity>& entities);
    };
}
