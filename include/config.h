#ifndef CONFIG_H
#define CONFIG_H
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <vector>

struct Config
{

    std::string data_path;
    int id; // 0:Trader1, 1:Trader2, 2:Exchanger1, 3:Exchanger2
    std::string ip[4];
    int open_ports[4][4][3] = {};
    bool has_connection[4][4] = {{0, 0, 1, 1}, {0, 0, 1, 1}, {1, 1, 0, 0}, {1, 1, 0, 0}};

    inline std::string identity_string()
    {
        switch (id)
        {
        case 0:
            return std::string("Trader1");
        case 1:
            return std::string("Trader2");
        case 2:
            return std::string("Exchanger1");
        case 3:
            return std::string("Exchanger2");
        default:
            exit(1);
            break;
        }
    }

    inline static Config parse_config(int argc, char *argv[])
    {
        if (argc != 3)
        {
            printf("Usage: config_path self_id(0:Trader1, 1:Trader2, 2:Exchanger1, 3:Exchanger2)\n");
            exit(0);
        }
        Config config;
        
        config.id = atoi(argv[2]);
        if (config.id < 0 || config.id >= 4)
        {
            printf("Not a valid id\n");
            exit(1);
        }

        auto inconf = std::ifstream(argv[1]);
        inconf >> config.data_path;
        for (size_t i = 0; i < 4; i++)
        {
            inconf >> config.ip[i];
        }
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                if (config.has_connection[i][j])
                {
                    for (size_t k = 0; k < 3; k++)
                    {
                        inconf >> config.open_ports[i][j][k];
                    }
                }
            }
        }
        return config;
    };

    inline void info()
    {
        printf("id : %d\n", id);
        for (size_t i = 0; i < 4; i++)
        {
            printf("server %d ip %s\n", i, ip[i].c_str());
        }
        for (size_t i = 0; i < 4; i++)
        {
            for (size_t j = 0; j < 4; j++)
            {
                if (has_connection[i][j])
                {
                    printf("server %d to sever %d at port: ", i, j);
                    for (size_t k = 0; k < 3; k++)
                    {
                        printf("%d ", open_ports[i][j][k]);
                    }
                    printf("\n");
                }
            }
        }
    }
    inline std::filesystem::path hook_path()
    {
        return std::filesystem::path(data_path) / std::filesystem::path("hook.h5");
    }
    inline std::filesystem::path what_path(const char *what)
    {
        std::filesystem::path filename = std::string(what) + std::to_string(id + 1) + ".h5";
        return std::filesystem::path(data_path) / filename;
    }
};

inline std::vector<std::string> split(std::string s, char c)
{
    std::stringstream ss(s);
    std::vector<std::string> re;
    std::string x;
    while (std::getline(ss, x))
    {
        re.push_back(x);
    }
    return re;
}

inline void change_port(int previous, int now)
{
}

inline void cmd(std::string x, Config &conf)
{
    auto xs = split(x, ' ');
    if (xs.empty())
    {
        printf(
            "Usage:\n"
            "       q, exit\n"
            "       set portA portB, set connection from portA to portB\n");
        return;
    }
    if (xs[0] == "set")
    {
        if (xs.size() != 3)
        {
            printf(
                "Usage:\n"
                "       q, exit\n"
                "       set portA portB, set connection from portA to portB\n");
            return;
        }

        auto previous = std::stoi(xs[1]);
        auto now = std::stoi(xs[2]);

        bool ok = false;
        for (size_t i = 0; i < 4; i++)
        {
            if (conf.has_connection[conf.id][i])
            {
                for (size_t k = 0; k < 3; k++)
                {
                    if (conf.open_ports[conf.id][i][k] == previous)
                    {
                        conf.open_ports[conf.id][i][k] = now;
                        printf("change %s to %d, from %d to %d\n", conf.identity_string(), i, previous, now);
                        change_port(previous, now);
                        ok = true;
                    }
                }
            }
        }
    }
    else if (xs[0] == "info")
    {
        conf.info();
    }
    else
    {
        printf(
            "Usage:\n"
            "       q, exit\n"
            "       set portA portB, set connection from portA to portB\n");
    }
}

inline void cui(Config &conf)
{
    std::string x;
    std::cout << ":";
    while (std::getline(std::cin, x))
    {
        if (x == "q")
        {
            break;
        }
        cmd(x, conf);
        std::cout << ":";
    }

    std::cout << "Bye" << std::endl;
}

inline void maintain(int argc, char *argv[])
{
    auto conf = Config::parse_config(argc, argv);
    std::cout << "Confirm Config Info" << std::endl;
    conf.info();
    cui(conf);
}

#endif