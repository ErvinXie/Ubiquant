#include "config.h"

std::vector<std::string> split(std::string s, char c)
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

void change_port(int previous, int now)
{
}

void usage()
{
    printf(
        "Usage:\n"
        "       q, exit\n"
        "       set portA portB, set connection from portA to portB\n");
};

void cmd(std::string x, Config &conf)
{
    auto xs = split(x, ' ');
    if (xs.empty())
    {
        usage();
        return;
    }
    if (xs[0] == "set")
    {
        if (xs.size() != 3)
        {
            usage();
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
        usage();
    }
}

void set_port(std::vector<std::string> xs, Config &conf)
{
    if (xs.size() != 3)
    {
        usage();
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

Measure metrics[NR_STOCKS];
Measure *getMeasure(int stk_id_from_0){
    return &metrics[stk_id_from_0];
}


void cui(Config &conf)
{
    std::string x;
    std::cout << ":";
    while (std::getline(std::cin, x))
    {
        if (x == "q" || x =="quit" || x=="exit")
        {
            std::cout << "Bye" << std::endl;
            break;
        }
        auto xs = split(x, ' ');
        if (!xs.empty())
        {
            if (xs[0] == "set")
            {
                set_port(xs, conf);
            }
            else if (xs[0] == "info")
            {
                conf.info();
            }
            else if (xs[0] == "progress" || xs[0] =="p")
            {
                for (size_t i = 0; i < NR_STOCKS; i++)
                {
                    metrics[i].info();
                }
            }else{
                usage();
            }
        }
        std::cout << ":";
    }
}

void maintain(int argc, char *argv[])
{
    auto conf = Config::parse_config(argc, argv);
    std::cout << "Confirm Config Info" << std::endl;
    conf.info();
    cui(conf);
}